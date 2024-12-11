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
	_consumed (0), _produced (0), isSingleSortedRun(false)
{
	TRACE (false);

	// reserving 1 page for output buffer in the ram
	int actual_ram_capacity = _plan->ram_capacity - _plan->page_size;

	dram = new DRAM(actual_ram_capacity, _plan->page_size);
	hdd = new HDD();

	for (Row row;  _input->next (row);  _input->free (row)) {

		// Adds record to the RAM
		// When RAM is full performs Internal Sort
		// Gracefully spills records to Disk (HDD) only as much as necessary using output buffer
		dram->addRecord(row, *hdd);
		++ _consumed;
	}

	delete _input;

	printf("\n");
	traceprintf ("%s consumed %lu rows\n", _plan->_name, (unsigned long) (_consumed));

	// if we encountered 0 records do nothing
	if(_consumed <= 0) return;

	// if the number of records consumed is less than or equal to the RAM Capacity
	// then simply sort the records in memory
	else if(_consumed <= actual_ram_capacity) {
		dram->sortRecords(_consumed);
		printf("\n----------------------------------------------------------------------------\n");
		printf("HDD Total Spill after Sorting - %d (0 since ram can accomodate whole input)\n", hdd->getSpillCount());
		printf("----------------------------------------------------------------------------\n");

		// we should return from here but we need to print some descriptive statements
		// hence we return after that below
	} 
	else {
		// after adding records to RAM some records may still be present in the RAM which are unsorted
		// (usually for the last few records) we need to sort these records

		// this is because sorting is triggered in the addRecord(...) function only when RAM is full
		// below function handles the scenario when scan stops generating record even before RAM is full
		dram->sortPartiallyFilledRam(*hdd);
	}

	// Calculations based on the parameters we have

	// Expected Number of Sorted Runs (W)
	int expectedW = _consumed <= actual_ram_capacity ? 1 : (int)(_consumed*1.0 / (_plan->ram_capacity - _plan->page_size));

	// Total Page Sized Buffers in the RAM (including the Output Buffer)
	int totalBuffers = _plan->ram_capacity / _plan->page_size;

	// Fan-In (B) is one less than totalBuffers as 1 buffer is reserved for Output
    int B = totalBuffers - 1;

	// Actual Number of Sorted runs generated after Internal Sorting
    int W = _consumed <= actual_ram_capacity ? 1 : hdd->getNumOfSortedRuns();

	// Initial Merge Fan-In to ensure subsequent merge steps have Fan-In of B
	int X = (W-2) % (B-1) + 2;

	// Merge Depth (... W = I/M) additional 1 is for the initial sorting phase
	int mergeDepth = 1 + std::ceil(std::log(W) / std::log(B));

	printf("\n-----------------------------------------\n");
    printf("RAM Parameters:\n");
    printf("Total Buffers - %d\n", totalBuffers);
    printf("B - %d (1 output buffer)\n", B);
    printf("Actual Number of Sorted Runs (W) - %d\n", W);
    printf("Initial Merge Fan-In (X) - %d\n", X);

	printf("\nCalculations:\n");
	printf("Expected Merge Depth (# of passes) - %d\n", mergeDepth);
	printf("Expected Number of Sorted Runs (W) - %d\n", expectedW);
    printf("-----------------------------------------\n\n");

	// printing this after in-memory sorting is actually complete (only so it looks good in output)
	// but for other passes the Pass # is printed at the start of the merge pass
	printf("------------------------- Pass 0 : Sorting -------------------------\n");

	// if the number of consumed records fit in memory no need for merge step
	if(_consumed <= actual_ram_capacity) return;

	// if the number of sorted runs are just 1 no need for merge step
	else if(hdd->getNumOfSortedRuns() == 1) {
		isSingleSortedRun = true;
		return;
	}

	// -------- Starting Merging --------

	// When sorted runs are spilled to HDD, the HDD ensures that they are inserted in sorted fashion by size
	// Hence we will always merge smaller runs before to make sure we are minimizing the I/O

	// this will merge all sorted runs until the number of sorted runs is less than equal to merging buffers
	// we stop when number of sorted runs <= B because the final merge step needs to happen on demand when next() is called
	dram->mergeSortedRuns(*hdd);


	// this will prepare the loser tree for the final merge step
	// on every next() invocation of the SortIterator a sorted Row will be returned and loser tree will be updated
	// thus the Final sorted output is generated on demand
	printf("------------------------- Pass %d : Merging (%d sorted runs) -------------------------\n", dram->pass, hdd->getNumOfSortedRuns());

	int mergingRunSt = 0;
    // int mergingRunEnd = std::min(mergingRunSt + B, (int)W) - 1;
	int mergingRunEnd = std::min(B, (int)hdd->getNumOfSortedRuns()) - 1;

	// prepare tree for final merging
	dram->prepareMergingTree(hdd->getSortedRuns(), mergingRunSt, mergingRunEnd, B);

} // SortIterator::SortIterator

SortIterator::~SortIterator ()
{
	TRACE (false);

	printf("\n----------------------------------------------------------------------------------\n");
	traceprintf ("%s produced %lu of %lu rows\n",
			_plan->_name, (unsigned long) (_produced), (unsigned long) (_consumed));
	printf("----------------------------------------------------------------------------------\n");
} // SortIterator::~SortIterator

bool SortIterator::next (Row & row)
{
	TRACE (false);

	// if we encountered 0 records do nothing
	if(_consumed <= 0) {
		printf("\n----------------------------------\n");
		printf("HDD Total Spill Count - %d\n", hdd->getSpillCount());
		printf("----------------------------------\n");
		return false;
	}

	// if number of records consumed all fit in memory
	// then the sorted records will be present in RAM itself
	// hence we simply fetch the sorted records from the RAM
	if(_consumed <= (_plan->ram_capacity - _plan->page_size)) {
		row = dram->getSortedRowFromRAM();
	}

	// if after internal sorting we only had one single sorted run
	// then we load that run from HDD onto RAM and consume record by record from RAM
	else if(isSingleSortedRun) {
		// in this case we have one sorted run on HDD which needs to be loaded on RAM
		row = dram->getRowFromSingleSortedRunOnHDD(*hdd);
	}

	// We use the final merging Tree of Losers created to get the sorted Records
	else {
		int totalBuffers = _plan->ram_capacity / _plan->page_size;
		int B = totalBuffers - 1;

		// gets it using the Tree of Losers
		row = dram->getNextSortedRow(*hdd, 0, B);
	}

	// If we get a row whose offsetValue is INT_MAX it means it is a Positive Fence Record
	// Which means we have reached the end of our sorted run (in this case whole sorted input is produced)
	if(row.offsetValue == INT_MAX) {
		// incase output_buffer is partially filled this method drains/spills it to HDD
		// so that HDD has entire input sorted
		dram->cleanupMerging(*hdd);
		printf("\n------------------------------------------------\n");
		printf("HDD Total Spill Count - %d (%.3f x I)\n", hdd->getSpillCount(), (hdd->getSpillCount() * 1.0) / _consumed);
		printf("------------------------------------------------\n");
		return false;
	}

	if (_produced >= _consumed) {
		printf("\n------------------------------------------------\n");
		printf("HDD Total Spill Count - %d (%.3f x I)\n", hdd->getSpillCount(), (hdd->getSpillCount() * 1.0) / _consumed);
		printf("------------------------------------------------\n");
		return false;
	}

	++ _produced;
	return true;
} // SortIterator::next

void SortIterator::free (Row & row)
{
	TRACE (false);
} // SortIterator::free
