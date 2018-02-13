#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include <string>

using std::string;
using std::vector;

typedef struct _node_conf {
	string* hostname;
	int port;
} node_conf;

class Config {
private:
	string configFileName;
	int nNodes;
	vector<node_conf* > nodes;

public:
	Config(char* configFileName);
	int getNumberOfNodes();
	void setNumberOfNodes(int nNodes);
	vector<node_conf*> getNodes();
	void setNodes(vector<node_conf*> nodes);
};

#include "config.cc"
#endif //CONFIG_H
