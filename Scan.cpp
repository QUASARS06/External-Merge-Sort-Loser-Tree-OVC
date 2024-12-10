#include "Scan.h"

// Scan Constructor
ScanPlan::ScanPlan(char const *const name, RowCount const count, int num_of_cols, int col_val_domain) 
		: Plan(name), _count(count), num_of_cols(num_of_cols), col_val_domain(col_val_domain)
{
	TRACE(false);
} // ScanPlan::ScanPlan

// Scan Destructor
ScanPlan::~ScanPlan()
{
	TRACE(false);
} // ScanPlan::~ScanPlan

Iterator *ScanPlan::init() const
{
	TRACE(false);
	return new ScanIterator(this);
} // ScanPlan::init

// ScanIterator Constructor
ScanIterator::ScanIterator(ScanPlan const *const plan) : _plan(plan), _count(0)
{
	TRACE(false);
} // ScanIterator::ScanIterator

// ScanIterator Destructor
ScanIterator::~ScanIterator()
{
	TRACE(false);
	printf("\n------------------------------------------------------------------------\n");
	traceprintf("produced %lu of %lu rows\n", (unsigned long)(_count), (unsigned long)(_plan->_count));
	printf("------------------------------------------------------------------------\n");
} // ScanIterator::~ScanIterator

bool ScanIterator::next(Row &row)
{
	TRACE(false);
	if (_count >= _plan->_count) return false;
	
	std::vector<int> arr(_plan->num_of_cols);
	
    for (int i = 0 ; i < _plan->num_of_cols ; ++i) {
		int a = rand() % _plan->col_val_domain;
        arr[i] = a;
    }

	row.columns = arr;
	row.offset = 0;
	row.offsetValue = arr[0];

	// printf("\nGenerated [%d, %d, %d, %d]\n", row.columns[0], row.columns[1], row.columns[2], row.columns[3]);

	++_count;
	return true;
} // ScanIterator::next

void ScanIterator::free(Row &row)
{
	TRACE(false);
} // ScanIterator::free
