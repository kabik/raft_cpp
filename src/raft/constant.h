#ifndef CONSTANT_H
#define CONSTANT_H

// communication related
#define BUFFER_SIZE                    2048

// RPC's message length related
#define APPEND_ENTRIES_RPC_LENGTH      1024
#define REQUEST_VOTE_RPC_LENGTH        2048
#define RESPONSE_APPEND_ENTRIES_LENGTH 16
#define RESPONSE_REQUEST_VOTE_LENGTH   16
#define ENTRIES_STR_LENGTH             1024
#define ENTRY_STR_LENGTH               32
#define COMMAND_STR_LENGTH             22

// timeout related
#define MIN_TIMEOUTTIME_MICROSECONDS   250000  // 0.25 sec
#define MAX_TIMEOUTTIME_MICROSECONDS   500000  // 0.5  sec

// append entries rpc related
#define HEARTBEAT_INTERVAL             50000   // 0.05 sec

#endif //CONSTANT_H
