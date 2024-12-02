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
	_rows (0), _parity(0), _prevRow(), _inversions(0)
{
	TRACE (false);
} // WitnessIterator::WitnessIterator

WitnessIterator::~WitnessIterator ()
{
	TRACE (false);

	delete _input;
	printf("\n-----------------------------------------------------------------------------------------\n");
	traceprintf ("%s witnessed %lu rows with parity %d and inversions %d\n",
			_plan->_name,
			(unsigned long) (_rows),
			_parity,
			_inversions);
	printf("-----------------------------------------------------------------------------------------\n\n");			
} // WitnessIterator::~WitnessIterator

bool WitnessIterator::next (Row & row)
{
	TRACE (false);

	if ( ! _input->next (row))  return false;
	++ _rows;

	// parity check - xor
	for(size_t i = 0; i < row.columns.size(); i++) _parity ^= (row.columns[i] << i);

	// inversions count
	// for(size_t i = 0; i < row.columns.size(); i++) {
	// 	for(size_t j = 0; j < _prevRow.columns.size(); j++) {
	// 		if (row.columns[i] > _prevRow.columns[j]) {
	// 			_inversions++;
	// 		}
	// 	}
	// }

	// Compare element-wise
    for (size_t i = 0; i < std::min(_prevRow.columns.size(), row.columns.size()); i++) {
        if (_prevRow.columns[i] > row.columns[i]) {
            _inversions++;
        } else if (_prevRow.columns[i] < row.columns[i]) {
            // No need to check further; lexicographical order satisfied
            break;
        }
    }

	// printf("[");
    // for(int i=0 ; i < row.columns.size() ; i++) {
    //     printf("%d", row.columns[i]);
    //     if(i < row.columns.size()-1) printf(", ");
    // }
    // // printf("]  |  Offset = %d  |  Offset Value = %d\n", row.offset, row.offsetValue);
	// printf("]  |  Inversions = %d\n", _inversions);

	_prevRow = row;

	return true;
} // WitnessIterator::next

void WitnessIterator::free (Row & row)
{
	TRACE (false);
	_input->free (row);
} // WitnessIterator::free
