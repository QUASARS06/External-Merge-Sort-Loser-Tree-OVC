## External Merge Sort Implementation

A brief description of what this project does and who it's for.

### Run All Tests
> make run_all

### Run Individual Tests with default values
> ./Test1.exe  
> ./Test2.exe

### Run Individual Tests with Arguments
Below table shows the arguments which can be passed to the Test program. A sample invocation to any test would look like this:

> ./Test1.exe -p 400 -r 2000 -n 40000 -c 4 -d 7 -fc 0 -fv 1 -fo '>' -s 0  
> ./Test1.exe -p 500 -r 20500 -n 22000 -c 5 -d 10 -fc 1 -fv 1 -fo '>' -s 0   

Note: The filter operator needs to be wrapped by single quotes to avoid shell to interpret it as a redirection

Above command runs Test1.exe with the following parameters:  
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