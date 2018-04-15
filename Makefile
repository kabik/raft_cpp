CC := g++
CFLAGS := -std=c++11 -pthread -g -Wall -Wextra -fdiagnostics-color=always
OBJ_DIR := bin
L := main
RM := rm -ri

all: $(L)

%: src/%.cc
	@if [ ! -d $(OBJ_DIR) ]; \
		then echo "mkdir -p $(OBJ_DIR)"; mkdir -p $(OBJ_DIR); \
		fi
	$(CC) $(CFLAGS) -O $< -o $(OBJ_DIR)/$@

test: test/test.cc
	@if [ ! -d $(OBJ_DIR) ]; \
		then echo "mkdir -p $(OBJ_DIR)"; mkdir -p $(OBJ_DIR); \
		fi
	$(CC) $(CFLAGS) -O $< -o $(OBJ_DIR)/$@

clean:
	$(RM) $(OBJ_DIR)
