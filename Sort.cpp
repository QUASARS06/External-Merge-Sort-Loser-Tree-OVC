#include "Sort.h"

SortPlan::SortPlan (char const * const name, int ram_capacity, int page_size, Plan * const input)
	: Plan (name), _input (input), ram_capacity (ram_capacity), page_size (page_size)
{
	TRACE (false);
} // SortPlan::SortPlan

SortPlan::~SortPlan ()
{
	TRACE (false);
	delete _input;
} // SortPlan::~SortPlan

Iterator * SortPlan::init () const
{
	TRACE (false);
	return new SortIterator (this);
} // SortPlan::init

SortIterator::SortIterator (SortPlan const * const plan) :
	_plan (plan), _input (plan->_input->init ()),
	_consumed (0), _produced (0)
{
	TRACE (false);
	dram = new DRAM(_plan->ram_capacity, _plan->page_size);
	hdd = new HDD();

	printf("------------------------- Pass 0 : Sorting -------------------------\n");
	for (Row row;  _input->next (row);  _input->free (row)) {
		
		dram->addRecord(row);

		if(dram->isFull()) {
			
			//printf("Before Sort\n");
			//dram->printAllRecords();
			dram->sortRecords();

			//printf("After Sort\n");
			//dram->printAllRecords();
			hdd->writeSortedRuns(dram->getAllRecords());
			dram->flushRAM();

		}

		++ _consumed;
	}

	delete _input;

	traceprintf ("%s consumed %lu rows\n\n",
			_plan->_name,
			(unsigned long) (_consumed));
	
	// printf("HDD\n");
	// hdd->printSortedRuns();

	// this will merge all sorted runs until the number of sorted runs is less than equal to merging buffers
	dram->mergeSortedRuns(*hdd);
	// hdd->printSortedRuns();
	// printf("HERE\n");

	printf("------------------------- Pass %d : Merging -------------------------\n", dram->pass);
	int totalBuffers = _plan->ram_capacity / _plan->page_size;
    int B = totalBuffers - 1;
    int W = hdd->getNumOfSortedRuns();

	int mergingRunSt = 0;
    int mergingRunEnd = std::min(mergingRunSt + B, (int)W) - 1;

	// prepare tree for final merging
	dram->prepareMergingTree(hdd->getSortedRuns(), mergingRunSt, mergingRunEnd, B);

} // SortIterator::SortIterator

SortIterator::~SortIterator ()
{
	TRACE (false);

	printf("\n------------------------------------------------------------------------\n");
	traceprintf ("%s produced %lu of %lu rows\n",
			_plan->_name,
			(unsigned long) (_produced),
			(unsigned long) (_consumed));
	printf("------------------------------------------------------------------------\n\n");
} // SortIterator::~SortIterator

bool SortIterator::next (Row & row)
{
	TRACE (false);

	int totalBuffers = _plan->ram_capacity / _plan->page_size;
    int B = totalBuffers - 1;
	// Merge rows using TreeOfLosers
    row = dram->getNextSortedRow(*hdd, 0, B);

	if(row.offsetValue == INT_MAX) {
		dram->cleanupMerging(*hdd);
		return false;
	}

	// printf("[");
    // for(int i=0 ; i < row.columns.size() ; i++) {
    //     printf("%d", row.columns[i]);
    //     if(i < row.columns.size()-1) printf(", ");
    // }
    // // printf("]  |  Offset = %d  |  Offset Value = %d\n", row.offset, row.offsetValue);
	// printf("]\n");

	if (_produced >= _consumed)  return false;

	++ _produced;
	return true;
} // SortIterator::next

void SortIterator::free (Row & row)
{
	TRACE (false);
} // SortIterator::free
