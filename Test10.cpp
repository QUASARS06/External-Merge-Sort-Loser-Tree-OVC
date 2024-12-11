#include "Iterator.h"
#include "Scan.h"
#include "Filter.h"
#include "Sort.h"
#include "Witness.h"
#include <map>
#include <cstdlib>
using namespace std;

int main(int argc, char *argv[])
{
	TRACE(false);

	// Map to store arguments and their values
    std::map<std::string, std::string> argMap;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg[0] == '-' && i + 1 < argc) {
            argMap[arg] = argv[++i]; // Add flag and its value to the map
        }
    }

	srand(static_cast<unsigned int>(time(0)));

	printf("------------------------------ Test 10 --------------------------------\n");
	printf("Description : Random Negative Numbers (see scan_type = 7 below)\n\n");
	
	// number of columns in each Row of Database Record
	int num_of_cols = argMap.find("-c") != argMap.end() ? std::atoi(argMap["-c"].c_str()) : 4;

	// Domain of the column values within a Row
	int col_val_domain = argMap.find("-d") != argMap.end() ? std::atoi(argMap["-d"].c_str()) : 10;


	// filter variables
	// below variables make the following filter : row.columns[col_num] |operator_type| value
	//											   row.columns[0] > 3;
	// allowed operators = '>'  '<'  '='
	int col_num = argMap.find("-fc") != argMap.end() ? std::atoi(argMap["-fc"].c_str()) : 0;
	int value = argMap.find("-fv") != argMap.end() ? std::atoi(argMap["-fv"].c_str()) : 1;
	char operator_type = (argMap.find("-fo") != argMap.end() && !argMap["-fo"].empty()) ? argMap["-fo"][0] : '<';

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

	// number of records that can be stored in RAM
	int ram_capacity = argMap.find("-r") != argMap.end() ? std::atoi(argMap["-r"].c_str()) : 2000;

	// page_size = 20 means 1 page can store 20 records
	int page_size = argMap.find("-p") != argMap.end() ? std::atoi(argMap["-p"].c_str()) : 400;

	// Total number of Rows/Records to be generated
	int num_of_records = argMap.find("-n") != argMap.end() ? std::atoi(argMap["-n"].c_str()) : 40000;

	printf("\nNumber of Columns - %d\n", num_of_cols);
	printf("Domain of Column Values - %d\n", col_val_domain);
	printf("\nRAM Capacity (M) - %d\n", ram_capacity);
	printf("Page Size - %d\n", page_size);
	printf("Number of Input Rows (I) - %d (%0.1f x M)\n", num_of_records, (num_of_records*1.0/(ram_capacity-page_size)));
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
	int scan_type = argMap.find("-s") != argMap.end() ? std::atoi(argMap["-s"].c_str()) : 7;

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
