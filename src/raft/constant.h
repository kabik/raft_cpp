#ifndef CONSTANT_H
#define CONSTANT_H

// hostname
#define HOSTNAME_LENGTH                100
#define PORT_LENGTH                    6

// communication related
#define MESSAGE_SIZE                   2560

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

const static char UPDATE            = 'W';
const static char READ              = 'R';
const static char DELETE            = 'D';

// timeout related
#define MIN_TIMEOUTTIME_MICROSECONDS   1000000  // 1     sec
#define MAX_TIMEOUTTIME_MICROSECONDS   1500000  // 1.5  sec

// append entries rpc related
#define HEARTBEAT_INTERVAL             50000   // 0.05 sec
#define RPC_ID_MAX                     1000000000

// measure
#define MEASURE_LOG_SIZE               200000

#endif //CONSTANT_H
