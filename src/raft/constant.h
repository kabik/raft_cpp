#ifndef CONSTANT_H
#define CONSTANT_H

// hostname
#define HOSTNAME_LENGTH                100
#define PORT_LENGTH                    6

// communication related
#define MESSAGE_SIZE                   2560

// RPC's message length related
#define MAX_NUM_OF_ENTRIES             64
#define ENTRY_STR_LENGTH               32
#define ENTRIES_STR_LENGTH             ENTRY_STR_LENGTH * MAX_NUM_OF_ENTRIES
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
#define CLIENTS_MAX                    1000

// measure
#define MEASURE_LOG_SIZE               200000

// storage latency
#define STORAGE_LATENCY                8       // μs

#endif //CONSTANT_H
