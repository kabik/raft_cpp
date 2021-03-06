#ifndef RPC_CC
#define RPC_CC

#include <stdio.h>
#include <string.h> // for memcpy

#include "constant.h"

enum RPCKind {
	RPC_KIND_APPEND_ENTRIES             = 0,
	RPC_KIND_REQUEST_VOTE               = 1,
	RPC_KIND_RESPONSE_APPEND_ENTRIES    = 2,
	RPC_KIND_RESPONSE_REQUEST_VOTE      = 3,
	RPC_KIND_REQUEST_LOCATION           = 4,
	RPC_KIND_RESPONSE_REQUEST_LOCATION  = 5,
	RPC_KIND_CLIENT_COMMAND             = 6,
	RPC_KIND_COMMIT_MESSAGE             = 7,
};

struct StrRPCKind : public std::string {
	StrRPCKind(RPCKind r) {
		switch(r) {
		break; case RPC_KIND_APPEND_ENTRIES            : { assign("RPC_KIND_APPEND_ENTRIES"           ); }
		break; case RPC_KIND_REQUEST_VOTE              : { assign("RPC_KIND_REQUEST_VOTE"             ); }
		break; case RPC_KIND_RESPONSE_APPEND_ENTRIES   : { assign("RPC_KIND_RESPONSE_APPEND_ENTRIES"  ); }
		break; case RPC_KIND_RESPONSE_REQUEST_VOTE     : { assign("RPC_KIND_RESPONSE_REQUEST_VOTE"    ); }
		break; case RPC_KIND_REQUEST_LOCATION          : { assign("RPC_KIND_REQUEST_LOCATION"         ); }
		break; case RPC_KIND_RESPONSE_REQUEST_LOCATION : { assign("RPC_KIND_RESPONSE_REQUEST_LOCATION"); }
		break; case RPC_KIND_CLIENT_COMMAND            : { assign("RPC_KIND_CLIENT_COMMAND"           ); }
		break; case RPC_KIND_COMMIT_MESSAGE            : { assign("RPC_KIND_COMMIT_MESSAGE"           ); }
		break; default                                 : { assign("ILLEGAL RPC KIND"                  ); }
		}
	}
};

RPCKind discernRPC(char* str) {
	char c = str[0];
	if (c >= '0' && c <= '9') {
		return (RPCKind)(c - '0');
	}
	return (RPCKind)-1;
}


// append_entries_rpc
typedef struct _append_entries_rpc {
	RPCKind rpcKind;
	int term;         // leader's term
	int leaderId;
	int prevLogIndex;
	int prevLogTerm;
	int leaderCommit; // leader's commitIndex
	int rpcId;
	bool isRequestRead;
	char entries[ENTRIES_STR_LENGTH];
} append_entries_rpc;

void arpcByFields(
	append_entries_rpc* arpc,
	int term,
	int leaderId,
	int prevLogIndex,
	int prevLogTerm,
	int leaderCommit,
	int rpcId,
	bool isRequestRead,
	char entries[ENTRIES_STR_LENGTH]
) {
	arpc->rpcKind       = RPC_KIND_APPEND_ENTRIES;
	arpc->term          = term;
	arpc->leaderId      = leaderId;
	arpc->prevLogIndex  = prevLogIndex;
	arpc->prevLogTerm   = prevLogTerm;
	arpc->leaderCommit  = leaderCommit;
	arpc->rpcId         = rpcId;
	arpc->isRequestRead = isRequestRead;
	memcpy(arpc->entries, entries, ENTRIES_STR_LENGTH);
}
void str2arpc(char str[MESSAGE_SIZE], append_entries_rpc* arpc) {
	sscanf(
		str,
		"%d %d %d %d %d %d %d %d %s",
		&arpc->rpcKind,
		&arpc->term,
		&arpc->leaderId,
		&arpc->prevLogIndex,
		&arpc->prevLogTerm,
		&arpc->leaderCommit,
		&arpc->rpcId,
		&arpc->isRequestRead,
		&arpc->entries
	);
}
void arpc2str(append_entries_rpc* arpc, char str[MESSAGE_SIZE]) {
	sprintf(
		str,
		"%d %d %d %d %d %d %d %d %s",
		arpc->rpcKind,
		arpc->term,
		arpc->leaderId,
		arpc->prevLogIndex,
		arpc->prevLogTerm,
		arpc->leaderCommit,
		arpc->rpcId,
		arpc->isRequestRead,
		arpc->entries
	);
}


// request_vote_rpc
typedef struct _request_vote_rpc {
	RPCKind rpcKind;
	int term;         // candidate's term
	int candidateId;
	int lastLogIndex;
	int lastLogTerm;
} request_vote_rpc;

void rrpcByFields(
	request_vote_rpc* rrpc,
	int term,
	int candidateId,
	int lastLogIndex,
	int lastLogTerm
) {
	rrpc->rpcKind       = RPC_KIND_REQUEST_VOTE;
	rrpc->term          = term;
	rrpc->candidateId   = candidateId;
	rrpc->lastLogIndex  = lastLogIndex;
	rrpc->lastLogTerm   = lastLogTerm;
}
void str2rrpc(char str[MESSAGE_SIZE], request_vote_rpc* rrpc) {
	sscanf(
		str,
		"%d %d %d %d %d",
		&rrpc->rpcKind,
		&rrpc->term,
		&rrpc->candidateId,
		&rrpc->lastLogIndex,
		&rrpc->lastLogTerm
	);
}
void rrpc2str(request_vote_rpc* rrpc, char str[MESSAGE_SIZE]) {
	sprintf(
		str,
		"%d %d %d %d %d",
		rrpc->rpcKind,
		rrpc->term,
		rrpc->candidateId,
		rrpc->lastLogIndex,
		rrpc->lastLogTerm
	);
}


// response_append_entries
typedef struct _response_append_entries {
	RPCKind rpcKind;
	int term;
	int rpcId;
	bool isRequestRead;
	bool success;
} response_append_entries;

void raeByFields(
	response_append_entries* rae,
	int term,
	int rpcId,
	bool isRequestRead,
	bool success
) {
	rae->rpcKind       = RPC_KIND_RESPONSE_APPEND_ENTRIES;
	rae->term          = term;
	rae->rpcId         = rpcId;
	rae->isRequestRead = isRequestRead;
	rae->success       = success;
}
void str2rae(char str[MESSAGE_SIZE], response_append_entries* rae) {
	sscanf(
		str,
		"%d %d %d %d %d",
		&rae->rpcKind,
		&rae->term,
		&rae->rpcId,
		&rae->isRequestRead,
		&rae->success
	);
}
void rae2str(response_append_entries* rae, char str[MESSAGE_SIZE]) {
	sprintf(
		str,
		"%d %d %d %d %d",
		rae->rpcKind,
		rae->term,
		rae->rpcId,
		rae->isRequestRead,
		rae->success
	);
}


// response_request_vote
typedef struct _response_request_vote {
	RPCKind rpcKind;
	int term;
	bool success;
} response_request_vote;

void rrvByFields(
	response_request_vote* rrv,
	int term,
	bool success
) {
	rrv->rpcKind = RPC_KIND_RESPONSE_REQUEST_VOTE;
	rrv->term    = term;
	rrv->success = success;
}
void str2rrv(char str[MESSAGE_SIZE], response_request_vote* rrv) {
	sscanf(
		str,
		"%d %d %d",
		&rrv->rpcKind,
		&rrv->term,
		&rrv->success
	);
}
void rrv2str(response_request_vote* rrv, char str[MESSAGE_SIZE]) {
	sprintf(
		str,
		"%d %d %d",
		rrv->rpcKind,
		rrv->term,
		rrv->success
	);
}


// request_location
typedef struct _request_location {
	RPCKind rpcKind;
} request_location;

void rlByFields(
	request_location* rl
) {
	rl->rpcKind = RPC_KIND_REQUEST_LOCATION;
}
void str2rl(char str[MESSAGE_SIZE], request_location* rl) {
	sscanf(
		str,
		"%d",
		&rl->rpcKind
	);
}
void rl2str(request_location* rl, char str[MESSAGE_SIZE]) {
	sprintf(
		str,
		"%d",
		rl->rpcKind
	);
}


// response_request_location
typedef struct _response_request_location {
	RPCKind rpcKind;
	char hostname[HOSTNAME_LENGTH];
	int port;
} response_request_location;

void rrlByFields(
	response_request_location* rrl,
	const char hostname[HOSTNAME_LENGTH],
	int port
) {
	rrl->rpcKind = RPC_KIND_RESPONSE_REQUEST_LOCATION;
	memcpy(rrl->hostname, hostname, HOSTNAME_LENGTH);
	rrl->port = port;
}
void str2rrl(char str[MESSAGE_SIZE], response_request_location* rrl) {
	sscanf(
		str,
		"%d %s %d",
		&rrl->rpcKind,
		&rrl->hostname,
		&rrl->port
	);
}
void rrl2str(response_request_location* rrl, char str[MESSAGE_SIZE]) {
	sprintf(
		str,
		"%d %s %d",
		rrl->rpcKind,
		rrl->hostname,
		rrl->port
	);
}


// client_command
typedef struct _client_command {
	RPCKind rpcKind;
	int commandId;
	char command[COMMAND_STR_LENGTH];
} client_command;

void ccByFields(
	client_command* cc,
	int commandId,
	const char command[COMMAND_STR_LENGTH]
) {
	cc->rpcKind = RPC_KIND_CLIENT_COMMAND;
	cc->commandId = commandId;
	memcpy(cc->command, command, COMMAND_STR_LENGTH);
}
void str2cc(char str[MESSAGE_SIZE], client_command* cc) {
	sscanf(
		str,
		"%d %d %s",
		&cc->rpcKind,
		&cc->commandId,
		&cc->command
	);
}
void cc2str(client_command* cc, char str[MESSAGE_SIZE]) {
	sprintf(
		str,
		"%d %d %s",
		cc->rpcKind,
		cc->commandId,
		cc->command
	);
}


// commit_message
typedef struct _commit_message {
	RPCKind rpcKind;
	int commandId;
} commit_message;

void cmByFields(
	commit_message* cm,
	int commandId
) {
	cm->rpcKind = RPC_KIND_COMMIT_MESSAGE;
	cm->commandId = commandId;
}
void str2cm(char str[MESSAGE_SIZE], commit_message* cm) {
	sscanf(
		str,
		"%d %d",
		&cm->rpcKind,
		&cm->commandId
	);
}
void cm2str(commit_message* cm, char str[MESSAGE_SIZE]) {
	sprintf(
		str,
		"%d %d",
		cm->rpcKind,
		cm->commandId
	);
}


#endif //RPC_CC
