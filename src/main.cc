#include <stdio.h>
#include <iostream>

#include "raft.cc"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Please specify a config file name." << endl;
        return 0;
    }

    Raft *raft = new Raft(argv[1]);

    return 0;
}
