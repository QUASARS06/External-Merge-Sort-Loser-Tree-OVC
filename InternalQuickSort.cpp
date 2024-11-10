#include "InternalQuickSort.h"
#include "Iterator.h"
#include <vector>

// Function to compare two rows lexicographically by their columns
bool compareRows(const Row& row1, const Row& row2) {
    size_t minSize = std::min(row1.columns.size(), row2.columns.size());
    for (size_t i = 0; i < minSize; ++i) {
        if (row1.columns[i] < row2.columns[i]) {
            return true;
        } else if (row1.columns[i] > row2.columns[i]) {
            return false;
        }
        // If elements are equal, continue to the next column
    }
    // If all compared elements are equal, the shorter vector is considered smaller
    return row1.columns.size() < row2.columns.size();
}

// Partition function for quicksort
int partition(std::vector<Row>& records, int low, int high) {
    Row pivot = records[high];  // Select the last element as the pivot
    int i = low - 1;            // Index of smaller element
    
    for (int j = low; j < high; ++j) {
        if (compareRows(records[j], pivot)) {
            i++;
            std::swap(records[i], records[j]);
        }
    }
    std::swap(records[i + 1], records[high]);
    return i + 1;
}

// Quicksort function for sorting records
void quickSort(std::vector<Row>& records, int low, int high) {
    if (low < high) {
        int pi = partition(records, low, high);  // Partitioning index
        quickSort(records, low, pi - 1);  // Recursively sort the left part
        quickSort(records, pi + 1, high); // Recursively sort the right part
    }
}
