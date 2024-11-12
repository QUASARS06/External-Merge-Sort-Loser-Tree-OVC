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

bool DRAM::isFull() {
    return records.size() >= capacity;
}

void DRAM::computeOVC() {

    Row prevRow;
    int offset;
    int offsetValue;

    for(int i=0 ; i < records.size() ; i++) {
        Row& currRow = records[i];
        offset = -1;
        offsetValue = 0;

        if(i == 0) {
            offset = currRow.columns.size() - 1;
            offsetValue = currRow.columns[0];
        } else {
            for (int j = 0; j < currRow.columns.size(); j++) {
                if (currRow.columns[j] != prevRow.columns[j]) {
                    offset = j;
                    offsetValue = currRow.columns[j];
                    break;
                }
            }
        }
        
        currRow.offset = offset;
        currRow.offsetValue = offsetValue;

        prevRow = currRow;
    }

}
