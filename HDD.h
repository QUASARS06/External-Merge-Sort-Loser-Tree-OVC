#include <vector>
class Row;

class HDD
{
	
public:
	HDD ();
	~HDD ();

    bool writeSortedRuns(std::vector<Row> sorted_run);
    int getNumOfSortedRuns();
    std::vector<std::vector<Row> >& getSortedRuns();
    void clearEmptySortedRuns();

    void addBufferToMergedRun(std::vector<Row> mergedBuffer);
    void appendMergedRunsToSortedRuns();

    void addOutputBufferToSingleSortedRun(std::vector<Row> outputBuffer);
    void addSingleSortedRunToSortedRuns();
    std::vector<Row>& getSingleSortedRun();

    void addSingleSortedRunCountToSpillCount();
    int getSpillCount();

private:
    std::vector<std::vector<Row> > sorted_runs; // represents the sorted runs on Disk
    std::vector<Row> merged_run; // used when spilling Rows merged during merge steps
    std::vector<Row> single_sorted_run; // used when spilling Rows sorted during internal sort

    int spill_count;

}; // class DRAM
