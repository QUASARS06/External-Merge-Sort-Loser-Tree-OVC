#include "DRAM.h"
#include "Iterator.h"
#include "TreeOfLosers.h"
using namespace std;

DRAM::DRAM (int capacity, int page_size)
    : capacity (capacity), page_size (page_size)
{

}

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

std::vector<std::vector<Row> > DRAM::convertToNestedVector() {
    std::vector<std::vector<Row> > nestedRows;
    for(int i=0 ; i < records.size() ; i++) {
        nestedRows.push_back({records[i]});
    }
    return nestedRows;
}

void DRAM::sortRecords() {
    TreeOfLosers sortingTree(convertToNestedVector());
    sortingTree.initializeTree();

    flushRAM();

    Row nextRow;
    while ((nextRow = sortingTree.getNextRow()).offsetValue != INT_MAX) {
        // printf("Winner = [");
        // for(int i=0;i<nextRow.columns.size();i++) {
        //     printf("%d, ", nextRow.columns[i]);
        // }
        // printf("]\n");
        addRecord(nextRow);
    }
}

void DRAM::mergeSortedRuns(std::vector<std::vector<Row> > sortedRuns) {
    TreeOfLosers mergingTree(sortedRuns);
    mergingTree.initializeTree();

    flushRAM();

    Row nextRow;
    while ((nextRow = mergingTree.getNextRow()).offsetValue != INT_MAX) {
        // printf("Winner = [");
        // for(int i=0;i<nextRow.columns.size();i++) {
        //     printf("%d, ", nextRow.columns[i]);
        // }
        // printf("]\n");
        //addRecord(nextRow);
        records.push_back(nextRow);
    }
    printf("After Merge\n");
    printAllRecords();
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
