#include "TreeOfLosers.h"
#include <cmath>
#include <climits>
#include <sstream>
#include <algorithm>
#include <iostream>
#include "Iterator.h"
using namespace std;

// Row implementation
Row::Row(const std::vector<int>& columns, int offset, int offsetValue) : columns(columns), offset(offset), offsetValue(offsetValue) {}

bool Row::isLessThan(const Row& other) const {
    if (offset != other.offset) {
        return offset > other.offset;
    }
    if (offsetValue != other.offsetValue) {
        return offsetValue < other.offsetValue;
    }

    for (size_t i = 0; i < columns.size(); ++i) {
        if (columns[i] != other.columns[i]) {
            return columns[i] < other.columns[i];
        }
    }

    return true;
}

std::string Row::findOVC(const Row& winner) const {
    if(offsetValue == INT_MAX && winner.offsetValue == INT_MAX) {
        return std::to_string(std::numeric_limits<int>::min()) + "@" + std::to_string(std::numeric_limits<int>::max());
    }
    for (size_t i = 0; i < winner.columns.size(); ++i) {
        if (winner.columns[i] != columns[i]) {
            return std::to_string(i) + "@" + std::to_string(columns[i]);
        }
    }
    return std::to_string(winner.columns.size()) + "@" + std::to_string(winner.columns.back());
}






// LoserTreeNode implementation
LoserTreeNode::LoserTreeNode(int runIndex) : runIndex(runIndex) {}

LoserTreeNode::~LoserTreeNode () {}




// TreeOfLosers implementation
TreeOfLosers::TreeOfLosers(const std::vector<std::vector<Row> > sortedRuns)
    : sortedRuns(sortedRuns) {
    
    loserTreeHeight = (int)std::ceil(std::log(sortedRuns.size()) / std::log(2.0));
    numOfLoserNodes = (int)std::pow(2, loserTreeHeight) - 1;
    numOfRuns = sortedRuns.size();
    treeSize = (int)std::pow(2, loserTreeHeight - 1) + (int)std::ceil(numOfRuns / 2.0) - 1;

    for (int i = 0; i < treeSize; ++i) {
        currentIndices.push_back(0); // Start at the first row of each run
        loserTree.push_back(LoserTreeNode(i)); // Initialize tree
    }

    loserTree.push_back(LoserTreeNode(numOfRuns)); // Add final tree node
}

// Row TreeOfLosers::getRow(int runIndex) const {
//     if (runIndex >= 0 && runIndex < numOfRuns && currentIndices[runIndex] < sortedRuns[runIndex].size()) {
//         return sortedRuns[runIndex][currentIndices[runIndex]];
//     }
//     return Row({ std::numeric_limits<int>::max(), std::numeric_limits<int>::max()},
//                    std::numeric_limits<int>::min(),
//                    std::numeric_limits<int>::max());
// }

Row& TreeOfLosers::getRow(int runIndex) {
    //printf("CALL %d\n", runIndex);
    if (runIndex >= 0 && runIndex < numOfRuns && currentIndices[runIndex] < sortedRuns[runIndex].size()) {
       // printf("HERE\n");
       // printf("1 %d\n",currentIndices[runIndex]);
        //printf("2\n");
        return sortedRuns[runIndex][currentIndices[runIndex]];
    }
    //printf("REALLY?\n");
    // Return a maximum value row if the run is exhausted
     static Row dummyRow({std::numeric_limits<int>::max(), std::numeric_limits<int>::max()},
                        std::numeric_limits<int>::min(),
                        std::numeric_limits<int>::max());
    return dummyRow;
}


int TreeOfLosers::init(int currNodeIdx) {
    //printf("START!\n");
    if (currNodeIdx > treeSize)
        return currNodeIdx;

    int leftChildIdx = 2 * currNodeIdx;
    int rightChildIdx = 2 * currNodeIdx + 1;

    // LoserTreeNode& loser = loserTree[currNodeIdx];
    int leftRunIdx = -1;
    int rightRunIdx = -1;

    if (leftChildIdx > treeSize) {
        leftRunIdx = leftChildIdx - (numOfLoserNodes + 1);
        rightRunIdx = rightChildIdx - (numOfLoserNodes + 1);
    } else {
        leftRunIdx = init(leftChildIdx);
        rightRunIdx = init(rightChildIdx);
    }

    Row& leftRow = getRow(leftRunIdx);
    Row& rightRow = getRow(rightRunIdx);

    Row* winnerRow = nullptr;
    Row* loserRow = nullptr;
    int winnerRunIdx = -1, loserRunIdx = -1;

    if (leftRow.isLessThan(rightRow)) {
        winnerRow = &leftRow;
        loserRow = &rightRow;
        winnerRunIdx = leftRunIdx;
        loserRunIdx = rightRunIdx;
    } else {
        winnerRow = &rightRow;
        loserRow = &leftRow;
        winnerRunIdx = rightRunIdx;
        loserRunIdx = leftRunIdx;
    }

    
    //printf("Curr Node = %d | Match between P1: %d v/s P2: %d  |  Winner = %d  |  Loser = %d\n", currNodeIdx, leftRunIdx, rightRunIdx, winnerRunIdx, loserRunIdx);

    // printf("Winner\n");
    // printf("cols = [");
    // for (size_t k = 0; k < winnerRow.columns.size(); ++k) {
    //     printf("%d", winnerRow.columns[k]);
    //     if (k < winnerRow.columns.size() - 1) printf(", ");
    // }
    // printf("] ");

    // printf("offset = %d ", winnerRow.offset);
    // printf("offsetValue = %d\n", winnerRow.offsetValue);

    // printf("\nLoser\n");
    // printf("cols = [");
    // for (size_t k = 0; k < loserRow.columns.size(); ++k) {
    //     printf("%d", loserRow.columns[k]);
    //     if (k < loserRow.columns.size() - 1) printf(", ");
    // }
    // printf("] ");

    // printf("offset = %d ", loserRow.offset);
    // printf("offsetValue = %d\n\n", loserRow.offsetValue);

    std::string loserOvc = loserRow->findOVC(*winnerRow);
    size_t delimiterPos = loserOvc.find("@");
    loserRow->offset = std::stoi(loserOvc.substr(0, delimiterPos));
    loserRow->offsetValue = std::stoi(loserOvc.substr(delimiterPos + 1));

    loserTree[currNodeIdx].runIndex = loserRunIdx;
    // loser.runIndex = loserRunIdx;

    return winnerRunIdx;
}

void TreeOfLosers::initializeTree() {
    int winnerRunIdx = init(1);
    LoserTreeNode& overallWinner = loserTree[0];
    overallWinner.runIndex = winnerRunIdx;

    //printf("DONE ?\n");

    // for(int i=0;i<loserTree.size();i++) {
    //     printf("%d -> Run Idx = %d\n", i, loserTree[i].runIndex);
    // }

    
    // // Loop through each run in sortedRuns
    // for (size_t i = 0; i < sortedRuns.size(); ++i) {
    //     printf("Run %zu:\n", i + 1);
        
    //     // Loop through each Row in the current run
    //     for (size_t j = 0; j < sortedRuns[i].size(); ++j) {
    //         const Row& row = sortedRuns[i][j];
            
    //         // Print columns, offset, and offsetValue for the current row
    //         printf("Row %zu: ", j + 1);

    //         printf("cols = [");
    //         for (size_t k = 0; k < row.columns.size(); ++k) {
    //             printf("%d", row.columns[k]);
    //             if (k < row.columns.size() - 1) printf(", ");
    //         }
    //         printf("] ");

    //         printf("offset = %d ", row.offset);
    //         printf("offsetValue = %d\n", row.offsetValue);
    //     }
    //     printf("----------------------------\n");
    // }



}

Row TreeOfLosers::getNextRow() {
    int winnerRunIdx = loserTree[0].runIndex;
    Row& winnerRow = getRow(winnerRunIdx);

    if (winnerRow.offsetValue == INT_MAX) return winnerRow;

    currentIndices[winnerRunIdx] += 1;
    updateTree(winnerRunIdx);

    return winnerRow;
}

void TreeOfLosers::updateTree(int competitorRunIndex) {
    int currLoserTreeNodeIndex = (competitorRunIndex + numOfLoserNodes + 1) / 2;

    while (currLoserTreeNodeIndex > 0) {
        bool didCurrWin = false;

        int currLoserTreeNodeRunIndex = loserTree[currLoserTreeNodeIndex].runIndex;
        Row& currRow = getRow(currLoserTreeNodeRunIndex);
        Row& competitorRow = getRow(competitorRunIndex);

        // printf("\nCurrIdx = %d  |  CompetitorIdx = %d  |  Competition between:\n", currLoserTreeNodeRunIndex, competitorRunIndex);


        // printf("Curr: \n");
        // printf("cols = [");
        // for (size_t k = 0; k < currRow.columns.size(); ++k) {
        //     printf("%d", currRow.columns[k]);
        //     if (k < currRow.columns.size() - 1) printf(", ");
        // }
        // printf("] ");

        // printf("offset = %d ", currRow.offset);
        // printf("offsetValue = %d\n", currRow.offsetValue);

        // printf("\nCP: \n");
        // printf("cols = [");
        // for (size_t k = 0; k < competitorRow.columns.size(); ++k) {
        //     printf("%d", competitorRow.columns[k]);
        //     if (k < competitorRow.columns.size() - 1) printf(", ");
        // }
        // printf("] ");

        // printf("offset = %d ", competitorRow.offset);
        // printf("offsetValue = %d\n\n", competitorRow.offsetValue);

        if (currRow.offset != competitorRow.offset) {
            if (currRow.offset > competitorRow.offset)
                didCurrWin = true;
        } else if (currRow.offsetValue != competitorRow.offsetValue) {
            if (currRow.offsetValue < competitorRow.offsetValue)
                didCurrWin = true;
        } else {
            if (currRow.isLessThan(competitorRow))
                didCurrWin = true;
        }

        if (didCurrWin) {
            loserTree[currLoserTreeNodeIndex].runIndex = competitorRunIndex;

            std::string newOvc = competitorRow.findOVC(currRow);
            int delimiterPos = newOvc.find("@");
            competitorRow.offset = std::stoi(newOvc.substr(0, delimiterPos));
            competitorRow.offsetValue = std::stoi(newOvc.substr(delimiterPos + 1));

            competitorRunIndex = currLoserTreeNodeRunIndex;
        } else {
            std::string newOvc = currRow.findOVC(competitorRow);
            int delimiterPos = newOvc.find("@");
            currRow.offset = std::stoi(newOvc.substr(0, delimiterPos));
            currRow.offsetValue = std::stoi(newOvc.substr(delimiterPos + 1));
        }

        currLoserTreeNodeIndex /= 2;
    }

    //printf("Win = %d\n", competitorRunIndex);
    loserTree[currLoserTreeNodeIndex].runIndex = competitorRunIndex;
}
