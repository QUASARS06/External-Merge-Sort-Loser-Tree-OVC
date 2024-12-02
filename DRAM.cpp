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
    pass = 1;

    int totalBuffers = capacity / page_size;
    int B = totalBuffers - 1;
    int W = hdd.getNumOfSortedRuns();

    int X = (W-2) % (B-1) + 2;

    int mergingRunSt = 0;
    int mergingRunEnd = X-1;

    int sortedRunEnd = hdd.getNumOfSortedRuns();

    // hdd.printSortedRuns();
    while(hdd.getNumOfSortedRuns() > B) {
        // load runs mergingRunSt -> mergingRunEnd into DRAM and merge
        mergeRuns(hdd, mergingRunSt, mergingRunEnd, X);
        // hdd.printSortedRuns();
        mergingRunSt = mergingRunEnd + 1;
        mergingRunEnd = std::min(mergingRunSt + B, (int)W) - 1;

        if(mergingRunSt >= sortedRunEnd) {
            // clear all empty runs
            hdd.clearEmptySortedRuns();
            mergingRunSt = 0;
            mergingRunEnd = std::min(mergingRunSt + B, (int)W) - 1;
            X = B;

            sortedRunEnd = hdd.getNumOfSortedRuns();
            // printf("HERE\n");
        }
    }
    hdd.clearEmptySortedRuns();
}

TreeOfLosers& DRAM::getMergingTree() {
    return *mergingTree;
}

void DRAM::prepareMergingTree(std::vector<std::vector<Row> >& sortedRuns, int sortedRunStIdx, int sortedRunEndIdx, int X) {

    flushRAM();
    records.resize(capacity);

    // printf("sortedRunStIdx - %d | sortedRunEndIdx - %d | page_size - %d | lastWinnerRunIdx - %d\n", sortedRunStIdx, sortedRunEndIdx, page_size, lastWinnerRunIdx);

    for(int sortedRunIdx = sortedRunStIdx ; sortedRunIdx <= sortedRunEndIdx ; sortedRunIdx++) {
        // loadBufferFromRun(sortedRunIdx, sortedRuns[sortedRunIdx], X); // Load the first `pageSize` rows
        loadBufferFromRun(sortedRunIdx, sortedRuns[sortedRunIdx], sortedRunStIdx); // Load the first `pageSize` rows
    }

    // printf("After Loading Ram\n");
    // printAllRecords();
    
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

        std::vector<std::vector<Row> >& sortedRuns = hdd.getSortedRuns();
        
        std::vector<Row>& run = sortedRuns[sortedRunIdx];
        run.insert(run.begin(), records[endIdxForRun]);
        
        // loadBufferFromRun(sortedRunIdx, run, X); // Load the first `pageSize` rows
        loadBufferFromRun(sortedRunIdx, run, sortedRunStIdx); // Load the first `pageSize` rows
        // Actual runIdx in sortedRuns = sortedRunStIdx + lastWinnerRunIdx
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

int DRAM::getCapacity() {
    return capacity;
}
