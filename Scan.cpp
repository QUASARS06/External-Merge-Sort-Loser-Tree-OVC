#include "Scan.h"

# define NUM_OF_COLS 4
# define COL_VAL_DOMAIN 5 

// Scan Constructor
ScanPlan::ScanPlan(char const *const name, RowCount const count) : Plan(name), _count(count)
{
	TRACE(true);
} // ScanPlan::ScanPlan

// Scan Destructor
ScanPlan::~ScanPlan()
{
	TRACE(true);
} // ScanPlan::~ScanPlan

Iterator *ScanPlan::init() const
{
	TRACE(true);
	return new ScanIterator(this);
} // ScanPlan::init

// ScanIterator Constructor
ScanIterator::ScanIterator(ScanPlan const *const plan) : _plan(plan), _count(0)
{
	TRACE(true);
} // ScanIterator::ScanIterator

// ScanIterator Destructor
ScanIterator::~ScanIterator()
{
	TRACE(true);
	printf("------------------------------------------------------------------------\n");
	traceprintf("produced %lu of %lu rows\n", (unsigned long)(_count), (unsigned long)(_plan->_count));
	printf("------------------------------------------------------------------------\n");
} // ScanIterator::~ScanIterator

bool ScanIterator::next(Row &row)
{
	TRACE(true);
	if (_count >= _plan->_count) return false;
	
	std::vector<int> arr(NUM_OF_COLS);
	
    for (int i = 0 ; i < NUM_OF_COLS ; ++i) {
		int a = rand() % COL_VAL_DOMAIN;
        arr[i] = a;
    }

	row.columns = arr;

	++_count;
	return true;
} // ScanIterator::next

void ScanIterator::free(Row &row)
{
	TRACE(true);
} // ScanIterator::free
