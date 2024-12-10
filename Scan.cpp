#include "Scan.h"

// Scan Constructor
ScanPlan::ScanPlan(char const *const name, RowCount const count, int num_of_cols, int col_val_domain, int scan_type) 
		: Plan(name), _count(count), num_of_cols(num_of_cols), col_val_domain(col_val_domain), scan_type(scan_type) 
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
	
    switch (_plan->scan_type)
    {
        case 1: // All records same
            if (_count == 0) {
                for (int i = 0; i < _plan->num_of_cols; ++i) {
                    arr[i] = rand() % _plan->col_val_domain;
                }
                fixed_record = arr; // Save for reuse
            } else {
                arr = fixed_record;
            }
            break;

        case 2: // All column values same
            {
                int val = rand() % _plan->col_val_domain;
                for (int i = 0; i < _plan->num_of_cols; ++i) {
                    arr[i] = val;
                }
            }
            break;

        case 3: // Records and column values same
            if (_count == 0) {
                int val = rand() % _plan->col_val_domain;
                arr.assign(_plan->num_of_cols, val); // All values in the record are `val`
                fixed_record = arr;          // Save for reuse
            } else {
                arr = fixed_record;
            }
            break;

        case 4: // Ascending records
            for (int i = 0; i < _plan->num_of_cols; ++i) {
                arr[i] = _count * _plan->col_val_domain / _plan->_count + i; // Scale by `_count`
            }
            break;

        case 5: // Descending records
            for (int i = 0; i < _plan->num_of_cols; ++i) {
                arr[i] = (_plan->_count - _count - 1) * _plan->col_val_domain / _plan->_count - i; // Reverse scale
            }
            break;

        case 6: // All zeroes
            arr.assign(_plan->num_of_cols, 0);
            break;
		
		case 7: // Random Negative records
            for (int i = 0; i < _plan->num_of_cols; ++i) {
                arr[i] = (rand() % _plan->col_val_domain) * -1;
            }
            break;
		
		default: // Random records
            for (int i = 0; i < _plan->num_of_cols; ++i) {
                arr[i] = rand() % _plan->col_val_domain;
            }
    }

	row.columns = arr;
	row.offset = 0;
	row.offsetValue = arr[0];

	printf("\nGenerated [%d, %d, %d, %d]\n", row.columns[0], row.columns[1], row.columns[2], row.columns[3]);

	++_count;
	return true;
} // ScanIterator::next

void ScanIterator::free(Row &row)
{
	TRACE(false);
} // ScanIterator::free
