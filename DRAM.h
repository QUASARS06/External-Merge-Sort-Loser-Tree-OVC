#include <vector>
class Row;

class DRAM
{
	
public:
	DRAM ();
	~DRAM ();

    bool addRecord(Row record);
    void printAllRecords();
    void sortRecords();

private:
    std::vector<Row> records;

}; // class DRAM
