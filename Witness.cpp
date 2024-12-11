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

Iterator * WitnessPlan::init () const
{
	TRACE (false);
	return new WitnessIterator (this);
} // WitnessPlan::init

WitnessIterator::WitnessIterator (WitnessPlan const * const plan) :
	_plan (plan), _input (plan->_input->init ()),
	_rows (0), _parity(0), _prevRow(), _inversions(0)
{
	TRACE (false);
} // WitnessIterator::WitnessIterator

WitnessIterator::~WitnessIterator ()
{
	TRACE (false);

	delete _input;
	printf("---------------------------------------------------------------------------------------------------------\n");
	traceprintf ("%s witnessed %lu rows with parity %d and inversions %d\n",
			_plan->_name, (unsigned long) (_rows), _parity, _inversions);
	printf("---------------------------------------------------------------------------------------------------------\n");			
} // WitnessIterator::~WitnessIterator

bool WitnessIterator::next (Row & row)
{
	TRACE (false);

	if ( ! _input->next (row))  return false;
	++ _rows;

	// parity check - xor
	for(size_t i = 0; i < row.columns.size(); i++) _parity ^= (row.columns[i] << i);

	// inversions count
    for (size_t i = 0; i < std::min(_prevRow.columns.size(), row.columns.size()); i++) {
        if (_prevRow.columns[i] > row.columns[i]) {
            _inversions++;
			break;
        } else if (_prevRow.columns[i] < row.columns[i]) {
            // No need to check further; lexicographical order satisfied
            break;
        }
    }

	_prevRow = row;

	return true;
} // WitnessIterator::next

void WitnessIterator::free (Row & row)
{
	TRACE (false);
	_input->free (row);
} // WitnessIterator::free
