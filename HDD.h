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

private:
    std::vector<std::vector<Row> > sorted_runs;
    std::vector<Row> merged_run;

}; // class DRAM
