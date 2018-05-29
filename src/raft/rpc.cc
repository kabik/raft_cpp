#ifndef RPC_CC
#define RPC_CC

#include <stdio.h>

#include "constant.h"

enum RPCKind {
	RPC_KIND_APPEND_ENTRIES             = 0,
	RPC_KIND_REQUEST_VOTE               = 1,
	RPC_KIND_RESPONSE_APPEND_ENTRIES    = 2,
	RPC_KIND_RESPONSE_REQUEST_VOTE      = 3,
	RPC_KIND_REQUEST_LOCATION           = 4,
	RPC_KIND_RESPONSE_REQUEST_LOCATION  = 5,
	RPC_KIND_CLIENT_COMMAND             = 6,
	RPC_KIND_COMMIT                     = 7,
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
		break; case RPC_KIND_COMMIT                    : { assign("RPC_KIND_COMMIT"                   ); }
		break; default                                 : { assign("ILLEGAL RPC KIND"                ); }
		}
	}
};

typedef struct _append_entries_rpc {
	RPCKind rpcKind;
	int term;         // leader's term
	int leaderId;
	int prevLogIndex;
	int prevLogTerm;
	int leaderCommit; // leader's commitIndex
	char entries[ENTRIES_STR_LENGTH];
} append_entries_rpc;

typedef struct _request_vote_rpc {
	RPCKind rpcKind;
	int term;         // candidate's term
	int candidateId;
	int lastLogIndex;
	int lastLogTerm;
} request_vote_rpc;

typedef struct _response_append_entries {
	RPCKind rpcKind;
	int term;
	bool success;
} response_append_entries;

typedef struct _response_request_vote {
	RPCKind rpcKind;
	int term;
	bool success;
} response_request_vote;

RPCKind discernRPC(char* str) {
	char c = str[0];
	if (c >= '0' && c <= '9') {
		return (RPCKind)(c - '0');
	}
	return (RPCKind)-1;
}

// create append_entries_rpc by fields
void arpcByFields(
	append_entries_rpc* arpc,
	int term,
	int leaderId,
	int prevLogIndex,
	int prevLogTerm,
	int leaderCommit,
	char entries[ENTRIES_STR_LENGTH]
) {
	arpc->rpcKind       = RPC_KIND_APPEND_ENTRIES;
	arpc->term          = term;
	arpc->leaderId      = leaderId;
	arpc->prevLogIndex  = prevLogIndex;
	arpc->prevLogTerm   = prevLogTerm;
	arpc->leaderCommit  = leaderCommit;
	memcpy(arpc->entries, entries, ENTRIES_STR_LENGTH);
}

// create request_vote_rpc by fields
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

// create response_append_entries by fields
void raeByFields(
	response_append_entries* rae,
	int term,
	bool success
) {
	rae->rpcKind = RPC_KIND_RESPONSE_APPEND_ENTRIES;
	rae->term    = term;
	rae->success = success;
}

// create response_request_vote by fields
void rrvByFields(
	response_request_vote* rrv,
	int term,
	bool success
) {
	rrv->rpcKind = RPC_KIND_RESPONSE_REQUEST_VOTE;
	rrv->term    = term;
	rrv->success = success;
}

// char[] to append_entries_rpc
void str2arpc(char str[RESPONSE_APPEND_ENTRIES_LENGTH], append_entries_rpc* arpc) {
	sscanf(
		str,
		"%d %d %d %d %d %d %s",
		&arpc->rpcKind,
		&arpc->term,
		&arpc->leaderId,
		&arpc->prevLogIndex,
		&arpc->prevLogTerm,
		&arpc->leaderCommit,
		&arpc->entries
	);
}

// char[] to request_vote_rpc
void str2rrpc(char str[REQUEST_VOTE_RPC_LENGTH], request_vote_rpc* rrpc) {
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

// char[] to response_append_entries
void str2rae(char str[RESPONSE_APPEND_ENTRIES_LENGTH], response_append_entries* rae) {
	sscanf(
		str,
		"%d %d %d",
		&rae->rpcKind,
		&rae->term,
		&rae->success
	);
}

// char[] to response_request_vote
void str2rrv(char str[RESPONSE_REQUEST_VOTE_LENGTH], response_request_vote* rrv) {
	sscanf(
		str,
		"%d %d %d",
		&rrv->rpcKind,
		&rrv->term,
		&rrv->success
	);
}

// append_entries_rpc to char[]
void arpc2str(append_entries_rpc* arpc, char str[APPEND_ENTRIES_RPC_LENGTH]) {
	sprintf(
		str,
		"%d %d %d %d %d %d %s",
		arpc->rpcKind,
		arpc->term,
		arpc->leaderId,
		arpc->prevLogIndex,
		arpc->prevLogTerm,
		arpc->leaderCommit,
		arpc->entries
	);
}

// request_vote_rpc to char[]
void rrpc2str(request_vote_rpc* rrpc, char str[REQUEST_VOTE_RPC_LENGTH]) {
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

// response_append_entries to char[]
void rae2str(response_append_entries* rae, char str[RESPONSE_APPEND_ENTRIES_LENGTH]) {
	sprintf(
		str,
		"%d %d %d",
		rae->rpcKind,
		rae->term,
		rae->success
	);
}

// response_request_vote to char[]
void rrv2str(response_request_vote* rrv, char str[RESPONSE_REQUEST_VOTE_LENGTH]) {
	sprintf(
		str,
		"%d %d %d",
		rrv->rpcKind,
		rrv->term,
		rrv->success
	);
}

#endif //RPC_CC
