#include "DRAM.h"
#include "HDD.h"
#include "Iterator.h"
#include "TreeOfLosers.h"
#include <unistd.h>
#include <algorithm>
using namespace std;

DRAM::DRAM (int capacity, int page_size) 
: mergingTree(nullptr), ram_unsorted_ptr(0), capacity (capacity), page_size (page_size) {
    output_buffer.reserve(page_size);
}

DRAM::~DRAM () {

}

Row DRAM::getSortedRowFromRAM() {
    if(records.empty()) {
        return Row({std::numeric_limits<int>::max()},
                        std::numeric_limits<int>::min(),
                        std::numeric_limits<int>::max());
    }
    Row top = records.front();
    records.erase(records.begin());
    return top;
}

bool DRAM::addRecord(Row record, HDD& hdd) {
    if(records.size() < capacity) {
        // printf("HERE\n");
        records.push_back(record);
        ram_unsorted_ptr += 1;
        return true;
    } else {
        // printf("RAM CAP = %d\n", capacity);
        // printf("UPTR = %d\n", ram_unsorted_ptr);
        // ram is full
        // sort if ram_unsorted_ptr = capacity meaning that all records in ram are unsorted
        // load one record from ram to output buffer, if output buffer is full spill and empty it

        if(ram_unsorted_ptr == capacity) {
            // sort the records and prepare for sending to hdd
            // printf("GOING TO SORT\n");
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

            // printf("Output Buffer Full Flushing to Disk\n");
            // printOutputBuffer();

            hdd.addOutputBufferToSingleSortedRun(output_buffer);

            output_buffer.clear();
            // output_buffer.resize(page_size);
        }

        if(ram_unsorted_ptr == 0) {
            ram_unsorted_ptr = capacity;

            // one whole sorted run is flushed to disk at this point
            // so we ask hdd to store it as a sorted run
            // printf("Sorted Run Completed\n");
            hdd.addSingleSortedRunToSortedRuns();
        }

    }

    return false;
}

void ensurePageSize(std::vector<Row>& vec, size_t target_size, std::vector<Row>& overflow_buffer) {
    if (vec.size() > target_size) {
        // Move excess elements to overflow buffer
        overflow_buffer.insert(overflow_buffer.end(), vec.begin() + target_size, vec.end());
        // Resize the vector to match the page size
        vec.resize(target_size);
    } else if (vec.size() < target_size) {
        // Add -1 to fill the vector to match the page size
        vec.insert(vec.end(), target_size - vec.size(), Row({std::numeric_limits<int>::max()},
                        std::numeric_limits<int>::min(),
                        std::numeric_limits<int>::max()));
    }
}

void DRAM::sortPartiallyFilledRam(HDD& hdd) {
    // printf("\nPARTIAL PTR = %d\n", ram_unsorted_ptr);
    if(ram_unsorted_ptr == capacity) {
        // printf("BANG!!\n");
        sortRecords(records.size());
        
        hdd.addOutputBufferToSingleSortedRun(records);
        hdd.addSingleSortedRunToSortedRuns();

        records.clear();

        return;
    }

    // printf("\nPARTIAL SORT BEFORE\n");
    // printAllRecords();
    // from 0 to ram_unsorted_ptr ram is sorted after that it is unsorted
    std::rotate(records.begin(), records.begin() + ram_unsorted_ptr, records.end());

    // printAllRecords();

    sortRecords(capacity - ram_unsorted_ptr);

    // printf("\nPARTIAL SORT AFTER\n");
    // printAllRecords();

    // int firstRunStIdx = 0;
    // int firstRunEnd = capacity - ram_unsorted_ptr - 1;

    // int secondRunStIdx = firstRunEnd + 1;
    // int secondRunEnd = records.size() - 1;

    // printf("\nfirstRunStIdx = %d\n", firstRunStIdx);
    // printf("firstRunEnd = %d\n", firstRunEnd);
    // printf("secondRunStIdx = %d\n", secondRunStIdx);
    // printf("secondRunEnd = %d\n", secondRunEnd);

    int idx = capacity - ram_unsorted_ptr - 1;

    hdd.addOutputBufferToSingleSortedRun(output_buffer);
    output_buffer.clear();

    std::vector<Row>& spilledSortedRun = hdd.getSingleSortedRun();

    std::vector<Row> sortedRun1(records.begin(), records.begin() + idx + 1);
    std::vector<Row> sortedRun2(records.begin() + idx + 1, records.end());

    sortedRun2.insert(sortedRun2.end(), spilledSortedRun.begin(), spilledSortedRun.end());

    // records size will always be even (since capacity of ram would be even)
    size_t target_size = records.size() / 2;

    std::vector<Row> overflow_buffer1;
    std::vector<Row> overflow_buffer2;

    ensurePageSize(sortedRun1, target_size, overflow_buffer1);
    ensurePageSize(sortedRun2, target_size, overflow_buffer2);

    sortedRun1.insert(sortedRun1.end(), sortedRun2.begin(), sortedRun2.end());
    records = sortedRun1;

    lastWinnerRunIdx = -1;
    std::vector<int> emptyIndirection;
    mergingTree = std::make_unique<TreeOfLosers>(records, target_size, 2 * target_size, currentIndices, lastWinnerRunIdx, 0, false, emptyIndirection);
    mergingTree->initializeTree();

    Row nextRow;
    while ((nextRow = mergingTree->getNextRow()).offsetValue != INT_MAX) {
        // printf("LWI = %d\n", lastWinnerRunIdx);
        output_buffer.push_back(nextRow);

        // check if lastWinnerRunIdx is at end of it's range then reload
        int endIdxForRun = (lastWinnerRunIdx+1) * target_size - 1;

        // This condition checks if run is going to exhaust and hence loads the corresponding buffer with more data if available
        if(currentIndices[lastWinnerRunIdx] >= endIdxForRun) {
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
            
            // loadBufferFromRun(sortedRunIdx, run, sortedRunStIdx); // Load the first `pageSize` rows
            currentIndices[lastWinnerRunIdx] = lastWinnerRunIdx * target_size;
        }

        if(output_buffer.size() == page_size) {
            hdd.addBufferToMergedRun(output_buffer);
            output_buffer.clear();
        }

    }

    cleanupMerging(hdd);

    hdd.clearEmptySortedRuns();


    // At this point we have 2 sorted runs in the RAM
    // 1st run from 0 to (capacity - ram_unsorted_ptr - 1)
    // 2nd run from ram_unsorted_ptr to (capacity - 1) 
    // and some part of the 2nd run is spilled on ram in the single sorted run

    // So we can merge these 2 runs we use the 2 buffers at the start for merging

}

void printArray(std::vector<int> arr, int offset, int offsetValue, int slot) {
    printf("%d -> [", slot);
    for(int i=0 ; i < arr.size() ; i++) {
        printf("%d", arr[i]);
        if(i < arr.size()-1) printf(", ");
    }
    printf("]  |  Offset = %d  |  Offset Value = %d\n", offset, offsetValue);
}

void DRAM::printAllRecords() {
    printf("Printing RAM !!!!\n");
    for(int i=0 ; i < records.size() ; i++) {
        printArray(records[i].columns, records[i].offset, records[i].offsetValue, i);
    }
}

void DRAM::printOutputBuffer() {
    if(output_buffer.empty()) {
        printf("Output Buffer EMPTY\n");
        return;
    }

    printf("Output Buffer:\n");
    for(int i=0 ; i < output_buffer.size() ; i++) {
        printArray(output_buffer[i].columns, output_buffer[i].offset, output_buffer[i].offsetValue, i);
    }
}

int getCacheSize(int num) {
    // Vector to store divisors
    std::vector<int> divisors;

    // Find all divisors
    for (int i = 2; i <= num / 2; ++i) {
        if (num % i == 0) {
            divisors.push_back(i);
        }
    }
    divisors.push_back(num); // Add the number itself

    // Find middle divisor
    int size = divisors.size();
    if (size % 2 == 0) {
        return divisors[size / 2 - 1]; // Return the left one if even
    } else {
        return divisors[size / 2]; // Return the middle one if odd
    }
}

// void DRAM::sortRecords() {
//     std::vector<int> currentIndices;           // Current index in each run
//     int lastWinnerRunIdx = -1;

//     TreeOfLosers sortingTree(records, 1, records.size(), currentIndices, lastWinnerRunIdx);
//     sortingTree.initializeTree();
    

//     std::vector<Row> temp_buffer;

//     Row nextRow;
//     while ((nextRow = sortingTree.getNextRow()).offsetValue != INT_MAX) {
//         temp_buffer.push_back(nextRow);
//         //printf("lastWinnerRunIdx = %d\n", lastWinnerRunIdx);
//     }

//     flushRAM();
//     records = temp_buffer;
// }

void DRAM::sortInPlaceUsingSortIdx(std::vector<int> sortIdx, int N) {
    //size_t N = records.size();  // Get the size of records (and sortIdx)
    using std::swap; // To permit Koenig lookup
    
    // Loop through all records and apply the permutation
    for (size_t i = 0; i < N; i++) {
        int current = i;

        // Loop through until the element is in its correct position
        while (i != sortIdx[current]) {
            int next = sortIdx[current];
            // Swap records[current] and records[next]
            swap(records[current], records[next]);
            
            // Mark the current position as processed
            sortIdx[current] = current;
            current = next;
        }
        sortIdx[current] = current; // Finalize the cycle
    }
}

void DRAM::sortRecords(int sortingSize) {
    if(sortingSize == 1) return;

    std::vector<int> sortedCacheRunsIndexes;
    int winnerRunIdx = -1;

    // Cache Sized runs sorting
    int cacheSize = getCacheSize(sortingSize);
    // printf("Sort Size = %d | Cache Size = %d\n", sortingSize, cacheSize);
    for(int i = 0; i < sortingSize; i += cacheSize) {
        int end = std::min(i + cacheSize, (int)sortingSize);

        if((end - i) == 1) {
            sortedCacheRunsIndexes.push_back(i);
            continue;
        }

        std::vector<int> currIdx;    // Current index in each run

        TreeOfLosers cacheSortingTree(records, 1, (end-i), currIdx, winnerRunIdx, i, false, sortedCacheRunsIndexes);
        cacheSortingTree.initializeTree();

        Row nextRow;
        while ((nextRow = cacheSortingTree.getNextRow()).offsetValue != INT_MAX) {
            sortedCacheRunsIndexes.push_back(winnerRunIdx + i);
        }
    }

    // printf("Cache Done\n");
    // printf("[");
    // for(int i=0 ; i < sortedCacheRunsIndexes.size() ; i++) {
    //     printf("%d", sortedCacheRunsIndexes[i]);
    //     if(i < sortedCacheRunsIndexes.size()-1) printf(", ");
    // }
    // // printf("]  |  Offset = %d  |  Offset Value = %d\n", row.offset, row.offsetValue);
	// printf("]\n");

    std::vector<int> currIdx;    // Current index in each run

    TreeOfLosers sortingTree(records, cacheSize, sortingSize, currIdx, lastWinnerRunIdx, 0, true, sortedCacheRunsIndexes);
    sortingTree.initializeTree();

    Row nextRow;

    std::vector<int> sortIdx;
    while ((nextRow = sortingTree.getNextRow()).offsetValue != INT_MAX) {
        // printf("LWI = %d\n", lastWinnerRunIdx);
        int tp = sortedCacheRunsIndexes[currIdx[lastWinnerRunIdx] - 1];
        sortIdx.push_back(tp);     
    }

    // printf("GGG\n");

    // printf("[");
    // for(int i=0 ; i < sortIdx.size() ; i++) {
    //     printf("%d", sortIdx[i]);
    //     if(i < sortIdx.size()-1) printf(", ");
    // }
    // // printf("]  |  Offset = %d  |  Offset Value = %d\n", row.offset, row.offsetValue);
	// printf("]\n");


    sortInPlaceUsingSortIdx(sortIdx, sortingSize);

}

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

    // hdd.printSortedRunsSize();

    while(hdd.getNumOfSortedRuns() > B) {
        // load runs mergingRunSt -> mergingRunEnd into DRAM and merge
        mergeRuns(hdd, mergingRunSt, mergingRunEnd, X);
        mergingRunSt = mergingRunEnd + 1;
        mergingRunEnd = std::min(mergingRunSt + B, (int)W) - 1;

        if(mergingRunSt >= sortedRunEnd) {
            // clear all empty runs
            hdd.clearEmptySortedRuns();
            // hdd.printSortedRunsSize();
            mergingRunSt = 0;
            mergingRunEnd = std::min(mergingRunSt + B, (int)W) - 1;
            X = B;

            sortedRunEnd = hdd.getNumOfSortedRuns();
        }
    }
    hdd.clearEmptySortedRuns();
}

TreeOfLosers& DRAM::getMergingTree() {
    return *mergingTree;
}

void DRAM::prepareMergingTree(std::vector<std::vector<Row> >& sortedRuns, int sortedRunStIdx, int sortedRunEndIdx, int X) {
    // printf("sortedRunStIdx = %d | sortedRunEndIdx = %d | X = %d\n", sortedRunStIdx, sortedRunEndIdx, X);
    

    // TODO: Need to do this carefully can't just flush the RAM directly
    flushRAM();
    records.resize(capacity);

    for(int sortedRunIdx = sortedRunStIdx ; sortedRunIdx <= sortedRunEndIdx ; sortedRunIdx++) {
        loadBufferFromRun(sortedRunIdx, sortedRuns[sortedRunIdx], sortedRunStIdx); // Load the first `pageSize` rows
    }
    
    lastWinnerRunIdx = -1;
    std::vector<int> emptyIndirection;
    mergingTree = std::make_unique<TreeOfLosers>(records, page_size, (sortedRunEndIdx - sortedRunStIdx + 1) * page_size, currentIndices, lastWinnerRunIdx, 0, false, emptyIndirection);
    mergingTree->initializeTree();

    outputBufferStIdx = capacity - page_size;
    outputBufferIdx = outputBufferStIdx;
}

void DRAM::cleanupMerging(HDD& hdd) {
    // Write any remaining rows in the output buffer to HDD

    if (!output_buffer.empty()) {
        hdd.addBufferToMergedRun(output_buffer);
        output_buffer.clear();
    }
    // if (outputBufferIdx < capacity) {
    //     hdd.addBufferToMergedRun(std::vector<Row>(records.begin() + outputBufferStIdx, records.begin() + outputBufferIdx ));
    // }

    hdd.appendMergedRunsToSortedRuns();
}

Row DRAM::getNextSortedRow(HDD& hdd, int sortedRunStIdx, int X) {

    TreeOfLosers& sortingTree = getMergingTree();
    Row nextRow = sortingTree.getNextRow();

    if(nextRow.offsetValue == INT_MAX) return nextRow;
    
    output_buffer.push_back(nextRow);
    // records[outputBufferIdx] = nextRow;
    // outputBufferIdx += 1;

    // check if lastWinnerRunIdx is at end of it's range then reload
    int endIdxForRun = (lastWinnerRunIdx+1) * page_size - 1;

    // This condition checks if run is going to exhausts and hence loads the corresponding buffer with more data if available
    if(currentIndices[lastWinnerRunIdx] >= endIdxForRun) {
        // Actual runIdx in sortedRuns = sortedRunStIdx + lastWinnerRunIdx
        int sortedRunIdx = sortedRunStIdx + lastWinnerRunIdx;

        std::vector<std::vector<Row> >& sortedRuns = hdd.getSortedRuns();
        
        std::vector<Row>& run = sortedRuns[sortedRunIdx];
        run.insert(run.begin(), records[endIdxForRun]);
        
        loadBufferFromRun(sortedRunIdx, run, sortedRunStIdx); // Load the first `pageSize` rows
        currentIndices[lastWinnerRunIdx] = lastWinnerRunIdx * page_size;
    }

    if(output_buffer.size() == page_size) {
        hdd.addBufferToMergedRun(output_buffer);
        output_buffer.clear();
    }

    // if(outputBufferIdx >= capacity) {
    //     // flush output buffer to hdd
    //     hdd.addBufferToMergedRun(std::vector<Row>(records.begin() + outputBufferStIdx, records.begin() + capacity ));
    //     outputBufferIdx = outputBufferStIdx;
    // }

    return nextRow;
    
}

void DRAM::mergeRuns(HDD& hdd, int sortedRunStIdx, int sortedRunEndIdx, int X) {

    if(sortedRunStIdx == 0) {
        printf("------------------------- Pass %d : Merging -------------------------\n", pass);
        pass++;
    }

    prepareMergingTree(hdd.getSortedRuns(), sortedRunStIdx, sortedRunEndIdx, X);

    // Merge rows using TreeOfLosers
    Row nextRow;
    while ((nextRow = getNextSortedRow(hdd, sortedRunStIdx, X)).offsetValue != INT_MAX);

    cleanupMerging(hdd);

}

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

std::vector<Row> DRAM::getAllRecords() {
    return records;
}

void DRAM::flushRAM() {
    records.clear();
}

bool DRAM::isFull() {
    return records.size() >= capacity;
}

bool DRAM::isEmpty() {
    return records.size() == 0;
}

int DRAM::getCapacity() {
    return capacity;
}
