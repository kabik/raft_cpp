#include <vector>

using namespace std;

typedef struct _host {
    string hostname;
    int port;
} host_t;

class Config {
private:
    int nNodes;
    vector<host_t* > hosts;

public:
    Config() {}

    int getNumberOfNodes() {
        return nNodes;
    }
    void setNumberOfNodes(int nNodes) {
        this->nNodes = nNodes;
    }

    vector<host_t*> getHosts() {
        return hosts;
    }
    void setHosts(vector<host_t*> hosts) {
        this->hosts = hosts;
    }
};
