#include "HDD.h"
#include "Iterator.h"
using namespace std;

HDD::HDD () {}

HDD::~HDD () {}


/**
 * Adds a sorted_run to all sorted_runs on Disk/HDD in sorted order
 * sorted_run is inserted in such a way that all sorted runs are sorted by their size (asc.)
 */
bool HDD::writeSortedRuns(std::vector<Row> sorted_run) {
    spill_count += sorted_run.size();

    int position = 0;
    for (size_t i = 0; i < sorted_runs.size(); i++) {
        if (sorted_runs[i].size() > sorted_run.size()) {
            break;
        }
        position++;
    }

    sorted_runs.insert(sorted_runs.begin() + position, sorted_run);
    return true;
}


void HDD::appendMergedRunsToSortedRuns() {
    writeSortedRuns(merged_run);

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
    sorted_runs.erase(std::remove_if(sorted_runs.begin(), sorted_runs.end(), isEmptyRun), sorted_runs.end());
}


void HDD::addOutputBufferToSingleSortedRun(std::vector<Row> outputBuffer) {
    single_sorted_run.insert(single_sorted_run.begin(), outputBuffer.begin(), outputBuffer.end());
}


void HDD::addSingleSortedRunToSortedRuns() {
    writeSortedRuns(single_sorted_run);
    single_sorted_run.clear();
}


std::vector<Row>& HDD::getSingleSortedRun() {
    return single_sorted_run;
}


void HDD::addSingleSortedRunCountToSpillCount() {
    spill_count += single_sorted_run.size();
}


int HDD::getSpillCount() {
    return spill_count;
}