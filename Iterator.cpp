#include "Iterator.h"

Row::Row ()
{
	TRACE (false);
} // Row::Row

Row::~Row ()
{
	TRACE (false);
} // Row::~Row

Plan::Plan (char const * const name) : _name (name)
{
	printf("Name = %s\n", name);
	TRACE (true);
} // Plan::Plan

Plan::~Plan ()
{
	printf("DELETING Name = %s\n", _name);
	TRACE (true);
} // Plan::~Plan

Iterator::Iterator () : _rows (0)
{
	TRACE (true);
} // Iterator::Iterator

Iterator::~Iterator ()
{
	TRACE (true);
} // Iterator::~Iterator

void Iterator::run ()
{
	TRACE (true);

	for (Row row;  next (row);  free (row))
		++ _rows;

	traceprintf ("entire plan produced %lu rows\n", (unsigned long) _rows);
} // Iterator::run
