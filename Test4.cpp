#include "Iterator.h"
#include "Scan.h"
#include "Filter.h"
#include "Sort.h"
#include "Witness.h"

int main(int argc, char *argv[])
{
	TRACE(false);

	srand(static_cast<unsigned int>(time(0)));

	printf("------------------------------ Test 4 --------------------------------\n");
	printf("Description : Heavy Filtering\n\n");

	int num_of_cols = 4;	// number of columns in each Row of Database Record
	int col_val_domain = 10;		// Domain of the column values within a Row


	// filter variables
	// below variables make the following filter : row.columns[col_num] |operator_type| value
	//											   row.columns[0] > 3;
	// allowed operators = '>'  '<'  '='
	int col_num = 0;
	int value = (col_val_domain - 2);
	char operator_type = '>';

	if(col_num >= num_of_cols) {
		printf("Column Number provided Out of Bounds (col_num should be <= %d)\n", (num_of_cols-1));
		return -1;
	}

	if(operator_type != '>' && operator_type != '<' && operator_type != '=') {
		printf("Invalid Operator Type. It should be one of the following : '>'  '<'  '='\n");
		return -1;
	}

	printf("Filter: select * from rows where row.columns[%d] %c %d\n", col_num, operator_type, value);


	// RAM attributes
	int ram_capacity = 2000;	// number of records that can be stored in RAM
	int page_size = 400;		// page_size = 20 means 1 page can store 20 records

	int num_of_records = 40000;		// Total number of Rows/Records to be generated

	printf("\nNumber of Columns - %d\n", num_of_cols);
	printf("Domain of Column Values - %d\n", col_val_domain);
	printf("\nRAM Capacity - %d\n", ram_capacity);
	printf("Page Size - %d\n", page_size);
	printf("Number of Input Rows - %d\n", num_of_records);
	printf("----------------------------------------------------------------------\n");

    int B = (int)(ram_capacity / page_size) - 1;

	if(ram_capacity % page_size != 0 || B <= 1) {
		if(B <= 1) printf("ERR!!!! There should be atleast 2 buffers for merging\n");
		else printf("ERR!!!! Ram capacity has to be a multiple of page size");

		return -1;
	}

	// scan_type helps determine the type of random records to generate
	// 0 / default - random records
	// 1 - all records same
	// 2 - all column values same
	// 3 - records same and values of columns also same
	// 4 - ascending generated records
	// 5 - descending generated records
	// 6 - all zeroes
	// 7 - random negative records
	int scan_type = 0;

	Plan *const plan =
		new WitnessPlan ("OUTPUT Witness",
				new SortPlan ("*** The main thing! ***", ram_capacity, page_size,
					new WitnessPlan ("INPUT Witness",
						new FilterPlan ("half", col_num, value, operator_type,
							new ScanPlan ("source", num_of_records, num_of_cols, col_val_domain, scan_type)
						)
					)
				)
			);

	Iterator *const it = plan->init();

	it->run();
	delete it;

	delete plan;

	return 0;
} // main
