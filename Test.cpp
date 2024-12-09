#include "Iterator.h"
#include "Scan.h"
#include "Filter.h"
#include "Sort.h"
#include "Witness.h"

int main(int argc, char *argv[])
{
	TRACE(false);

	srand(static_cast<unsigned int>(time(0)));

	// filter variables
	int col_num = 0;
	int value = -1;
	char operator_type = '>';

	// RAM


	// int ram_capacity = 12;	// number of records that can be stored in RAM
	// int page_size = 2;

	// int num_of_records = 22;
	// int num_of_records = 20;


	int ram_capacity = 15;	// number of records that can be stored in RAM
	int page_size = 5;

	int num_of_records = 64;

	// int ram_capacity = 20;	// number of records that can be stored in RAM
	// int page_size = 4;

	// int num_of_records = 64;

	// int ram_capacity = 2000;	// number of records that can be stored in RAM
	// int page_size = 40;

	// int num_of_records = 40000;

	std::srand(42);

    int B = (int)(ram_capacity / page_size) - 1;

	if(ram_capacity % page_size != 0 || B <= 1) {
		if(B <= 1) printf("ERR!!!! There should be atleast 2 buffers for merging\n");
		else printf("ERR!!!! Ram capacity has to be a multiple of page size");

		return -1;
	}

	Plan *const plan =
		new WitnessPlan ("output",
				new SortPlan ("*** The main thing! ***", ram_capacity, page_size,
					new WitnessPlan ("input",
						new FilterPlan ("half", col_num, value, operator_type,
							new ScanPlan ("source", num_of_records)
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
