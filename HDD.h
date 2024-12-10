#include <vector>
class Row;

class HDD
{
	
public:
	HDD ();
	~HDD ();

    bool writeSortedRuns(std::vector<Row> sorted_run);
    void printSortedRuns();
    int getNumOfSortedRuns();
    std::vector<std::vector<Row> >& getSortedRuns();
    void addBufferToMergedRun(std::vector<Row> mergedBuffer);
    void printMergedRuns();
    void appendMergedRunsToSortedRuns();
    void clearEmptySortedRuns();
    void moveSmallerRunToStart();

    void addOutputBufferToSingleSortedRun(std::vector<Row> outputBuffer);
    void addSingleSortedRunToSortedRuns();
    void printSingleSortedRun();

    std::vector<Row>& getSingleSortedRun();
    bool isSingleSortedRunEmpty();

    void printSortedRunsSize();

private:
    std::vector<std::vector<Row> > sorted_runs;
    std::vector<Row> merged_run;

    std::vector<Row> single_sorted_run;

}; // class DRAM
