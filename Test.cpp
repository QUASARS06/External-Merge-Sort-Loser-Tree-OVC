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
	int value = 0;
	char operator_type = '>';

	// RAM
	int ram_capacity = 3;	// number of records that can be stored in RAM

	Plan *const plan =
		new WitnessPlan ("output",
				new SortPlan ("*** The main thing! ***", ram_capacity,
					new WitnessPlan ("input",
						new FilterPlan ("half", col_num, value, operator_type,
							new ScanPlan ("source", 13)
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
