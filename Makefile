CC := g++
CFLAGS := -std=c++11 -pthread -g -fdiagnostics-color=always
BIN_DIR := bin
L := main main_rsocket client client_rsocket
RM := rm -rf

all: $(L)

%: src/%.cc
	@if [ ! -d $(BIN_DIR) ]; then echo "mkdir -p $(BIN_DIR)"; mkdir -p $(BIN_DIR); fi
	$(CC) $(CFLAGS) -O $< -o $(BIN_DIR)/$@

main_rsocket: src/main.cc
	@if [ ! -d $(BIN_DIR) ]; then echo "mkdir -p $(BIN_DIR)"; mkdir -p $(BIN_DIR); fi
	$(CC) $(CFLAGS) -DENABLE_RSOCKET -lrdmacm -O $< -o $(BIN_DIR)/$@

client: src/client/client.cc
	@if [ ! -d $(BIN_DIR) ]; then echo "mkdir -p $(BIN_DIR)"; mkdir -p $(BIN_DIR); fi
	$(CC) $(CFLAGS) -O $< -o $(BIN_DIR)/$@

client_rsocket: src/client/client.cc
	@if [ ! -d $(BIN_DIR) ]; then echo "mkdir -p $(BIN_DIR)"; mkdir -p $(BIN_DIR); fi
	$(CC) $(CFLAGS) -DENABLE_RSOCKET -lrdmacm -O $< -o $(BIN_DIR)/$@

createInput: src/client/createInput.cc
	@if [ ! -d $(BIN_DIR) ]; then echo "mkdir -p $(BIN_DIR)"; mkdir -p $(BIN_DIR); fi
	$(CC) $(CFLAGS) -O $< -o $(BIN_DIR)/$@

test: test/test.cc
	@if [ ! -d $(BIN_DIR) ]; then echo "mkdir -p $(BIN_DIR)"; mkdir -p $(BIN_DIR); fi
	$(CC) $(CFLAGS) -O $< -o $(BIN_DIR)/$@

clean:
	$(RM) $(BIN_DIR)
