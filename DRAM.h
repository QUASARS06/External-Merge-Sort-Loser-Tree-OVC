#include <vector>
class Row;

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
    void mergeSortedRuns(std::vector<std::vector<Row> > sortedRuns);

private:
    std::vector<Row> records;
    int capacity; // represents the number of records that can be stored in RAM
    int page_size;

    std::vector<std::vector<Row> > convertToNestedVector();

}; // class DRAM
