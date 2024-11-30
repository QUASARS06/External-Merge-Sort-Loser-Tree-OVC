#include "Iterator.h"
#include "Scan.h"
#include "Filter.h"
#include "Sort.h"
#include "Witness.h"

int main(int argc, char *argv[])
{
	TRACE(true);

	srand(static_cast<unsigned int>(time(0)));

	// filter variables
	int col_num = 0;
	int value = -1;
	char operator_type = '>';

	// RAM
	// int ram_capacity = 15;	// number of records that can be stored in RAM
	// int page_size = 5;

	// int num_of_records = 60;

	int ram_capacity = 20;	// number of records that can be stored in RAM
	int page_size = 4;

	int num_of_records = 60;

	if(ram_capacity == page_size || ram_capacity % page_size != 0) return -1;

	std::srand(42);

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

	printf("\nPlan Created | Calling INIT\n\n");

	Iterator *const it = plan->init();

	printf("\nInitialization Done | Calling RUN\n\n");

	it->run();
	delete it;

	delete plan;

	return 0;
} // main
