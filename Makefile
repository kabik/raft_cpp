CC := g++
CFLAGS := -std=c++11 -pthread -g -Wall -Wextra -fdiagnostics-color=always
BIN_DIR := bin
L := main
RM := rm -ri

all: $(L)

%: src/%.cc
	@if [ ! -d $(BIN_DIR) ]; \
		then echo "mkdir -p $(BIN_DIR)"; mkdir -p $(BIN_DIR); \
		fi
	$(CC) $(CFLAGS) -O $< -o $(BIN_DIR)/$@

test: test/test.cc
	@if [ ! -d $(BIN_DIR) ]; \
		then echo "mkdir -p $(BIN_DIR)"; mkdir -p $(BIN_DIR); \
		fi
	$(CC) $(CFLAGS) -O $< -o $(BIN_DIR)/$@

clean:
	$(RM) $(BIN_DIR)
