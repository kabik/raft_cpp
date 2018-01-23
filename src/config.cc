#include <vector>

using namespace std;

typedef struct _node {
    string nodename;
    int port;
} node_t;

class Config {
private:
    int nNodes;
    vector<node_t* > nodes;

public:
    Config() {}

    int getNumberOfNodes() {
        return nNodes;
    }
    void setNumberOfNodes(int nNodes) {
        this->nNodes = nNodes;
    }

    vector<node_t*> getNodes() {
        return nodes;
    }
    void setNodes(vector<node_t*> nodes) {
        this->nodes = nodes;
    }
};
