CC := g++
CFLAGS := -g -Wall -Wextra
OBJ_DIR := bin
L := main
RM := rm -ri

all: $(L)

%: src/%.cc
	@if [ ! -d $(OBJ_DIR) ]; \
		then echo "mkdir -p $(OBJ_DIR)"; mkdir -p $(OBJ_DIR); \
		fi
	$(CC) -O $< -o $(OBJ_DIR)/$@.o

clean:
	$(RM) $(OBJ_DIR)
