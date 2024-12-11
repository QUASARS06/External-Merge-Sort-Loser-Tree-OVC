#include <vector>
class Row;
class HDD;
class TreeOfLosers;

class DRAM
{
	
public:
	DRAM (int ram_capacity, int page_size);
	~DRAM ();

    bool addRecord(Row record, HDD& hdd);
    void sortPartiallyFilledRam(HDD& hdd);
    
    void sortInPlaceUsingSortIdx(std::vector<int> sortIdx, int N);
    void sortRecords(int sortingSize);

    void mergeSortedRuns(HDD& hdd);
    void mergeRuns(HDD& hdd, int sortedRunStIdx, int sortedRunEndIdx);

    void loadBufferFromRun(int runIndex, std::vector<Row>& run, int X);

    void prepareMergingTree(std::vector<std::vector<Row> >& sortedRuns, int sortedRunStIdx, int sortedRunEndIdx);
    void cleanupMerging(HDD& hdd);
    TreeOfLosers& getMergingTree();

    Row getNextSortedRow(HDD& hdd, int sortedRunStIdx);

    Row getSortedRowFromRAM();
    Row getRowFromSingleSortedRunOnHDD(HDD& hdd);

    void flushRAM();

    int pass;

private:
    std::vector<Row> records;   // actual RAM
    std::vector<Row> output_buffer;
    std::unique_ptr<TreeOfLosers> mergingTree;
    int ram_unsorted_ptr;
    int capacity; // represents the number of records that can be stored in RAM
    int page_size;
    int lastWinnerRunIdx;
    std::vector<int> currentIndices;
    

}; // class DRAM
