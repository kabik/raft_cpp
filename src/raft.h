#ifndef RAFT_H
#define RAFT_H

class Config;

class Raft {
private:
    Config *config;

public:
    Raft();
    void createConfig(char* configFileName);
    Config* getConfig();
};

#include "raft.cc"
#endif //RAFT_H
