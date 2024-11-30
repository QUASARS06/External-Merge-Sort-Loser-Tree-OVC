#include <vector>
class Row;
class HDD;

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
    int getCapacity();

    void sortRecords();
    void mergeSortedRuns(HDD& hdd);
    void mergeRuns(HDD& hdd, int sortedRunStIdx, int sortedRunEndIdx, int X);
    void loadBufferFromRun(int runIndex, std::vector<Row>& run, int X);
    bool isExhausted(int lastWinnerRunIdx, std::vector<int> currentIndices);

private:
    std::vector<Row> records;
    int capacity; // represents the number of records that can be stored in RAM
    int page_size;

}; // class DRAM
