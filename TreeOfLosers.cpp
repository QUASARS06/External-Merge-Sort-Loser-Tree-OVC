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
    // if offsets don't match then row with higher offset is earlier in sort sequence
    if (offset != other.offset) {
        return offset > other.offset;
    }

    // if offsets match and offsetValues differ then row with lower offsetValue is earlier in sort sequence
    if (offsetValue != other.offsetValue) {
        return offsetValue < other.offsetValue;
    }

    // Otherwise, additional data values in the two rows must be compared

    // note that column value comparisons start after the offset
    // since we know that since offsets match those column value comparisons will be redundant
    // thus we save column value comparisons using OVC
    for (size_t i = offset+1; i < columns.size(); ++i) {
        ct++;
        if (columns[i] != other.columns[i]) {
            return columns[i] < other.columns[i];
        }
    }

    return true;
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
 *               basically 0th index in indirection says that element 3 (index 0) in RAM belongs at index 2 in RAM
 *                         in sorted sequence
 *                         1st index in indirection says that element 0 (index 1) in RAM belongs at index 0 and so on
 * 
 */
TreeOfLosers::TreeOfLosers(std::vector<Row>& sortedRuns, int pageSize, int sortedRunSize, 
                           std::vector<int>& currentIndices, int& lastWinnerRunIdx, int sortedRunsIndexOffset, 
                           bool useIndirection, std::vector<int> indirection)
    : sortedRuns(sortedRuns), currentIndices(currentIndices), pageSize(pageSize), sortedRunSize(sortedRunSize),
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
 * In case of out of bounds - if run is out of bounds or index in run is out of bounds return Positive Fence Row
 */
Row& TreeOfLosers::getRow(int runIndex) {
    if(runIndex >= 0 && runIndex < numOfRuns) {

        // find the bounds in the RAM for that run
        int startIdx = pageSize * runIndex;
        int endIdx = std::min(startIdx + pageSize - 1, sortedRunSize-1);

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

    // get LoserTreeNode index for the left and right child of current node to play tournament
    int leftChildIdx = 2 * currNodeIdx;
    int rightChildIdx = 2 * currNodeIdx + 1;

    // stores the run indexes of the runs corresponding to a Row 
    // which are going to compete at the current LoserTree Node
    int leftRunIdx = -1;
    int rightRunIdx = -1;

    if (leftChildIdx > treeSize) {
        // if the LoserTreeNode index goes beyond the treeSize (number of nodes in the LoserTree)
        // it means we are at leaf level, at this point we calculate the run indexes of run
        // which will compete at that leaf node

        leftRunIdx = leftChildIdx - (numOfLoserNodes + 1);
        rightRunIdx = rightChildIdx - (numOfLoserNodes + 1);
    } else {
        // if we are at any internal node then we recursively get the runIndexes of the winners
        // from the left and right childs of the current node

        leftRunIdx = init(leftChildIdx);
        rightRunIdx = init(rightChildIdx);
    }

    // get the actual rows based on the run indexes
    Row& leftRow = getRow(leftRunIdx);
    Row& rightRow = getRow(rightRunIdx);

    Row* loserRow = nullptr;
    int winnerRunIdx = -1, loserRunIdx = -1;

    // the new offset of loser increases by the number of comparisons required to determine winner/loser
    // and the new offsetValue is the value of loser row at that offset
    // Hence we minimize the number of Column Value comparisons using OVC
    int comparisonsToDetermineLoser = 0;

    if (leftRow.isLessThan(rightRow, comparisonsToDetermineLoser)) {
        loserRow = &rightRow;
        winnerRunIdx = leftRunIdx;
        loserRunIdx = rightRunIdx;
    } else {
        loserRow = &leftRow;
        winnerRunIdx = rightRunIdx;
        loserRunIdx = leftRunIdx;
    }

    if(comparisonsToDetermineLoser > 0) {
        int row_len = loserRow->columns.size();

        // new offset of loser increase by number of column value comparisons required to determine winner/loser    
        loserRow->offset += comparisonsToDetermineLoser;

        // handles out of bounds (if it occurs)
        if(loserRow->offset > row_len) {
            loserRow->offset = row_len;
            loserRow->offsetValue = loserRow->columns.back();
        } else {
            // new offsetValue is value at new offset
            loserRow->offsetValue = loserRow->columns[loserRow->offset];
        }
    }

    // set the loserRunIdx at the current node
    loserTree[currNodeIdx].runIndex = loserRunIdx;

    // pass the winnerRunIdx higher up in the LoserTree
    return winnerRunIdx;
}


/**
 * Creates the Initial Tree of Losers
 * By having the initial matches between top record in each Run competing
 */
void TreeOfLosers::initializeTree() {
    int winnerRunIdx = init(1);
    LoserTreeNode& overallWinner = loserTree[0];
    overallWinner.runIndex = winnerRunIdx;
}


/**
 * Pops the root of the Tree of Losers
 * Invokes a leaf-to-root pass to update the Tree of Losers
 */
Row TreeOfLosers::getNextRow() {
    
    int winnerRunIdx = loserTree[0].runIndex; // get the run index of winner from the LoserTree root
    Row& winnerRow = getRow(winnerRunIdx);  // get actual row based on the index

    if (winnerRow.offsetValue == INT_MAX) return winnerRow;

    currentIndices[winnerRunIdx] += 1;  // increase the index for that run to point to next row in the run
    updateTree(winnerRunIdx);   // update tree by doing a leaf to root pass to determine next winner

    lastWinnerRunIdx = winnerRunIdx;
    return winnerRow;
}


/**
 * Performs a leaf to root pass to update the Tree of Losers to determine next winner
 * competitorRunIndex is basically the runIndex of the run of the last winner
 * The next Row in that run competes at the leaf and hence new winner is decided as we go from leaf to root
 */
void TreeOfLosers::updateTree(int competitorRunIndex) {
    
    // we get the index of LoserTreeNode (basically the leaf node) from which the matches are going to start
    int currLoserTreeNodeIndex = (competitorRunIndex + numOfLoserNodes + 1) / 2;

    while (currLoserTreeNodeIndex > 0) {
        // this flag basically means that the Row present in the currLoserTreeNodeIndex which we are at
        // whether is won or not
        // if it won it means that it will go higher up in the loser tree and hence new row will be assigned to this LoserTreeNode
        // but if it was still the loser then we don't do anything, we just update the OVC if required
        bool didCurrWin = false;

        // every time a competitor (previous winner from child nodes) competes at each node while we move up the tree
        int currLoserTreeNodeRunIndex = loserTree[currLoserTreeNodeIndex].runIndex;
        Row& currRow = getRow(currLoserTreeNodeRunIndex);
        Row& competitorRow = getRow(competitorRunIndex);

        // the new offset of loser increases by the number of comparisons required to determine winner/loser
        // and the new offsetValue is the value of loser row at that offset
        // Hence we minimize the number of Column Value comparisons using OVC
        int comparisonsToDetermineLoser = 0;

        // use OVC to determine sort order between two rows
        if (currRow.isLessThan(competitorRow, comparisonsToDetermineLoser)) {
            // currRow - Winner
            // competitorRow - Loser
            didCurrWin = true;
        }

        if (didCurrWin) {
            // currRow - Winner
            // competitorRow - Loser

            // if Row in current LoserTreeNode won then it will go higher up in the tree
            // and the competitor which is the loser will be assigned to this Node
            // OVC will be updated for the loser if required
            loserTree[currLoserTreeNodeIndex].runIndex = competitorRunIndex;

            if(comparisonsToDetermineLoser > 0) {
                int row_len = competitorRow.columns.size();
                
                // new offset of loser increase by number of column value comparisons required to determine winner/loser
                competitorRow.offset += comparisonsToDetermineLoser;

                // handles out of bounds (if it occurs)
                if(competitorRow.offset > row_len) {
                    competitorRow.offset = row_len;
                    competitorRow.offsetValue = competitorRow.columns.back();
                } else {
                    // new offsetValue is value at new offset
                    competitorRow.offsetValue = competitorRow.columns[competitorRow.offset];
                }
            }

            competitorRunIndex = currLoserTreeNodeRunIndex;
        } else {
            // currRow - Loser
            // competitorRow - Winner

            // if Row in current LoserTreeNode lost again then we just update OVC if required
            if(comparisonsToDetermineLoser > 0) {
                int row_len = currRow.columns.size();
                
                // new offset of loser increase by number of column value comparisons required to determine winner/loser
                currRow.offset += comparisonsToDetermineLoser;

                // handles out of bounds (if it occurs)
                if(currRow.offset > row_len) {
                    currRow.offset = row_len;
                    currRow.offsetValue = currRow.columns.back();
                } else {
                    // new offsetValue is value at new offset
                    currRow.offsetValue = currRow.columns[currRow.offset];
                }
            }

        }

        // move up to parent node in the LoserTree
        currLoserTreeNodeIndex /= 2;
    }

    // Set Overall Winner in the Root of the LoserTree
    loserTree[currLoserTreeNodeIndex].runIndex = competitorRunIndex;
}
