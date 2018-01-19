#include <stdio.h>
#include <fstream>
#include <sstream>

#include "config.cc"
#include "functions.cc"

using namespace std;

class Raft {
private:
    Config *config;
    void createConfig(char* configFileName);

public:
    Raft(char* configFileName);
    Config getConfig();
};
