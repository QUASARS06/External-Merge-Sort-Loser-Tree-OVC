#include "Witness.h"

WitnessPlan::WitnessPlan (char const * const name, Plan * const input)
	: Plan (name), _input (input)
{
	TRACE (false);
} // WitnessPlan::WitnessPlan

WitnessPlan::~WitnessPlan ()
{
	TRACE (false);
	delete _input;
} // WitnessPlan::~WitnessPlan


// ------------------------------------------------------------
// -------------------- WITNESS ITERATOR ----------------------
// ------------------------------------------------------------

Iterator * WitnessPlan::init () const
{
	TRACE (false);
	return new WitnessIterator (this);
} // WitnessPlan::init

WitnessIterator::WitnessIterator (WitnessPlan const * const plan) :
	_plan (plan), _input (plan->_input->init ()),
	_rows (0)
{
	TRACE (false);
} // WitnessIterator::WitnessIterator

WitnessIterator::~WitnessIterator ()
{
	TRACE (false);

	delete _input;
	printf("------------------------------------------------------------------------\n");
	traceprintf ("%s witnessed %lu rows\n",
			_plan->_name,
			(unsigned long) (_rows));
	printf("------------------------------------------------------------------------\n");			
} // WitnessIterator::~WitnessIterator

bool WitnessIterator::next (Row & row)
{
	TRACE (true);

	if ( ! _input->next (row))  return false;
	++ _rows;
	return true;
} // WitnessIterator::next

void WitnessIterator::free (Row & row)
{
	TRACE (false);
	_input->free (row);
} // WitnessIterator::free
