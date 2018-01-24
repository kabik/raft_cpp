#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include <string>

using namespace std;

typedef struct _node {
    string hostname;
    int port;
} node_t;

class Config {
private:
    string configFileName;
    int nNodes;
    vector<node_t* > nodes;

public:
    Config(char* configFileName);
    int getNumberOfNodes();
    void setNumberOfNodes(int nNodes);
    vector<node_t*> getNodes();
    void setNodes(vector<node_t*> nodes);
};

#include "config.cc"
#endif //CONFIG_H
