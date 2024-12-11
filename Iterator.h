#pragma once

#include "defs.h"
#include <vector>

typedef uint64_t RowCount;

class Row
{
public:
	Row();
	virtual ~Row();

	Row(const std::vector<int>& columns, int offset, int offsetValue);
	
	std::vector<int> columns;
	int offset;
	int offsetValue;

	bool isLessThan(const Row& other, int& ct) const;

private:
	// ...
}; // class Row

class Plan
{
	friend class Iterator;

public:
	Plan(char const *const name);
	virtual ~Plan();
	virtual class Iterator *init() const = 0;

protected:
	char const *const _name;

private:
}; // class Plan

class Iterator
{
public:
	Iterator();
	virtual ~Iterator();
	void run();
	virtual bool next(Row &row) = 0;
	virtual void free(Row &row) = 0;

private:
	RowCount _rows;
}; // class Iterator
