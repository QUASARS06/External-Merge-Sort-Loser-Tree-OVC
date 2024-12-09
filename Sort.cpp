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

	// reserving 1 page for output buffer in the ram
	int actual_ram_capacity = _plan->ram_capacity - _plan->page_size;
	// int actual_ram_capacity = _plan->ram_capacity;

	dram = new DRAM(actual_ram_capacity, _plan->page_size);
	hdd = new HDD();

	for (Row row;  _input->next (row);  _input->free (row)) {
		
		// printf("\nAdding record '%llu' [%d, %d, %d, %d]\n", (_consumed + 1), row.columns[0], row.columns[1], row.columns[2], row.columns[3]);
		dram->addRecord(row, *hdd);
		// dram->printAllRecords();
		// printf("\n\n");

		// if(dram->isFull()) {
		// 	// printf("\nBefore Sorting:\n");
		// 	// dram->printAllRecords();

		// 	dram->sortRecords();

		// 	// printf("\nAfter Sorting:\n");
		// 	// dram->printAllRecords();

		// 	hdd->writeSortedRuns(dram->getAllRecords());
		// 	dram->flushRAM();
		// }

		++ _consumed;
	}
	dram->sortPartiallyFilledRam(*hdd);

	// hdd->printSortedRuns();

	// dram->printAllRecords();

	// hdd->printSingleSortedRun();

	// dram->printOutputBuffer();

	// dram->printOutputBuffer();

	// if ram is not full but there are still records in it then we need to sort and write them to hdd
	// if(!dram->isEmpty()) {
	// 	// printf("\nBefore Sorting:\n");
	// 	// dram->printAllRecords();

	// 	dram->sortRecords();

	// 	// printf("\nAfter Sorting:\n");
	// 	// dram->printAllRecords();

	// 	hdd->writeSortedRuns(dram->getAllRecords());
	// 	dram->flushRAM();
	// }

	delete _input;

	printf("\n");
	traceprintf ("%s consumed %lu rows\n", _plan->_name, (unsigned long) (_consumed));


    int expectedW = std::ceil(_consumed*1.0 / (_plan->ram_capacity - _plan->page_size));

	int totalBuffers = _plan->ram_capacity / _plan->page_size;
    int B = totalBuffers - 1;
    int W = hdd->getNumOfSortedRuns();

	int X = (W-2) % (B-1) + 2;

	int mergeDepth = 1 + std::ceil(std::log(W) / std::log(B));

	printf("\n-----------------------------------------\n");
    printf("RAM Parameters:\n");
    printf("RAM Capacity - %d\n", _plan->ram_capacity);
    printf("Page Size - %d\n", _plan->page_size);
    printf("Total Buffers - %d\n", totalBuffers);
    printf("B - %d (1 output buffer)\n", B);
    printf("Actual Number of Sorted Runs (W) - %d\n", W);
    printf("Initial Merge Fan-In (X) - %d\n", X);

	printf("\nCalculations:\n");
	printf("Expected Merge Depth (# of passes) - %d\n", mergeDepth);
	printf("Expected Number of Sorted Runs (W) - %d\n", expectedW);
    printf("-----------------------------------------\n\n");

	// printing this after in-memory sorting is actually complete (only so it looks good in output)
	// but for other passes it's printed at the start of the merge pass
	printf("------------------------- Pass 0 : Sorting -------------------------\n");


	// -------- Starting Merging --------

	// In the case where the last run doesn't completely occupy memory it will be smaller than rest of runs
	// Since we first only merge (W-2) % (B-1) + 2 runs to minimize I/O we need to merge smaller runs
	// So from an implementation perspective I'll just move the smallest run at the start of the sorted runs
	// which will make sure we are minimizing the I/O ( as per the volcano paper )
	hdd->moveSmallerRunToStart();

	// this will merge all sorted runs until the number of sorted runs is less than equal to merging buffers
	dram->mergeSortedRuns(*hdd);

	// this will prepare the loser tree for the final merge step
	printf("------------------------- Pass %d : Merging -------------------------\n", dram->pass);

	int mergingRunSt = 0;
    int mergingRunEnd = std::min(mergingRunSt + B, (int)W) - 1;

	// prepare tree for final merging
	dram->prepareMergingTree(hdd->getSortedRuns(), mergingRunSt, mergingRunEnd, B);


} // SortIterator::SortIterator

SortIterator::~SortIterator ()
{
	TRACE (false);

	printf("\n----------------------------------------------------------------------------------\n");
	traceprintf ("%s produced %lu of %lu rows\n",
			_plan->_name,
			(unsigned long) (_produced),
			(unsigned long) (_consumed));
	printf("----------------------------------------------------------------------------------\n");
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
