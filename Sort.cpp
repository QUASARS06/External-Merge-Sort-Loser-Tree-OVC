#include "Sort.h"

SortPlan::SortPlan (char const * const name, int ram_capacity, int page_size, Plan * const input)
	: Plan (name), _input (input), ram_capacity (ram_capacity), page_size (page_size)
{
	TRACE (true);
} // SortPlan::SortPlan

SortPlan::~SortPlan ()
{
	TRACE (true);
	delete _input;
} // SortPlan::~SortPlan

Iterator * SortPlan::init () const
{
	TRACE (true);
	return new SortIterator (this);
} // SortPlan::init

SortIterator::SortIterator (SortPlan const * const plan) :
	_plan (plan), _input (plan->_input->init ()),
	_consumed (0), _produced (0)
{
	TRACE (true);
	dram = new DRAM(_plan->ram_capacity, _plan->page_size);
	hdd = new HDD();

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

	traceprintf ("%s consumed %lu rows\n",
			_plan->_name,
			(unsigned long) (_consumed));
	
	// printf("HDD\n");
	// hdd->printSortedRuns();

	dram->mergeSortedRuns(*hdd);
	hdd->printSortedRuns();

} // SortIterator::SortIterator

SortIterator::~SortIterator ()
{
	TRACE (false);

	printf("------------------------------------------------------------------------\n");
	traceprintf ("%s produced %lu of %lu rows\n",
			_plan->_name,
			(unsigned long) (_produced),
			(unsigned long) (_consumed));
	printf("------------------------------------------------------------------------\n");
} // SortIterator::~SortIterator

bool SortIterator::next (Row & row)
{
	TRACE (true);
	// hdd->printSortedRuns();
	// printf("Printing HDD\n");
	// hdd->printSortedRuns();
	//dram->mergeSortedRuns(hdd->getSortedRuns());
	
	// while(hdd->getNumOfSortedRuns() != 1) {

	// 	// pull from hdd and load into ram and merge and write back to hdd
	// 	// Use this formula to generate initial runs => ((W-2) % (F-1) + 2)

		
		
	// }

	// this means the number of sorted runs is less than the Fan-In
	// Hence now on each next() request we can pop the head of the tree of loser to return record

	// dram->popLoserTreeHead();

	if (_produced >= _consumed)  return false;

	++ _produced;
	return true;
} // SortIterator::next

void SortIterator::free (Row & row)
{
	TRACE (false);
} // SortIterator::free
