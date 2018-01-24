#include <stdio.h>

#include "raft.h"
#include "config.h"
#include "state.h"

Raft::Raft() {}

void Raft::createConfig(char* configFileName) {
    this->config = new Config(configFileName);
}

Config* Raft::getConfig() {
    return this->config;
}
