#include <vector>
class Row;

class HDD
{
	
public:
	HDD ();
	~HDD ();

    bool writeSortedRuns(std::vector<Row> sorted_run);
    void printSortedRuns();

private:
    std::vector<std::vector<Row> > sorted_runs;

}; // class DRAM
