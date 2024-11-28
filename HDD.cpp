#include "HDD.h"
#include "Iterator.h"
using namespace std;

HDD::HDD ()
{

}

HDD::~HDD () {

}

bool HDD::writeSortedRuns(std::vector<Row> sorted_run) {
    sorted_runs.push_back(sorted_run);
    return true;
}

void printArray1(std::vector<int> arr) {
    printf("[");
    for(int i=0 ; i < arr.size() ; i++) {
        printf("%d", arr[i]);
        if(i < arr.size()-1) printf(", ");
    }
    printf("]");
}

void printAllRecords1(std::vector<Row> records) {
    for(int i=0 ; i < records.size() ; i++) {
        printArray1(records[i].columns);
        printf("  O = %d | OV = %d\n", records[i].offset, records[i].offsetValue);
    }
}

void HDD::printSortedRuns() {
    for(int i=0;i<sorted_runs.size();i++) {
        printf("Run %d:\n", (i+1));
        printAllRecords1(sorted_runs[i]);
        printf("\n");
    }
}

int HDD::getNumOfSortedRuns() {
    return sorted_runs.size();
}

std::vector<std::vector<Row> > HDD::getSortedRuns() {
    return sorted_runs;
}
