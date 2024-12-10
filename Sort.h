#include "Iterator.h"
#include "DRAM.h"
#include "HDD.h"

class SortPlan : public Plan
{
	friend class SortIterator;
public:
	SortPlan (char const * const name, int ram_capacity, int page_size, Plan * const input);
	~SortPlan ();
	Iterator * init () const;
private:
	Plan * const _input;
	int ram_capacity;
	int page_size;
}; // class SortPlan

class SortIterator : public Iterator
{
public:
	SortIterator (SortPlan const * const plan);
	~SortIterator ();
	bool next (Row & row);
	void free (Row & row);
private:
	SortPlan const * const _plan;
	Iterator * const _input;
	RowCount _consumed, _produced;
	DRAM * dram;
	HDD * hdd;

	bool isSingleSortedRun;
}; // class SortIterator
