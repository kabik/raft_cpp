#ifndef CONSTANT_H
#define CONSTANT_H

// hostname
#define HOSTNAME_LENGTH                100

// communication related
#define MESSAGE_SIZE                   2048

// RPC's message length related
#define ENTRIES_STR_LENGTH             1024
#define ENTRY_STR_LENGTH               32
#define COMMAND_STR_LENGTH             22
#define COMMAND_KIND_LENGTH            3
#define COMMIT_MESSAGE_LENGTH          32
#define KEY_LENGTH                     8
#define VALUE_LENGTH                   8

const static char ENTRIES_DELIMITER = '/';
const static char ENTRY_DELIMITER   = ':';
const static char COMMAND_DELIMITER = ',';

// timeout related
#define MIN_TIMEOUTTIME_MICROSECONDS   500000  // 0.5   sec
#define MAX_TIMEOUTTIME_MICROSECONDS   750000  // 0.75  sec

// append entries rpc related
#define HEARTBEAT_INTERVAL             50000   // 0.05 sec

#endif //CONSTANT_H
