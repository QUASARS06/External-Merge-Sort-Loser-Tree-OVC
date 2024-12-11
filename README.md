## External Merge Sort Implementation

* Implementation of External Merge Sort by generating Database Rows and simulating how they would flow between HDD, RAM and CPU Cache for sorting.  
* Uses Offset Value Coding (OVC) to minimize the number of column value comparisons to determine sort order between two records.  
* Also uses a Tree of Losers Priority Queue to reduce the number of row comparisons. New winner can be determined with a single leaf to root pass with only a single comparison at each level (as compared to 2 comparisons in standard Priority Queues). Furthermore, with OVC usually a single integer comparison can sort two Rows.
* Also implemented Cache Sized Runs while sorting the Rows in RAM.
* Implemented Graceful Degradation (internal to external sort) to minimize the number of rows spilled to disk. We only spill the required number of sorted rows to accomodate additional input rows, instead of spilling an entire run once it is sorted. Usually the number of records spilled is (see output logs when running tests)
    > number of passes (including initial sorting pass) * I.
* Also implemented the 1-step to n-step Graceful Degradation which merges a certain 'X' runs at the start to fully utilize the 'B' RAM buffers in subsequent merge steps. Here, we always merge smaller runs first thus reducing the amount of I/O.
* Implemented basic filter to filter out certain records based on filter condition (see usage below)
* Implemented basic Witness Step which counts the number of records, parity and inversion in both Input and Output. Number of Records and Parity should be same in Input and Output Witness, while the number of inversions should be zero in the Output Witness.

### Run All Tests
Total Number of Tests = 15 (Test0.exe to Test14.exe)
> make run_all

### Run Individual Tests with default values
> ./Test0.exe  
> ./Test1.exe

### Run Individual Tests with Arguments
Below table shows the arguments which can be passed to the Test program. A sample invocation to any test would look like this:

> ./Test0.exe -p 400 -r 2000 -n 40000 -c 4 -d 7 -fc 0 -fv 1 -fo '>' -s 0  

Note: The filter operator needs to be wrapped by single quotes to avoid shell to interpret it as a redirection

Above command runs Test0.exe with the following parameters:  
* Page Size: 400 (records)
* RAM Capacity: 2,000 (records)
* Number of Rows/Records: 20,000 (records)
* Number of Columns: 4
* Column Value Domain: 7
* Filter Column Number: 0
* Filter Value: 1
* Filter Operator: '>'
* Scan Type: 0 (random records, see below for more info)

Filter evaluates to:  
> select * from rows where row.columns[0] > 1;  


| Argument | Name | Description | Possible Values |
| ------ | ------ | ------ | ------ |
| -p | Page Size | page_size = 20 means 1 page can store 20 records | Positive Even Integer (smaller than ram capacity) |
| -r | RAM Capacity | number of records that can be stored in RAM | Positive Even Integer (should be multiple of page size  and big enough for atleast 2 buffers) |
| -n | Number of Rows | Total number of Rows/Records to be generated | Whole Number |
| -c | Number of Columns | number of columns in each Row of Database Record | Positive Integer |
| -d | Column Value Domain | Domain of the column values within a Row | Integer |
| -fc | Filter Column Number | Syntax: row.columns[**'col_num'**] operator_type value | 0-based column index lower than number of columns |
| -fv | Filter Value | Syntax: row.columns[col_num] **'operator_type'** value | Ideally would be within the Column value Domain but could be any integer |
| -fo | Filter Operator | Syntax: row.columns[col_num] operator_type **'value'** | Allowed operators: **'>'  '<'  '='** (needs to be wrapped by single quotes to avoid shell to interpret it as a redirection)|
| -s | Scan Type | scan_type helps determine the type of random records to generate | 0-7 (see below for individual meaning) |

Scan Type Values:  
* 0 / default - random records
* 1 - all records same
* 2 - all column values same
* 3 - records same and values of columns also same
* 4 - ascending generated records
* 5 - descending generated records
* 6 - all zeroes
* 7 - random negative records