#include "Iterator.h"
#include "Scan.h"
#include "Filter.h"
#include "Sort.h"
#include "Witness.h"

int main(int argc, char *argv[])
{
	TRACE(false);

	srand(static_cast<unsigned int>(time(0)));

	printf("------------------------------ Test 2 --------------------------------\n");
	printf("Description : Generates random rows and applies basic filter\n\n");

	int num_of_cols = 4;
	int col_val_domain = 7;

	// filter variables
	// below variables make the following filter : row.columns[col_num] |operator_type| value
	//											   row.columns[0] > 3;
	// allowed operators = '>'  '<'  '='
	int col_num = 0;
	int value = -1;
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
	printf("----------------------------------------------------------------------\n");

	// RAM


	// int ram_capacity = 12;	// number of records that can be stored in RAM
	// int page_size = 2;

	// int num_of_records = 22;
	// int num_of_records = 20;


	// int ram_capacity = 15;	// number of records that can be stored in RAM
	// int page_size = 5;

	// int num_of_records = 64;

	// int ram_capacity = 20;	// number of records that can be stored in RAM
	// int page_size = 4;

	// int num_of_records = 64;

	// int ram_capacity = 2000;	// number of records that can be stored in RAM
	// int page_size = 40;

	// int num_of_records = 40000;

	// int ram_capacity = 2000;	// number of records that can be stored in RAM
	// int page_size = 20;

	// int num_of_records = 10000;

	int ram_capacity = 15;	// number of records that can be stored in RAM
	int page_size = 5;

	int num_of_records = 23;

	std::srand(42);
	// std::srand(1);

    int B = (int)(ram_capacity / page_size) - 1;

	if(ram_capacity % page_size != 0 || B <= 1) {
		if(B <= 1) printf("ERR!!!! There should be atleast 2 buffers for merging\n");
		else printf("ERR!!!! Ram capacity has to be a multiple of page size");

		return -1;
	}

	// scan_type helps determine the type of random records to generate
	// 1 - random records
	// 2 - all records same
	// 3 - all column values same
	// 4 - records as well as column values same
	// 5 - ascending generated records
	// 6 - descending generated records
	// 7 - all zeroes
	int scan_type = 1;

	Plan *const plan =
		new WitnessPlan ("output",
				new SortPlan ("*** The main thing! ***", ram_capacity, page_size,
					new WitnessPlan ("input",
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
