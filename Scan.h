#include "Iterator.h"

class ScanPlan : public Plan
{
	friend class ScanIterator;
public:
	ScanPlan (char const * const name, RowCount const count, int const num_of_cols, int const col_val_domain);
	~ScanPlan ();
	Iterator * init () const;
private:
	RowCount const _count;
	int const num_of_cols;
	int const col_val_domain;
}; // class ScanPlan

class ScanIterator : public Iterator
{
public:
	ScanIterator (ScanPlan const * const plan);
	~ScanIterator ();
	bool next (Row & row);
	void free (Row & row);
private:
	ScanPlan const * const _plan;
	RowCount _count;
}; // class ScanIterator
