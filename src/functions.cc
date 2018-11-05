#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <vector>
#include <string>
#include <string.h>
#include <sstream>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <random>

/* === rsocket === */
#ifdef ENABLE_RSOCKET
#include <rdma/rsocket.h>

// rsocket functions
#define S_SOCKET(f, t, p)            rsocket(f, t, p)
#define S_CLOSE(f)                   rclose(f)
#define S_SEND(s, b, l, f)           rsend(s, b, l, f)
#define S_RECV(s, b, l, f)           rrecv(s, b, l, f)
#define S_ADDRINFO                   rdma_addrinfo
#define S_GETADDRINFO(a, p, h, r)    rdma_getaddrinfo(a, p, h, r)
#define S_FREEADDRINFO(r)            rdma_freeaddrinfo(r)
#define S_SETSOCKOPT(s, l, n, v, ol) rsetsockopt(s, l, n, v, ol)

// server-specific
#define S_ACCEPT(s, a, l)       raccept(s, a, l)
#define S_BIND(s, a, l)         rbind(s, a, l)
#define S_GETSOCKNAME(s, n, l)  rgetsockname(s, n, l)
#define S_LISTEN(s, b)          rlisten(s, b)
#define S_SRC_ADDR(a)           a->ai_src_addr
#define S_SRC_ADDRLEN(a)        a->ai_src_len
// client-specific
#define S_CONNECT(s, a, l)           rconnect(s, a, l)
#define S_DST_ADDR(a)                a->ai_dst_addr
#define S_DST_ADDRLEN(a)             a->ai_dst_len

#else

// BSD-socket functions
#define S_SOCKET(f, t, p)            ::socket(f, t, p)
#define S_CLOSE(f)                   close(f)
#define S_SEND(s, b, l, f)           send(s, b, l, f)
#define S_RECV(s, b, l, f)           recv(s, b, l, f)
#define S_ADDRINFO                   addrinfo
#define S_GETADDRINFO(a, p, h, r)    getaddrinfo(a, p, h, r)
#define S_FREEADDRINFO(r)            freeaddrinfo(r)
#define S_SETSOCKOPT(s, l, n, v, ol) setsockopt(s, l, n, v, ol)

// server-specific
#define S_ACCEPT(s, a, l)       accept(s, a, l)
#define S_BIND(s, a, l)         bind(s, a, l)
#define S_GETSOCKNAME(s, n, l)  getsockname(s, n, l)
#define S_LISTEN(s, b)          listen(s, b)
#define S_SRC_ADDR(a)           a->ai_addr
#define S_SRC_ADDRLEN(a)        a->ai_addrlen
// client-specific
#define S_CONNECT(s, a, l)           connect(s, a, l)
#define S_DST_ADDR(a)                a->ai_addr
#define S_DST_ADDRLEN(a)             a->ai_addrlen

#endif // ENABLE_RSOCKET
/* === rsocket === */

#define STR(var) #var

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::stringstream;

// devide a string by a certain letter
vector<string> split(const string &s, char delim) {
	vector<string> elems;
	stringstream ss(s);
	string item;
	while (getline(ss, item, delim)) {
		if (!item.empty()) {
			elems.push_back(item);
		}
	}
	return elems;
}

// mkdir
static int stat_mkdir(const char *filepath, mode_t mode) {
	struct stat sb = {0};
	int rc = 0;

	rc = stat(filepath, &sb);
	if (rc == 0) {
		if (!S_ISDIR(sb.st_mode)) {
			cerr << "Error: Not a directory: " << *filepath << endl;
			return -1;
		}
		return 0;
	}

	rc = mkdir(filepath, mode);
	if (rc < 0) {
		cerr << "Error: mkdir(: " << errno << ") " << strerror(errno) << ": " << *filepath << endl;
		return -1;
	}

	return 0;
}

static int mkdir_path(const char *filepath, mode_t mode) {
	char *p = NULL;
	char *buf = NULL;
	int rc = 0;

	buf = (char *)malloc(strlen(filepath) + 4);
	if (buf == NULL) {
		cerr << "Error: malloc(" << errno << ") " << strerror(errno) << endl;
		return -1;
	}
	strcpy(buf, filepath);

	for (p = strchr(buf+1, '/'); p; p = strchr(p+1, '/')) {
		*p = '\0';
		rc = stat_mkdir(buf, mode);
		if (rc != 0) {
			free(buf);
			return -1;
		}
		*p = '/';
	}

	free(buf);
	return 0;
}

static int mymkdir(const char *filepath) {
	int rc = mkdir_path(filepath, 0755);
	return rc;
}

static int myrand(int min, int max) {
	std::random_device rnd;
	return rnd() % (max - min) + min;
}

#endif //FUNCTIONS_H
