#include <vector>
#include <string>
class Row;

class LoserTreeNode {

public:
    int runIndex;  // Index of the run this node represents

    LoserTreeNode(int runIndex);
    ~LoserTreeNode();
};

class TreeOfLosers {
private:
    std::vector<std::vector<Row> > sortedRuns;  // Input sorted runs
    std::vector<LoserTreeNode> loserTree;      // Tree of losers
    std::vector<int> currentIndices;           // Current index in each run

    int loserTreeHeight;
    int numOfLoserNodes;
    int numOfRuns;
    int treeSize;

    Row& getRow(int runIndex);
    int init(int currNodeIdx);
    void updateTree(int competitorRunIndex);

public:
    TreeOfLosers(const std::vector<std::vector<Row> > sortedRuns);
    void initializeTree();
    Row getNextRow();
};
