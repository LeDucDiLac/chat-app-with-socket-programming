CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -I$(SERVER_DIR) -Ilibs
CXXFLAGS = -Wall -Wextra -std=c++11 -I$(SERVER_DIR) -Ilibs
LDFLAGS =
LDFLAGS_SQLITE = -lsqlite3

# Directories
SERVER_DIR = src/TCP_Server
CLIENT_DIR = src/TCP_Client

# Source files
SERVER_SRC = $(SERVER_DIR)/server.c
CLIENT_SRC = $(CLIENT_DIR)/client.c
UTILS_SRC = $(SERVER_DIR)/tcp_utils.c

TEST_SRC= $(SERVER_DIR)/test.c
TEST_JSON_SRC= $(SERVER_DIR)/test_json.cpp

# Output executables
SERVER_BIN = server
CLIENT_BIN = client

# Default target - build both
all: server client

# Build server
server: $(SERVER_SRC) $(UTILS_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) $(UTILS_SRC) -o $(SERVER_BIN) $(LDFLAGS)

# Build client
client: $(CLIENT_SRC) $(UTILS_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) $(UTILS_SRC) -o $(CLIENT_BIN) $(LDFLAGS)

# Clean compiled files
clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN) test test_json
	rm -f $(SERVER_DIR)/*.o $(CLIENT_DIR)/*.o

# Clean and rebuild
rebuild: clean all

test: $(TEST_SRC)
	$(CC) -o test $(TEST_SRC) $(LDFLAGS_SQLITE)

test-json: $(TEST_JSON_SRC)
	$(CXX) $(CXXFLAGS) -o test_json $(TEST_JSON_SRC) $(LDFLAGS_SQLITE)

# Run server (example)
run-server: server
	./$(SERVER_BIN) 5550 storage

# Run client (example)
run-client: client
	./$(CLIENT_BIN) 127.0.0.1 5550

.PHONY: all server client clean rebuild run-server run-client test test-json