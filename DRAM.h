#include <vector>
class Row;

class DRAM
{
	
public:
	DRAM (int ram_capacity);
	~DRAM ();

    bool addRecord(Row record);
    void printAllRecords();
    void sortRecords();
    std::vector<Row> getAllRecords();
    void flushRAM();
    bool isFull();
    void computeOVC();

private:
    std::vector<Row> records;
    int capacity; // represents the number of records that can be stored in RAM

}; // class DRAM
