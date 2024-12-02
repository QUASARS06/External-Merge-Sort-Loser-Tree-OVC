#include "DRAM.h"
#include "HDD.h"
#include "Iterator.h"
#include "TreeOfLosers.h"
#include <unistd.h>
using namespace std;

DRAM::DRAM (int capacity, int page_size) : mergingTree(nullptr), capacity (capacity), page_size (page_size) {}

DRAM::~DRAM () {

}

bool DRAM::addRecord(Row record) {
    if(records.size() < capacity) {
        records.push_back(record);
        return true;
    }

    return false;
}

void printArray(std::vector<int> arr, int offset, int offsetValue) {
    printf("[");
    for(int i=0 ; i < arr.size() ; i++) {
        printf("%d", arr[i]);
        if(i < arr.size()-1) printf(", ");
    }
    printf("]  |  Offset = %d  |  Offset Value = %d\n", offset, offsetValue);
}

void DRAM::printAllRecords() {
    printf("Printing RAM !!!!\n");
    for(int i=0 ; i < records.size() ; i++) {
        printArray(records[i].columns, records[i].offset, records[i].offsetValue);
    }
}

int getCacheSize(int num) {
    // Vector to store divisors
    std::vector<int> divisors;

    // Find all divisors
    for (int i = 1; i <= num / 2; ++i) {
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

void DRAM::sortRecords() {
    if(records.size() == 1) return;

    std::vector<Row> sortedCacheRuns;
    int winnerRunIdx = -1;

    // Cache Sized runs sorting
    int cacheSize = getCacheSize(capacity);
    for(int i = 0; i < records.size(); i += cacheSize) {
        int end = std::min(i + cacheSize, (int)records.size());

        if((end - i) == 1) {
            sortedCacheRuns.push_back(records[i]);
            continue;
        }

        std::vector<int> currIdx;    // Current index in each run
        std::vector<Row> cacheSizedRecords = std::vector<Row>(records.begin() + i, records.begin() + end);

        TreeOfLosers cacheSortingTree(cacheSizedRecords, 1, cacheSizedRecords.size(), currIdx, winnerRunIdx);
        cacheSortingTree.initializeTree();

        Row nextRow;
        while ((nextRow = cacheSortingTree.getNextRow()).offsetValue != INT_MAX) {
            sortedCacheRuns.push_back(nextRow);
        }
    }
    flushRAM();
    records = sortedCacheRuns;

    // it is our assumption that ram_capacity will always be a multiple of page_size
    // so by that extension we need to make sure of it also for our cache sized runs
    // hence we fence with infinite rows to make this condition true
    int numOfFenceRows = std::ceil(records.size()*1.0/cacheSize) * cacheSize - records.size();
    for(int i=0;i<numOfFenceRows;i++) {
        records.push_back(Row({std::numeric_limits<int>::max()},
                        std::numeric_limits<int>::min(),
                        std::numeric_limits<int>::max()));
    }

    std::vector<int> currIdx;    // Current index in each run

    TreeOfLosers sortingTree(records, cacheSize, records.size(), currIdx, lastWinnerRunIdx);
    sortingTree.initializeTree();

    std::vector<Row> temp_buffer;

    Row nextRow;
    while ((nextRow = sortingTree.getNextRow()).offsetValue != INT_MAX) {
        temp_buffer.push_back(nextRow);
    }

    flushRAM();
    records = temp_buffer;

}

void DRAM::mergeSortedRuns(HDD& hdd) {
    pass = 1;

    int totalBuffers = capacity / page_size;
    int B = totalBuffers - 1;
    int W = hdd.getNumOfSortedRuns();

    int X = (W-2) % (B-1) + 2;

    int mergingRunSt = 0;
    int mergingRunEnd = X-1;

    int sortedRunEnd = hdd.getNumOfSortedRuns();

    while(hdd.getNumOfSortedRuns() > B) {
        // load runs mergingRunSt -> mergingRunEnd into DRAM and merge
        mergeRuns(hdd, mergingRunSt, mergingRunEnd, X);
        mergingRunSt = mergingRunEnd + 1;
        mergingRunEnd = std::min(mergingRunSt + B, (int)W) - 1;

        if(mergingRunSt >= sortedRunEnd) {
            // clear all empty runs
            hdd.clearEmptySortedRuns();
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
    flushRAM();
    records.resize(capacity);

    for(int sortedRunIdx = sortedRunStIdx ; sortedRunIdx <= sortedRunEndIdx ; sortedRunIdx++) {
        loadBufferFromRun(sortedRunIdx, sortedRuns[sortedRunIdx], sortedRunStIdx); // Load the first `pageSize` rows
    }
    
    lastWinnerRunIdx = -1;
    mergingTree = std::make_unique<TreeOfLosers>(records, page_size, (sortedRunEndIdx - sortedRunStIdx + 1) * page_size, currentIndices, lastWinnerRunIdx);
    mergingTree->initializeTree();

    outputBufferStIdx = capacity - page_size;
    outputBufferIdx = outputBufferStIdx;
}

void DRAM::cleanupMerging(HDD& hdd) {
    // Write any remaining rows in the output buffer to HDD
    if (outputBufferIdx < capacity) {
        hdd.addBufferToMergedRun(std::vector<Row>(records.begin() + outputBufferStIdx, records.begin() + outputBufferIdx ));
    }

    hdd.appendMergedRunsToSortedRuns();
}

Row DRAM::getNextSortedRow(HDD& hdd, int sortedRunStIdx, int X) {

    TreeOfLosers& sortingTree = getMergingTree();
    Row nextRow = sortingTree.getNextRow();

    if(nextRow.offsetValue == INT_MAX) return nextRow;
    
    records[outputBufferIdx] = nextRow;
    outputBufferIdx += 1;

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

    if(outputBufferIdx >= capacity) {
        // flush output buffer to hdd
        hdd.addBufferToMergedRun(std::vector<Row>(records.begin() + outputBufferStIdx, records.begin() + capacity ));
        outputBufferIdx = outputBufferStIdx;
    }

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
