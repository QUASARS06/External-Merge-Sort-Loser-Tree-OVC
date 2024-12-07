#include "Scan.h"

# define NUM_OF_COLS 4
# define COL_VAL_DOMAIN 7 

// Scan Constructor
ScanPlan::ScanPlan(char const *const name, RowCount const count) : Plan(name), _count(count)
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
	
	std::vector<int> arr(NUM_OF_COLS);
	
    for (int i = 0 ; i < NUM_OF_COLS ; ++i) {
		int a = rand() % COL_VAL_DOMAIN;
        arr[i] = a;
    }

	row.columns = arr;
	row.offset = 0;
	row.offsetValue = arr[0];

	++_count;
	return true;
} // ScanIterator::next

void ScanIterator::free(Row &row)
{
	TRACE(false);
} // ScanIterator::free
