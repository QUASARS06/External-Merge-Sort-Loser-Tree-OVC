#include "Filter.h"

FilterPlan::FilterPlan (char const * const name, int col_num, int value, char operator_type, Plan * const input)
	: Plan (name), _input (input), col_num (col_num), value (value), operator_type (operator_type)
{
	TRACE (true);
} // FilterPlan::FilterPlan

FilterPlan::~FilterPlan ()
{
	TRACE (true);
	delete _input;
} // FilterPlan::~FilterPlan

Iterator * FilterPlan::init () const
{
	TRACE (true);
	return new FilterIterator (this);
} // FilterPlan::init

FilterIterator::FilterIterator (FilterPlan const * const plan) :
	_plan (plan), _input (plan->_input->init ()),
	_consumed (0), _produced (0)
{
	TRACE (true);
} // FilterIterator::FilterIterator

FilterIterator::~FilterIterator ()
{
	TRACE (true);

	delete _input;

	printf("------------------------------------------------------------------------\n");
	traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_produced),
			(unsigned long) (_consumed));
	printf("------------------------------------------------------------------------\n");
} // FilterIterator::~FilterIterator

bool FilterIterator::next (Row & row)
{
	TRACE (true);

	for (;;)
	{
		if ( ! _input->next (row))  return false;

		++ _consumed;
		
		if ( (_plan->operator_type == '<' && row.columns[_plan->col_num] < _plan->value) ||
		    (_plan->operator_type == '>' && row.columns[_plan->col_num] > _plan->value) )
			break;

		_input->free (row);
	}

	++ _produced;
	return true;
} // FilterIterator::next

void FilterIterator::free (Row & row)
{
	TRACE (true);
	_input->free (row);
} // FilterIterator::free
