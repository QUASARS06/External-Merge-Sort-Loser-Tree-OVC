#include "Sort.h"

SortPlan::SortPlan (char const * const name, int ram_capacity, Plan * const input)
	: Plan (name), _input (input), ram_capacity (ram_capacity)
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
	dram = new DRAM(_plan->ram_capacity);
	hdd = new HDD();

	for (Row row;  _input->next (row);  _input->free (row)) {
		
		dram->addRecord(row);

		if(dram->isFull()) {
			
			dram->sortRecords();
			dram->computeOVC();
			hdd->writeSortedRuns(dram->getAllRecords());
			dram->flushRAM();

		}

		++ _consumed;
	}

	delete _input;

	traceprintf ("%s consumed %lu rows\n",
			_plan->_name,
			(unsigned long) (_consumed));
} // SortIterator::SortIterator

SortIterator::~SortIterator ()
{
	TRACE (true);

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
	// printf("BEFORE: \n");
	// dram->printAllRecords();
	// dram->sortRecords();

	// printf("AFTER: \n");
	// dram->printAllRecords();
	//hdd->printSortedRuns();

	if (_produced >= _consumed)  return false;

	++ _produced;
	return true;
} // SortIterator::next

void SortIterator::free (Row & row)
{
	TRACE (true);
} // SortIterator::free
