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

bool Row::isLessThan(const Row& other, int& ct) const {
    if (offset != other.offset) {
        return offset > other.offset;
    }
    if (offsetValue != other.offsetValue) {
        return offsetValue < other.offsetValue;
    }

    for (size_t i = offset+1; i < columns.size(); ++i) {
        ct++;
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

/**
 * sortedRuns - these are the sortedRuns (usually will be the ram itself) of page_size, 
 *              incase of internal sort of size 1
 * 
 * pageSize - the page size of the system or the RAM
 * 
 * sortedRunSize - denotes the number of competitors at the leaf level, 
 *                 **Note** : that this is NOT the number of runs but rather the number of actual competitors
 *                 
 *                 example if there are 2 runs each having 5 elements then sortedRunSize = 10
 *                 So basically is pagesize is 5 and ram capacity is 10 elements then
 *                 
 *                 <---------------------- RAM ---------------------->
 *                 <-------- Run 1 --------> <-------- Run 2 -------->
 *                 | x1 | x2 | x3 | x4 | x5 | y1 | y2 | y3 | y4 | y5 | .... space for output buffer ....
 * 
 * currentIndices - stores the index of the topmost element in the each run
 * 
 * lastWinnerRunIdx - Index of the run from which last winner originated from
 * 
 * sortedRunsIndexOffset - We always pass the RAM to the TreeOfLosers for sorting, in case of cache sized runs
 *                         we divide the ram into cache sized slots and treat each slot as one run
 *                         thus this basically tells the tournament tree the offset or the starting point
 *                         of the current runs being processed, example if cache size t = 3 then the runs would be
 *                         [0, 2] , [3, 5] , [6, 8] and so on.... so the start index of each run 0, 3, 6 would be
 *                         stored in this variable to tell the tournament tree the offset in the RAM. This is used
 *                         in the getRow(....) method below which would get the next row in the run appropriately
 *                         based on this offset.
 * 
 *   ** NOTE : **   -   The Tournament tree always works on 0-based indexing logically which means that runs 
 *                      competing would have indexes 0, 1, 2, 3, ....
 *                      But the actual runs which are competing in this tree may not be 0 based like the above
 *                      cache sized runs, offsets like above help us translate the logical run indexes in the tree
 *                      to actual run indexes in the RAM
 * 
 * useIndirection - tells if the indirection vector (described below) needs to be used or not
 * 
 * indirection - (basically a pointer but in form of indexes) stores the index in RAM which the corresponding
 *               index in sortedRuns points to. In case of cached sized runs we use this indirection to store the
 *               sorted indexes in the indirection array. Example if RAM = [3 , 0 , 1] then indirection = [2 , 0 , 1]
 *               basically 0th index in indirection says that element 3 in RAM belongs at position 2 in sorted sequence
 *                         1st index in indirection says that element 0 in RAM belongs at positon 0 and so on
 * 
 */

TreeOfLosers::TreeOfLosers(std::vector<Row>& sortedRuns, int pageSize, int sortedRunSize, 
                           std::vector<int>& currentIndices, int& lastWinnerRunIdx, int sortedRunsIndexOffset, 
                           bool useIndirection, std::vector<int> indirection)
    : sortedRuns(sortedRuns), currentIndices(currentIndices), pageSize(pageSize), 
      lastWinnerRunIdx(lastWinnerRunIdx), sortedRunsIndexOffset(sortedRunsIndexOffset), 
      useIndirection(useIndirection), indirection(indirection) 
    
    {
    
    // the height of the loser tree depends on the number of sorted runs competing at leaf level
    // because the number of leaf nodes will be half the number of sorted runs competing at leaf level
    loserTreeHeight = (int)std::ceil(std::log(sortedRunSize) / std::log(2.0));

    // number of nodes in the loser tree (almost complete binary tree) depends on the height
    // this includes 1 level extra for the 2 competitors at each leaf in the TOL
    // but note that the last level mentioned above is not part of the actual tree which is constructed in code
    // this variable is used to determine the actual indexes of the runs which are competing at a leaf later on in code
    numOfLoserNodes = (int)std::pow(2.0, loserTreeHeight) - 1;

    // using the sortedRunSize and the pageSize we can calculate the actual number of runs at leaf
    // from our above example we would get => (10 + 5 - 1) / 5 = 2
    // the (pageSize - 1) helps deal with incomplete runs for example if 2nd run only had 1 element
    // then sortedRunSize = 6 and simply doing sortedRunSize / pageSize would give us 1
    // but with this formula we get (6 + 5 - 1) / 5 = 2
    numOfRuns = (sortedRunSize + pageSize - 1) / pageSize;

    // actual number of nodes in the Tree of Losers (Priority Queue Array)
    treeSize = (int)std::pow(2.0, loserTreeHeight - 1) + (int)std::ceil(numOfRuns / 2.0) - 1;

    // printf("\npageSize = %d\n", pageSize);
    // printf("sortedRunSize = %d\n", sortedRunSize);
    // printf("loserTreeHeight = %d\n", loserTreeHeight);
    // printf("numOfLoserNodes = %d\n", numOfLoserNodes);
    // printf("numOfRuns = %d\n", numOfRuns);
    // printf("treeSize = %d\n\n", treeSize);

    // Clear and resize currentIndices
    currentIndices.clear();
    currentIndices.resize(numOfRuns);

    // each run occupies page sized space in the ram
    // so starting index of each run is calculated appropriately below
    for (int i = 0; i < numOfRuns; ++i) {
        currentIndices[i] = pageSize * i; // Start at the first row of each run
    }

    for (int i = 0; i <= treeSize; ++i) {
        loserTree.push_back(LoserTreeNode(i)); // Initialize tree
    }
}

/**
 * For a give runIndex returns the row at the top of the run
 * the index of top of the run is determined using the currentIndices vector
 * Uses indirection if the useIndirection boolean is set
 * In case of out of bounds - run is out of bounds or index in run is out of bounds return MAX Row
 */
Row& TreeOfLosers::getRow(int runIndex) {
    if(runIndex >= 0 && runIndex < numOfRuns) {

        // find the bounds in the RAM for that run
        int startIdx = pageSize * runIndex;
        int endIdx = startIdx + pageSize - 1;

        int currIdx = currentIndices[runIndex];

        // check if current index for that run is within the bounds
        if(startIdx <= currIdx && currIdx <= endIdx && (currIdx + sortedRunsIndexOffset) < sortedRuns.size()) {
            if(useIndirection) return sortedRuns[indirection[currIdx + sortedRunsIndexOffset]];
            else return sortedRuns[currIdx + sortedRunsIndexOffset];
        }
    }

    static Row dummyRow({std::numeric_limits<int>::max()},
                    std::numeric_limits<int>::min(),
                    std::numeric_limits<int>::max());
    return dummyRow;
}

/**
 * Recursively initialize the tournament tree by playing the initial matches between the runs
 * currNodeIdx - denotes the node in the tournament tree where match is being played
 * the match will be played between the left and right child of NODE represented by currNodeIdx
 */
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

    int comparisonsToDetermineLoser = 0;
    if (leftRow.isLessThan(rightRow, comparisonsToDetermineLoser)) {
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

    // std::string loserOvc = loserRow->findOVC(*winnerRow);
    // size_t delimiterPos = loserOvc.find("@");
    // loserRow->offset = std::stoi(loserOvc.substr(0, delimiterPos));
    // loserRow->offsetValue = std::stoi(loserOvc.substr(delimiterPos + 1));

    if(comparisonsToDetermineLoser > 0) {
        int row_len = loserRow->columns.size();
        
        loserRow->offset += comparisonsToDetermineLoser;
        if(loserRow->offset > row_len) {
            loserRow->offset = row_len;
            loserRow->offsetValue = loserRow->columns.back();
        } else {
            loserRow->offsetValue = loserRow->columns[loserRow->offset];
        }
    }

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

        int comparisonsToDetermineLoser = 0;
        // if (currRow.offset != competitorRow.offset) {
        //     if (currRow.offset > competitorRow.offset) didCurrWin = true;
        // } else if (currRow.offsetValue != competitorRow.offsetValue) {
        //     if (currRow.offsetValue < competitorRow.offsetValue) didCurrWin = true;
        // } else {
        //     if (currRow.isLessThan(competitorRow)) didCurrWin = true;
        // }

        if (currRow.isLessThan(competitorRow, comparisonsToDetermineLoser)) {
            didCurrWin = true;
        }

        if (didCurrWin) {
            loserTree[currLoserTreeNodeIndex].runIndex = competitorRunIndex;

            // printf("OLD OVC Loser [%d, %d, %d, %d] => Offset: %d | Offset Value: %d\n", competitorRow.columns[0], competitorRow.columns[1], competitorRow.columns[2], competitorRow.columns[3], competitorRow.offset, competitorRow.offsetValue);
            // printf("OLD OVC Winner [%d, %d, %d, %d] => Offset: %d | Offset Value: %d\n", currRow.columns[0], currRow.columns[1], currRow.columns[2], currRow.columns[3], currRow.offset, currRow.offsetValue);

            // std::string newOvc = competitorRow.findOVC(currRow);
            // int delimiterPos = newOvc.find("@");
            // competitorRow.offset = std::stoi(newOvc.substr(0, delimiterPos));
            // competitorRow.offsetValue = std::stoi(newOvc.substr(delimiterPos + 1));

            if(comparisonsToDetermineLoser > 0) {
                int row_len = competitorRow.columns.size();
                
                competitorRow.offset += comparisonsToDetermineLoser;
                if(competitorRow.offset > row_len) {
                    competitorRow.offset = row_len;
                    competitorRow.offsetValue = competitorRow.columns.back();
                } else {
                    competitorRow.offsetValue = competitorRow.columns[competitorRow.offset];
                }
            }

            // printf("NEW OVC Loser [%d, %d, %d, %d] => Offset: %d | Offset Value: %d\n", competitorRow.columns[0], competitorRow.columns[1], competitorRow.columns[2], competitorRow.columns[3], competitorRow.offset, competitorRow.offsetValue);
            // printf("comparisonsToDetermineLoser = %d\n\n", comparisonsToDetermineLoser);

            competitorRunIndex = currLoserTreeNodeRunIndex;
        } else {
            // std::string newOvc = currRow.findOVC(competitorRow);
            // int delimiterPos = newOvc.find("@");
            // currRow.offset = std::stoi(newOvc.substr(0, delimiterPos));
            // currRow.offsetValue = std::stoi(newOvc.substr(delimiterPos + 1));
            
            if(comparisonsToDetermineLoser > 0) {
                int row_len = currRow.columns.size();
                
                currRow.offset += comparisonsToDetermineLoser;
                if(currRow.offset > row_len) {
                    currRow.offset = row_len;
                    currRow.offsetValue = currRow.columns.back();
                } else {
                    currRow.offsetValue = currRow.columns[currRow.offset];
                }
            }

        }

        currLoserTreeNodeIndex /= 2;
    }

    loserTree[currLoserTreeNodeIndex].runIndex = competitorRunIndex;
}
