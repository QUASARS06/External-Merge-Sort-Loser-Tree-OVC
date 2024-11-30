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
TreeOfLosers::TreeOfLosers(std::vector<Row>& sortedRuns, int pageSize, int sortedRunSize, std::vector<int>& currentIndices, int& lastWinnerRunIdx)
    : sortedRuns(sortedRuns), pageSize(pageSize), sortedRunSize(sortedRunSize), currentIndices(currentIndices), lastWinnerRunIdx(lastWinnerRunIdx) {
    
    loserTreeHeight = (int)std::ceil(std::log(sortedRunSize) / std::log(2.0));
    numOfLoserNodes = (int)std::pow(2, loserTreeHeight) - 1;
    numOfRuns = (sortedRunSize + pageSize - 1) / pageSize;
    treeSize = (int)std::pow(2, loserTreeHeight - 1) + (int)std::ceil(numOfRuns / 2.0) - 1;

    // printf("pageSize = %d\n", pageSize);
    // printf("sortedRunSize = %d\n", sortedRunSize);
    // printf("loserTreeHeight = %d\n", loserTreeHeight);
    // printf("numOfLoserNodes = %d\n", numOfLoserNodes);
    // printf("numOfRuns = %d\n", numOfRuns);
    // printf("treeSize = %d\n", treeSize);

    // Clear and resize currentIndices
    currentIndices.clear();
    currentIndices.resize(numOfRuns, 0); // Initialize with 0 for all runs

    for (int i = 0; i < numOfRuns; ++i) {
        currentIndices[i] = pageSize * i; // Start at the first row of each run
    }

    for (int i = 0; i < treeSize; ++i) {
        //currentIndices[i] = pageSize * i; // Start at the first row of each run
        loserTree.push_back(LoserTreeNode(i)); // Initialize tree
    }

    loserTree.push_back(LoserTreeNode(numOfRuns)); // Add final tree node
}

Row& TreeOfLosers::getRow(int runIndex) {
    if(runIndex >= 0 && runIndex < numOfRuns) {
        int startIdx = pageSize * runIndex;
        int endIdx = startIdx + pageSize - 1;

        int currIdx = currentIndices[runIndex];

        if(startIdx <= currIdx && currIdx <= endIdx) {
            return sortedRuns[currIdx];
        }

        // printf("----------------------------- DAMN -----------------------------\n");
        // printf("runIndex = %d\n", runIndex);
        // printf("startIdx = %d\n", startIdx);
        // printf("endIdx = %d\n", endIdx);
        // printf("currIdx = %d\n\n", currIdx);
    }

    

    static Row dummyRow({std::numeric_limits<int>::max(), std::numeric_limits<int>::max()},
                    std::numeric_limits<int>::min(),
                    std::numeric_limits<int>::max());
    return dummyRow;
}


int TreeOfLosers::init(int currNodeIdx) {

    if (currNodeIdx > treeSize)
        return currNodeIdx;

    int leftChildIdx = 2 * currNodeIdx;
    int rightChildIdx = 2 * currNodeIdx + 1;

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

    std::string loserOvc = loserRow->findOVC(*winnerRow);
    size_t delimiterPos = loserOvc.find("@");
    loserRow->offset = std::stoi(loserOvc.substr(0, delimiterPos));
    loserRow->offsetValue = std::stoi(loserOvc.substr(delimiterPos + 1));

    loserTree[currNodeIdx].runIndex = loserRunIdx;

    return winnerRunIdx;
}

void TreeOfLosers::initializeTree() {
    int winnerRunIdx = init(1);
    LoserTreeNode& overallWinner = loserTree[0];
    overallWinner.runIndex = winnerRunIdx;

}

Row TreeOfLosers::getNextRow() {
    int winnerRunIdx = loserTree[0].runIndex;
    Row& winnerRow = getRow(winnerRunIdx);

    if (winnerRow.offsetValue == INT_MAX) return winnerRow;

    currentIndices[winnerRunIdx] += 1;
    updateTree(winnerRunIdx);

    lastWinnerRunIdx = winnerRunIdx;
    return winnerRow;
}

void TreeOfLosers::updateTree(int competitorRunIndex) {
    int currLoserTreeNodeIndex = (competitorRunIndex + numOfLoserNodes + 1) / 2;

    while (currLoserTreeNodeIndex > 0) {
        bool didCurrWin = false;

        int currLoserTreeNodeRunIndex = loserTree[currLoserTreeNodeIndex].runIndex;
        Row& currRow = getRow(currLoserTreeNodeRunIndex);
        Row& competitorRow = getRow(competitorRunIndex);

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

    loserTree[currLoserTreeNodeIndex].runIndex = competitorRunIndex;
}
