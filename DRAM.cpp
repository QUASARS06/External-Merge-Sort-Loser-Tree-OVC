#include "DRAM.h"
#include "HDD.h"
#include "Iterator.h"
#include "TreeOfLosers.h"
using namespace std;

DRAM::DRAM (int capacity, int page_size) : capacity (capacity), page_size (page_size) {}

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

void DRAM::sortRecords() {
    std::vector<int> currentIndices;           // Current index in each run
    int lastWinnerRunIdx = -1;

    TreeOfLosers sortingTree(records, 1, records.size(), currentIndices, lastWinnerRunIdx);
    sortingTree.initializeTree();
    

    std::vector<Row> temp_buffer;

    Row nextRow;
    while ((nextRow = sortingTree.getNextRow()).offsetValue != INT_MAX) {
        temp_buffer.push_back(nextRow);
        //printf("lastWinnerRunIdx = %d\n", lastWinnerRunIdx);
    }

    flushRAM();
    records = temp_buffer;
}

void DRAM::mergeSortedRuns(HDD& hdd) {
    flushRAM();

    int totalBuffers = capacity / page_size;
    int B = totalBuffers - 1;
    int W = hdd.getNumOfSortedRuns();

    int X = (W-2) % (B-1) + 2;

    int mergingRunSt = 0;
    int mergingRunEnd = X-1;
    
    int pass = 0;
    printf("--------------------------------------------- PASS %d ---------------------------------------------\n", pass);

    int sortedRunEnd = hdd.getNumOfSortedRuns();
    while(hdd.getNumOfSortedRuns() > B) {
        // load runs mergingRunSt -> mergingRunEnd into DRAM and merge
        mergeRuns(hdd, mergingRunSt, mergingRunEnd, X);
        mergingRunSt = mergingRunEnd + 1;
        mergingRunEnd = std::min(mergingRunSt + B, (int)W) - 1;

        if(mergingRunSt >= sortedRunEnd) {
            pass++;
            printf("--------------------------------------------- PASS %d ---------------------------------------------\n", pass);
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

void DRAM::mergeRuns(HDD& hdd, int sortedRunStIdx, int sortedRunEndIdx, int X) {
    int totalBuffers = capacity / page_size;
    int B = totalBuffers - 1; // Reserve 1 buffer for output

    std::vector<std::vector<Row> >& sortedRuns = hdd.getSortedRuns();

    flushRAM();
    records.resize(capacity);

    for(int sortedRunIdx = sortedRunStIdx ; sortedRunIdx <= sortedRunEndIdx ; sortedRunIdx++) {
        loadBufferFromRun(sortedRunIdx, sortedRuns[sortedRunIdx], X); // Load the first `pageSize` rows
    }

    // printf("RAM After Loading!!\n");
    // printAllRecords();

    // printf("\nsortedRunStIdx = %d | sortedRunEndIdx = %d\n\n", sortedRunStIdx, sortedRunEndIdx);

    // printf("\nNew HDD\n");
    // hdd.printSortedRuns();

    // Initialize the TreeOfLosers
    std::vector<int> currentIndices;           // Current index in each run
    int lastWinnerRunIdx = -1;
    TreeOfLosers sortingTree(records, page_size, (sortedRunEndIdx - sortedRunStIdx + 1) * page_size, currentIndices, lastWinnerRunIdx);
    sortingTree.initializeTree();

    int outputBufferStIdx = capacity - page_size;
    int outputBufferIdx = outputBufferStIdx;

    // Merge rows using TreeOfLosers
    Row nextRow;
    while ((nextRow = sortingTree.getNextRow()).offsetValue != INT_MAX) {
        records[outputBufferIdx] = nextRow;
        outputBufferIdx += 1;

        // printf("lastWinnerRunIdx = %d\n", lastWinnerRunIdx);
        // printArray(nextRow.columns, nextRow.offset, nextRow.offsetValue);
        // printf("Current Indices = [");
        // for(int i=0;i<currentIndices.size();i++) {
        //     printf("%d, ", currentIndices[i]);
        // }
        // printf("]\n\n");

        // check if lastWinnerRunIdx is at end of it's range then reload
        int endIdxForRun = (lastWinnerRunIdx+1) * page_size - 1;

        if(currentIndices[lastWinnerRunIdx] >= endIdxForRun) {
            //printf("Run %d is going to exhaust\n", lastWinnerRunIdx);
            
            int sortedRunIdx = sortedRunStIdx + lastWinnerRunIdx;
            
            std::vector<Row>& run = sortedRuns[sortedRunIdx];
            run.insert(run.begin(), records[endIdxForRun]);
            
            loadBufferFromRun(sortedRunIdx, run, X); // Load the first `pageSize` rows
            // Actual runIdx in sortedRuns = sortedRunStIdx + lastWinnerRunIdx

            // printf("\n\nRAM after reloading\n");
            // printAllRecords();
            // printf("\nRAM after reloading\n");
            currentIndices[lastWinnerRunIdx] = lastWinnerRunIdx * page_size;
        }

        if(outputBufferIdx >= capacity) {
            //printf("Offloading\n\n");
            //printAllRecords();
            // flush output buffer to hdd
            hdd.addBufferToMergedRun(std::vector<Row>(records.begin() + outputBufferStIdx, records.begin() + capacity ));
            //std::vector<Row> slice(records.begin() + outputBufferStIdx, records.begin() + capacity );

            outputBufferIdx = outputBufferStIdx;
        }

        // Reload buffers if any input buffer is exhausted
        // for (int i = 0; i < runsToMerge.size(); ++i) {
        //     if (isBufferExhausted(i)) {
        //         if (!loadBufferFromRun(i, runsToMerge[i], pageSize)) {
        //             loadDummyRow(i); // Load dummy if the run is exhausted
        //         }
        //     }
        // }
    }

    // printf("DONE-----------------\n");
    //hdd.printMergedRuns();
    hdd.appendMergedRunsToSortedRuns();



    // Write any remaining rows in the output buffer to HDD
    // if (outputBufferIdx < capacity) {
    //     hdd.addBufferToMergedRun(std::vector<Row>(records.begin() + outputBufferStIdx, records.begin() + outputBufferStIdx + outputBufferIdx ));
    // }
}

void DRAM::loadBufferFromRun(int runIndex, std::vector<Row>& run, int X) {
    // Calculate start and end indices for the next chunk
    int startIdxInDram = (runIndex < X ? runIndex : (runIndex - X)) * page_size;

    for(int i=0;i<page_size;i++) {
        if(run.size() > 0) {
            records[startIdxInDram + i] = run.front();
            run.erase(run.begin());
        } else {
            records[startIdxInDram + i] = Row({std::numeric_limits<int>::max(), std::numeric_limits<int>::max()},
                        std::numeric_limits<int>::min(),
                        std::numeric_limits<int>::max());
        }
    }
}

// bool DRAM::isExhausted(int lastWinnerRunIdx, std::vector<int> currentIndices) {
//     return 
// }

std::vector<Row> DRAM::getAllRecords() {
    return records;
}

void DRAM::flushRAM() {
    records.clear();
}

bool DRAM::isFull() {
    return records.size() >= capacity;
}

int DRAM::getCapacity() {
    return capacity;
}
