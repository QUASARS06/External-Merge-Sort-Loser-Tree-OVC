#include <vector>
class Row;

bool compareRows(const Row& row1, const Row& row2);

int partition(std::vector<Row>& records, int low, int high);

void quickSort(std::vector<Row>& records, int low, int high);
