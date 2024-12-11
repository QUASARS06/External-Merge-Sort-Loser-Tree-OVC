#include "DRAM.h"
#include "HDD.h"
#include "Iterator.h"
#include "TreeOfLosers.h"
#include <unistd.h>
#include <algorithm>
using namespace std;

DRAM::DRAM (int capacity, int page_size) : mergingTree(nullptr), ram_unsorted_ptr(0), 
                                           capacity (capacity), page_size (page_size) 
{
    output_buffer.reserve(page_size);
}

DRAM::~DRAM () {}


/**
 * When the whole input fits in RAM then the sorted records are present in RAM
 * This method returns one sorted record present in RAM on each invocation of next() in SortIterator
 */
Row DRAM::getSortedRowFromRAM() {
    // if RAM gets empty then return a Positive Fence Record
    if(records.empty()) {
        return Row({std::numeric_limits<int>::max()},
                        std::numeric_limits<int>::min(),
                        std::numeric_limits<int>::max());
    }

    // Pops and Returns Record at start of RAM
    Row top = records.front();
    records.erase(records.begin());
    return top;
}


/**
 * Used when there is only a single sorted run on HDD after internal sort
 * We load sorted records from HDD on RAM and return one sorted record
 * on each invocation of next() in SortIterator
 */
Row DRAM::getRowFromSingleSortedRunOnHDD(HDD& hdd) {

    // If the RAM is empty we check if HDD has any more records
    // If Yes we load them onto RAM and return Row
    // Otherwise we load a Positive Fence Record to denote end of Sorted Run
    if(records.empty()) {
        std::vector<Row>& hddSortedRun = hdd.getSortedRuns()[0];

        for(int i=0;i<capacity;i++) {
            if(hddSortedRun.empty()) break;

            Row r = hddSortedRun.front();
            records.push_back(r);
            hddSortedRun.erase(hddSortedRun.begin());
        }

        if(hddSortedRun.empty()) {
            hddSortedRun.push_back(Row({std::numeric_limits<int>::max()},
                        std::numeric_limits<int>::min(),
                        std::numeric_limits<int>::max()));
        }
    }

    // Pops and Returns Record at start of RAM
    Row top = records.front();
    records.erase(records.begin());
    return top;
}


/**
 * Adds a Row to RAM
 */
bool DRAM::addRecord(Row record, HDD& hdd) {
    // If RAM has space for more records simply add record to RAM
    if(records.size() < capacity) {
        records.push_back(record);
        ram_unsorted_ptr += 1;
        return true;
    } 
    
    // If RAM is full and unsorted we sort it (but don't spill anything to HDD after sorting)
    
    // We start filling the output buffer with sorted records, when we get more input records
    // Once output buffer is full it is spilled to HDD (thus we achieve Graceful Degradation here)
    // Instead of spilling whole sorted run at once we only spill as much as required
    // to accomodate more input records

    // once the RAM is full the new records are loaded in the RAM in opposite order meaning from end of RAM (vector) 
    // this makes subsequent merging efficient as we have start of previously sorted run in RAM
    // this is controlled by the ram_unsorted_ptr which points to the start of unsorted records being loaded to RAM
    // Once all input has been loaded to RAM the ram_unsorted_ptr helps segregate the sorted and unsorted parts in RAM

    else {
        // ram is full
        // sort if ram_unsorted_ptr = capacity meaning that all records in ram are unsorted
        if(ram_unsorted_ptr == capacity) {
            // sort the records and prepare for sending to hdd
            sortRecords(records.size());
        }

        ram_unsorted_ptr -= 1;

        // remove the element at ram_unsorted_ptr and push to output buffer in sorted manner
        Row r = records[ram_unsorted_ptr];
        output_buffer.insert(output_buffer.begin(), r);

        // add the NEW record to RAM
        records[ram_unsorted_ptr] = record;

        // check if output_buffer is full then we need to flush to disk
        if(output_buffer.size() == page_size) {
            // flush to disk

            // we maintain a different vector (single_sorted_run) on HDD to collect the sorted run
            // since the spilling will happen one output buffer at a time
            hdd.addOutputBufferToSingleSortedRun(output_buffer);
            output_buffer.clear();
        }

        // cycle back the ram_unsorted_ptr if we have reached start of RAM
        if(ram_unsorted_ptr == 0) {
            ram_unsorted_ptr = capacity;

            // one whole sorted run of memory size (M) is flushed to disk at this point
            // so we ask hdd to store it as a sorted run
            hdd.addSingleSortedRunToSortedRuns();
        }

    }

    return false;
}


/**
 * Ensures that the number of Rows in vec == target_size
 * Additional Rows are added to the overflow_buffer
 * If number of Rows in vec < target_size then we add Positive Fence Rows
 */
void ensureTargetSize(std::vector<Row>& vec, size_t target_size, std::vector<Row>& overflow_buffer) {
    if (vec.size() > target_size) {
        // Move excess elements to overflow buffer
        overflow_buffer.insert(overflow_buffer.end(), vec.begin() + target_size, vec.end());

        // Resize the vector to match the target size
        vec.resize(target_size);
    } else if (vec.size() < target_size) {
        
        // Add Positive Fence Rows to fill the vector to match the target size
        vec.insert(vec.end(), target_size - vec.size(), Row({std::numeric_limits<int>::max()},
                        std::numeric_limits<int>::min(),
                        std::numeric_limits<int>::max()));
    }
}


/**
 * After adding records to RAM some records may still be present in the RAM which are unsorted
 * (usually for the last few records) we need to sort these records
 * 
 * This is because sorting is triggered in the addRecord(...) function only when RAM is full
 * below function handles the scenario when scan stops generating record even before RAM is full
 */
void DRAM::sortPartiallyFilledRam(HDD& hdd) {

    // Incase where the last set of input records perfectly fit into RAM
    // ram_unsorted_ptr will be equal to capacity
    // hence we simply sort the records in RAM
    // spill this sorted run to HDD and save in sorted_runs on HDD
    if(ram_unsorted_ptr == capacity) {
        sortRecords(records.size());
        
        hdd.addOutputBufferToSingleSortedRun(records);
        hdd.addSingleSortedRunToSortedRuns();

        records.clear();

        printf("\n-----------------------------------------------------------------------------------------\n");
        printf("HDD Total Spill after Sorting - %d (Spill <= Row Count due to Graceful Degradation)\n", hdd.getSpillCount());
        printf("-----------------------------------------------------------------------------------------\n");
        return;
    }

    // this part of code will be evaluated if some part of RAM is sorted and remaining part is unsorted

    // from 0 to ram_unsorted_ptr ram is sorted after that it is unsorted
    // hence we rotate records in the RAM to bring unsorted records to start (implementation requirement)
    // and then sort the unsorted part of the RAM
    std::rotate(records.begin(), records.begin() + ram_unsorted_ptr, records.end());
    sortRecords(capacity - ram_unsorted_ptr);

    // idx is basically the barrier to differentiate between the two sorted runs in RAM
    int idx = capacity - ram_unsorted_ptr - 1;

    // drain output_buffer
    hdd.addOutputBufferToSingleSortedRun(output_buffer);
    output_buffer.clear();

    // At this point we have 3 sets of sorted rows
    // [0, idx] - Sorted in RAM
    // [idx+1, ram_capacity] - Sorted in RAM   +   Spilled records for this sorted run on HDD (in single_sorted_run)
    // these records were (gracefully) spilled to accomodate the records in [0, idx]

    // thus to account for this spill we add the size of these records to the spill count
    hdd.addSingleSortedRunCountToSpillCount();

    // At this point internal sorting is over
    printf("\n-----------------------------------------------------------------------------------------\n");
    printf("HDD Total Spill after Sorting - %d (Spill <= Row Count due to Graceful Degradation)\n", hdd.getSpillCount());
    printf("-----------------------------------------------------------------------------------------\n");

    // But what we do is instead of having 2 smaller runs as described above in RAM 
    // (some part of 2nd run is of course spilled and present on HDD)

    // We merge these two runs into one larger run below

    // sortedRun1 , sortedRun2 + spilledSortedRun are the two sorted runs which we have
    std::vector<Row>& spilledSortedRun = hdd.getSingleSortedRun();

    std::vector<Row> sortedRun1(records.begin(), records.begin() + idx + 1);
    std::vector<Row> sortedRun2(records.begin() + idx + 1, records.end());

    // what we do next is divide the whole RAM into 2 even parts and load the 2 runs into it for merging
    // the overflow_buffers help accomodate extra records

    // we can argue that there in no space on RAM to accomodate this overflow_buffers
    // we can assume that the overflow_buffers will be spilled back to HDD once we have prepared the RAM for merging
    // so there might be a very small increase in spill_count which we don't account for here
    // So this is a small assumption from our end just to make implementation simpler

    // sortedRun2 + spilledSortedRun
    sortedRun2.insert(sortedRun2.end(), spilledSortedRun.begin(), spilledSortedRun.end());

    // records size will always be even (since capacity of ram would be even)
    size_t target_size = records.size() / 2;

    std::vector<Row> overflow_buffer1;
    std::vector<Row> overflow_buffer2;

    // makes sortedRuns of target_size and extra records added to overflow_buffers
    // incase of underflow Positive Fence Records are added to sortedRuns
    ensureTargetSize(sortedRun1, target_size, overflow_buffer1);
    ensureTargetSize(sortedRun2, target_size, overflow_buffer2);

    // set the RAM to sortedRun1 + sortedRun2 (each of target_size)
    sortedRun1.insert(sortedRun1.end(), sortedRun2.begin(), sortedRun2.end());
    records = sortedRun1;

    // Merge them using Tree of Losers
    // note below that the pageSize in TOL = target_size 
    // meaning each buffer/run competing in the Tree of Losers has target_size number of Rows/Records

    lastWinnerRunIdx = -1;
    std::vector<int> emptyIndirection;
    mergingTree = std::make_unique<TreeOfLosers>(records, target_size, 2 * target_size, currentIndices, lastWinnerRunIdx, 0, false, emptyIndirection);
    mergingTree->initializeTree();

    Row nextRow;
    while ((nextRow = mergingTree->getNextRow()).offsetValue != INT_MAX) {
        output_buffer.push_back(nextRow);

        // check if lastWinnerRunIdx is at end of it's range then reload
        int endIdxForRun = (lastWinnerRunIdx+1) * target_size - 1;

        // This condition checks if run is going to exhaust and hence loads the corresponding buffer with more data if available
        if(currentIndices[lastWinnerRunIdx] >= endIdxForRun) {
            
            // if any buffer of a run is going to exhaust we load the buffer with more rows from overflow_buffer
            std::vector<Row>& run = lastWinnerRunIdx == 0 ? overflow_buffer1 : overflow_buffer2;
            run.insert(run.begin(), records[endIdxForRun]);

            for(int i=lastWinnerRunIdx*target_size;i<=endIdxForRun;i++) {
                if(run.size() > 0) {
                    records[i] = run.front();
                    run.erase(run.begin());
                } else {
                    records[i] = Row({std::numeric_limits<int>::max()},
                        std::numeric_limits<int>::min(),
                        std::numeric_limits<int>::max());
                }  
            }
            
            // since we reloaded the buffer for a particular run
            // we also reload currentIndices to point to start of run in RAM
            currentIndices[lastWinnerRunIdx] = lastWinnerRunIdx * target_size;
        }

        // spill output_buffer to HDD once full
        if(output_buffer.size() == page_size) {
            hdd.addBufferToMergedRun(output_buffer);
            output_buffer.clear();
        }

    }

    // cleans up partially filled output buffers and spill it to HDD
    cleanupMerging(hdd);

    // clears any empty sorted runs on HDD
    hdd.clearEmptySortedRuns();

    flushRAM();
}


/**
 * While Cache Size would be fixed, in this implementation the cache size is basically
 * the middle divisor of the number of records we are sorting in memory
 * for larger memory sizes the cache size maybe be unreasonable using this approach but this is just an
 * implementation choice and the code will work even with a fixed small cache size (>= 2)
 */
int getCacheSize(int num) {
    std::vector<int> divisors;

    for (int i = 2; i <= num / 2; ++i) {
        if (num % i == 0) {
            divisors.push_back(i);
        }
    }
    divisors.push_back(num);

    int size = divisors.size();
    return size % 2 == 0 ? divisors[size / 2 - 1] : divisors[size / 2];
}


/**
 * every index in sortIdx corresponds to => the index of Record in RAM which is supposed to be present at that index
 * Example: if RAM = [4, 1, 3, 2] (actual values)
 * then    sortIdx = [1, 3, 2, 0] (indexes)
 * Basically sortIdx[0] = 1 means that the element present at index 1 in RAM which is '1'
 *                        is supposed to be present at index 0 in RAM
 * 
 * This method sorts the RAM in place using the sortIdx vector
 */
void DRAM::sortInPlaceUsingSortIdx(std::vector<int> sortIdx, int N) {
    using std::swap;
    
    for (size_t i = 0; i < N; i++) {
        int current = i;

        while (i != sortIdx[current]) {
            int next = sortIdx[current];

            swap(records[current], records[next]);
            
            sortIdx[current] = current;
            current = next;
        }
        sortIdx[current] = current;
    }
}


/**
 * Sorts RAM from [0, sortingSize - 1] by dividing into cache sized runs
 * The cache sized runs are sorted using a Tree of Losers
 * Individual sorted cache sized runs are merged into a single sorted run again using a Tree of Losers
 */
void DRAM::sortRecords(int sortingSize) {
    
    // single record is already sorted
    if(sortingSize == 1) return;

    // this is the indirection which will be used later in the 2nd Tree of Losers for merging cache sized runs
    // more details about indirection is described in the Tree of Losers constructor comments
    // Simply speaking since we "practically" don't have space in RAM to store the cache sized sorted runs
    // We instead store the indexes (indirection) where each record is supposed to be when sorted
    std::vector<int> sortedCacheRunsIndexes;
    int winnerRunIdx = -1;

    // Cache Sized runs sorting
    int cacheSize = getCacheSize(sortingSize);

    // sorts cache sized portions of RAM
    for(int i = 0; i < sortingSize; i += cacheSize) {
        int end = std::min(i + cacheSize, (int)sortingSize);

        if((end - i) == 1) {
            sortedCacheRunsIndexes.push_back(i);
            continue;
        }

        std::vector<int> currIdx;    // Current index in each run

        // records - Tree of Losers ONLY works on the RAM, always the RAM is passed as the input runs to TOL
        //           the other parameters help in working on the appropriate region of the RAM like offset, etc
        // 1 - pageSize is 1 means that each run to be merged(hence sorted) is 1 record big
        // (end-i) - number of records/rows in this cache sized RAM region to be sorted(merged)
        // currIdx - (see TOL constructor comments)
        // winnerRunIdx - passed by reference since we need the run index of last winner (root)
        // i - offset within RAM which we are working on at the moment
        // false - no indirection
        // sortedCacheRunsIndexes - dummy won't be used since indirection is false
        TreeOfLosers cacheSortingTree(records, 1, (end-i), currIdx, winnerRunIdx, i, false, sortedCacheRunsIndexes);
        cacheSortingTree.initializeTree();

        Row nextRow;
        while ((nextRow = cacheSortingTree.getNextRow()).offsetValue != INT_MAX) {
            // i is added as an offset since subsequent cache runs start at index i
            // but the tree of losers treats them on 0-based indexing
            // thus the winnerRunIdx returned is 0-indexed and hence we add i to convert
            // the logical winnerRunIdx (in Tree of Losers) to actual index in RAM
            sortedCacheRunsIndexes.push_back(winnerRunIdx + i);
        }
    }

    std::vector<int> currIdx;    // Current index in each run

    // records - Tree of Losers ONLY works on the RAM, always the RAM is passed as the input runs to TOL
    //           the other parameters help in working on the appropriate region of the RAM like offset, etc
    // cacheSize - pageSize(in TOL) = cacheSize meaning each run has 1 cacheSize number of records
    //             since TOL merges pageSized buffers at a time hence we say that in this case the
    //             buffers are actually 1 cacheSize big
    // sortingSize - number of records/rows in RAM we are merging
    // currIdx - (see TOL constructor comments)
    // lastWinnerRunIdx - passed by reference since we need the run index of last winner (root)
    // 0 - offset within RAM is 0 since we are now merging runs in the whole RAM (upto sortingSize-1)
    // true - means we'll use indirection (see TOL constructor comments and getRow() in TOL)
    // sortedCacheRunsIndexes - indirection index vector to point to correct record in RAM
    TreeOfLosers sortingTree(records, cacheSize, sortingSize, currIdx, lastWinnerRunIdx, 0, true, sortedCacheRunsIndexes);
    sortingTree.initializeTree();

    Row nextRow;

    std::vector<int> sortIdx;
    while ((nextRow = sortingTree.getNextRow()).offsetValue != INT_MAX) {
        // lastWinnerRunIdx gives the run from which current winner Row came from
        // and currIdx points to the next Row which is competing from that Run (hence the -1)
        // combination of these indexes gives us the index of the Row in RAM which just won (logical)
        // but we have an indirection hence another level of translation is required mentioned below
    
        // sortedCacheRunsIndexes is the indirection vector which gives us the actual Row which won
        // and we append it to sortIdx which basically means that row at this particular index should
        // be present at the corresponding index in RAM (= index in sortIdx)
        int tp = sortedCacheRunsIndexes[currIdx[lastWinnerRunIdx] - 1];
        sortIdx.push_back(tp);     
    }

    // sorts the Rows in RAM in place using the indexes governing the sort in sortIdx
    sortInPlaceUsingSortIdx(sortIdx, sortingSize);

}


/**
 * Keeps merging sorted runs on HDD, until number of sorted runs on HDD are less than equal to B
 */
void DRAM::mergeSortedRuns(HDD& hdd) {
    pass = 1;

    int totalBuffers = capacity / page_size;

    // not doing -1 because we set the ram capacity with 1 page size deducted for output buffer
    int B = totalBuffers;
    int W = hdd.getNumOfSortedRuns();

    int X = (W-2) % (B-1) + 2;

    // 1-step to n-step Graceful Degradation
    // We use the forumla to merge only X runs at the start
    int mergingRunSt = 0;
    int mergingRunEnd = X-1;

    int sortedRunEnd = hdd.getNumOfSortedRuns();

    // keep merging sorted runs while number of sorted runs is greater than B
    // at the end of this while loop we will have that many sorted runs which can be merged in 1 merge step
    while(hdd.getNumOfSortedRuns() > B) {
        // load runs mergingRunSt -> mergingRunEnd into DRAM and merge
        mergeRuns(hdd, mergingRunSt, mergingRunEnd);

        // move to next B sorted runs
        mergingRunSt = mergingRunEnd + 1;
        mergingRunEnd = std::min(mergingRunSt + B, (int)W) - 1;

        // this means that we just merged all the sorted runs in this pass
        // now we have larger runs generated from previous pass
        // and a new pass is about to start
        // thus we reset all variables
        if(mergingRunSt >= sortedRunEnd) {
            // clear all empty runs (since sorted runs from HDD are loaded in RAM and will get empty after a point)
            hdd.clearEmptySortedRuns();

            // reset for next pass
            mergingRunSt = 0;
            mergingRunEnd = std::min(mergingRunSt + B, (int)W) - 1;

            sortedRunEnd = hdd.getNumOfSortedRuns();
        }
    }

    // cleanup
    hdd.clearEmptySortedRuns();
}


TreeOfLosers& DRAM::getMergingTree() {
    return *mergingTree;
}


/**
 * Given the sortedRuns (extracted from HDD)
 * loads the RAM buffers with page_size number of records from each run
 * prepares and initializes the Tree of Loser for Merging
 */
void DRAM::prepareMergingTree(std::vector<std::vector<Row> >& sortedRuns, int sortedRunStIdx, int sortedRunEndIdx) {
    // flush ram to fill Rows from each run to be merged in RAM
    flushRAM();
    records.resize(capacity);

    for(int sortedRunIdx = sortedRunStIdx ; sortedRunIdx <= sortedRunEndIdx ; sortedRunIdx++) {
        // Load the first `pageSize` rows from each run which is getting merged
        loadBufferFromRun(sortedRunIdx, sortedRuns[sortedRunIdx], sortedRunStIdx); 
    }
    
    lastWinnerRunIdx = -1;
    std::vector<int> emptyIndirection;
    mergingTree = std::make_unique<TreeOfLosers>(records, page_size, (sortedRunEndIdx - sortedRunStIdx + 1) * page_size, currentIndices, lastWinnerRunIdx, 0, false, emptyIndirection);
    mergingTree->initializeTree();
}


/**
 * Spills partially filled output buffer to HDD
 * and then appends the sorted merged run to sorted_runs on HDD
 */
void DRAM::cleanupMerging(HDD& hdd) {

    // Write any remaining rows in the output buffer to HDD and clear the output buffer
    if (!output_buffer.empty()) {
        hdd.addBufferToMergedRun(output_buffer);
        output_buffer.clear();
    }

    // append the merged (sorted) run to all the sorted runs present on HDD
    hdd.appendMergedRunsToSortedRuns();
}


/**
 * Pops the root from the Tree of Losers to get the next sorted Row
 * Then updates the tree by doing a leaf to root pass
 * Ensures that if a buffer for a Run in the RAM is going to exhaust then load the next buffer of sorted
 *          records from corresponding Run in HDD to that RAM buffer
 * Winners are appended to the output buffer and spilled to HDD once full
 */
Row DRAM::getNextSortedRow(HDD& hdd, int sortedRunStIdx) {

    TreeOfLosers& sortingTree = getMergingTree();
    Row nextRow = sortingTree.getNextRow();

    if(nextRow.offsetValue == INT_MAX) return nextRow;
    
    output_buffer.push_back(nextRow);

    // check if lastWinnerRunIdx is at end of it's range then reload
    int endIdxForRun = (lastWinnerRunIdx+1) * page_size - 1;

    // This condition checks if run is going to exhaust and hence loads the corresponding buffer with more data if available
    if(currentIndices[lastWinnerRunIdx] >= endIdxForRun) {
        // Actual runIdx in sortedRuns = sortedRunStIdx + lastWinnerRunIdx
        int sortedRunIdx = sortedRunStIdx + lastWinnerRunIdx;

        std::vector<std::vector<Row> >& sortedRuns = hdd.getSortedRuns();
        
        std::vector<Row>& run = sortedRuns[sortedRunIdx];
        
        // we proactively load the buffer when only one record is left in the buffer for that run
        // to simplify implementation we just add that one record back to start of run
        // and then load that RAM buffer again with the next page_size number of records from sorted run in HDD
        // incase the run doesn't have enough records left the remaining buffer is filled with Positive Fence records
        run.insert(run.begin(), records[endIdxForRun]);
        
        loadBufferFromRun(sortedRunIdx, run, sortedRunStIdx);

        // since we reloaded the buffer for a particular run
        // we also reload currentIndices to point to start of run in RAM
        currentIndices[lastWinnerRunIdx] = lastWinnerRunIdx * page_size;
    }

    // if output buffer is full spill to HDD
    // merged_run is a special vector again on HDD (like single_sorted_run before)
    // we stores the merged run for a pass and at end of pass this merged run is appended to the sorted runs
    // on HDD ensuring sorted order by run size
    if(output_buffer.size() == page_size) {
        hdd.addBufferToMergedRun(output_buffer);
        output_buffer.clear();
    }

    return nextRow;
    
}


/**
 * Merges sorted runs on HDD from sortedRunStIdx -> sortedRunEndIdx
 */
void DRAM::mergeRuns(HDD& hdd, int sortedRunStIdx, int sortedRunEndIdx) {

    // sortedRunStIdx = 0 means start of new pass, thus we print the pass log
    if(sortedRunStIdx == 0) {
        if(pass == 1) printf("------------------------- Pass %d : Merging (%d sorted runs & initial merge fan-in = %d) -------------------------\n", pass, hdd.getNumOfSortedRuns(), (sortedRunEndIdx - sortedRunStIdx + 1));
        else printf("------------------------- Pass %d : Merging (%d sorted runs) -------------------------\n", pass, hdd.getNumOfSortedRuns());

        pass++;
    }

    // prepare Tree of Losers merging tree to merge the runs from sortedRunStIdx -> sortedRunEndIdx
    prepareMergingTree(hdd.getSortedRuns(), sortedRunStIdx, sortedRunEndIdx);

    // Merge rows using TreeOfLosers
    Row nextRow;
    while ((nextRow = getNextSortedRow(hdd, sortedRunStIdx)).offsetValue != INT_MAX);

    cleanupMerging(hdd);

}


/**
 * Given a runIndex and corresponding run on HDD
 * Loads page_size number of records from that run into corresponding buffer on RAM (using runStIdx)
 * In case of underflow Positive Fence Rows are added to the corresponding buffer
 */
void DRAM::loadBufferFromRun(int runIndex, std::vector<Row>& run, int runStIdx) {
    // Calculate start and end indices for the next chunk
    int startIdxInDram = (runIndex - runStIdx) * page_size;

    for(int i=0;i<page_size;i++) {
        if(run.size() > 0) {
            records[startIdxInDram + i] = run.front();
            run.erase(run.begin());
        } else {
            records[startIdxInDram + i] = Row({std::numeric_limits<int>::max()},
                        std::numeric_limits<int>::min(),
                        std::numeric_limits<int>::max());
        }
    }
}


void DRAM::flushRAM() {
    records.clear();
}
