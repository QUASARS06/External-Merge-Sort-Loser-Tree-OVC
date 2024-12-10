#include "HDD.h"
#include "Iterator.h"
using namespace std;

HDD::HDD ()
{

}

HDD::~HDD () {

}

bool HDD::writeSortedRuns(std::vector<Row> sorted_run) {
    // printf("Spilling - %lu records\n", sorted_run.size());
    int position = 0;
    for (size_t i = 0; i < sorted_runs.size(); i++) {
        if (sorted_runs[i].size() > sorted_runs.size()) {
            break; // Found the correct position
        }
        position++;
    }

    // Insert the new vector at the found position
    sorted_runs.insert(sorted_runs.begin() + position, sorted_run);


    // sorted_runs.push_back(sorted_run);
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
        printf("%d -> ", i);
        printArray1(records[i].columns);
        printf("  O = %d | OV = %d\n", records[i].offset, records[i].offsetValue);
    }
}

void HDD::printSortedRuns() {
    printf("\nPrinting Sorted Runs\n");
    for(int i=0;i<sorted_runs.size();i++) {
        printf("Run %d:\n", (i));
        printAllRecords1(sorted_runs[i]);
        printf("\n");
    }
}

void HDD::printSortedRunsSize() {
    printf("\nPrinting Sorted Runs Size\n");
    for(int i=0;i<sorted_runs.size();i++) {
        printf("Run %d  |  Size: %lu \n", (i), sorted_runs[i].size());
    }
}

void HDD::printMergedRuns() {
    printAllRecords1(merged_run);
}

void HDD::appendMergedRunsToSortedRuns() {
    sorted_runs.push_back(merged_run);

    // Clear mergedRun for future use
    merged_run.clear();
}

int HDD::getNumOfSortedRuns() {
    int ct = 0;
    for(int i=0;i<sorted_runs.size();i++) {
        if(!sorted_runs[i].empty()) ct++;
    }
    return ct;
}

std::vector<std::vector<Row> >& HDD::getSortedRuns() {
    return sorted_runs;
}

void HDD::addBufferToMergedRun(std::vector<Row> mergedBuffer) {
    merged_run.insert(merged_run.end(), mergedBuffer.begin(), mergedBuffer.end());
}

bool isEmptyRun(const std::vector<Row>& run) {
    return run.empty();
}

void HDD::clearEmptySortedRuns() {
    sorted_runs.erase(std::remove_if(sorted_runs.begin(), sorted_runs.end(), isEmptyRun),
                                    sorted_runs.end());
}

// void HDD::moveSmallerRunToStart() {
//     if (sorted_runs.size() < 2) return;

//     size_t firstSize = sorted_runs[0].size();
//     size_t lastSize = sorted_runs[sorted_runs.size() - 1].size();

//     if (lastSize < firstSize) {
//         vector<Row> lastVector = sorted_runs[sorted_runs.size() - 1];
//         sorted_runs.pop_back();
//         sorted_runs.insert(sorted_runs.begin(), lastVector);
//     }
// }

void HDD::addOutputBufferToSingleSortedRun(std::vector<Row> outputBuffer) {
    single_sorted_run.insert(single_sorted_run.begin(), outputBuffer.begin(), outputBuffer.end());
}

void HDD::addSingleSortedRunToSortedRuns() {
    writeSortedRuns(single_sorted_run);
    single_sorted_run.clear();
}

void HDD::printSingleSortedRun() {
    printf("Single Sorted Run:\n");
    printAllRecords1(single_sorted_run);
}

std::vector<Row>& HDD::getSingleSortedRun() {
    return single_sorted_run;
}

bool HDD::isSingleSortedRunEmpty() {
    return single_sorted_run.empty();
}