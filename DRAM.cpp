#include "DRAM.h"
#include "InternalQuickSort.h"
#include "Iterator.h"
using namespace std;

DRAM::DRAM (int capacity)
    : capacity (capacity)
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

void printArray(std::vector<int> arr) {
    printf("[");
    for(int i=0 ; i < arr.size() ; i++) {
        printf("%d", arr[i]);
        if(i < arr.size()-1) printf(", ");
    }
    printf("]\n");
}

void DRAM::printAllRecords() {
    for(int i=0 ; i < records.size() ; i++) {
        printArray(records[i].columns);
    }
}

void DRAM::sortRecords() {
    quickSort(records, 0, records.size() -1);
}

std::vector<Row> DRAM::getAllRecords() {
    return records;
}

void DRAM::flushRAM() {
    records.clear();
}
