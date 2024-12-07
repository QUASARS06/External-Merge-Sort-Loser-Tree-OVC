#include <vector>
class Row;
class HDD;
class TreeOfLosers;

class DRAM
{
	
public:
	DRAM (int ram_capacity, int page_size);
	~DRAM ();

    bool addRecord(Row record);
    void printAllRecords();
    std::vector<Row> getAllRecords();
    void flushRAM();
    bool isFull();
    bool isEmpty();
    int getCapacity();

    void sortRecords();
    void mergeSortedRuns(HDD& hdd);
    void mergeRuns(HDD& hdd, int sortedRunStIdx, int sortedRunEndIdx, int X);
    void loadBufferFromRun(int runIndex, std::vector<Row>& run, int X);
    bool isExhausted(int lastWinnerRunIdx, std::vector<int> currentIndices);
    Row getNextSortedRow(HDD& hdd, int sortedRunStIdx, int X);
    void prepareMergingTree(std::vector<std::vector<Row> >& sortedRuns, int sortedRunStIdx, int sortedRunEndIdx, int X);
    void cleanupMerging(HDD& hdd);
    TreeOfLosers& getMergingTree();

    void sortInPlaceUsingSortIdx(std::vector<int> sortIdx);

    int outputBufferStIdx;
    int outputBufferIdx;
    int lastWinnerRunIdx;
    std::vector<int> currentIndices;
    //TreeOfLosers mergingTree;
    std::unique_ptr<TreeOfLosers> mergingTree;

    int pass;
    int ram_unsorted_ptr;

private:
    std::vector<Row> records;
    int capacity; // represents the number of records that can be stored in RAM
    int page_size;
    

}; // class DRAM
