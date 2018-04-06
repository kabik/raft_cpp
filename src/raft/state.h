#ifndef STATE_H
#define STATE_H

#define STR(var) #var

enum State {
	FOLLOWER  = 0,
	CANDIDATE = 1,
	LEADER    = 2,
};

struct StrState : public string {
	StrState(State e) {
		switch(e) {
		break; case FOLLOWER  : { assign("FOLLOWER"     ); }
		break; case CANDIDATE : { assign("CANDIDATE"    ); }
		break; case LEADER    : { assign("LEADER"       ); }
		break; default        : { assign("illegal state"); }
		}
	}
};

#endif //STATE_H
