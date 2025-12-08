CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -I$(UTILS_DIR) -I$(LIBS_DIR)
CXXFLAGS = -Wall -Wextra -std=c++11 -I$(UTILS_DIR) -I$(LIBS_DIR)
LDFLAGS =
LDFLAGS_SQLITE = -lsqlite3

# Directories
LIBS_DIR = libs
SERVER_DIR = src/server
CLIENT_DIR = src/client
UTILS_DIR = src/utils

# Source files
SERVER_SRC = $(SERVER_DIR)/server.cpp
CLIENT_SRC = $(CLIENT_DIR)/client.cpp
PROTOCOL_SRC = $(UTILS_DIR)/protocol.cpp
DB_MANAGER_SRC = $(SERVER_DIR)/db_manager.cpp
FRIEND_SERVICE = $(SERVER_DIR)/friend_service.cpp

TEST_SRC= $(SERVER_DIR)/test.c
TEST_JSON_SRC= $(SERVER_DIR)/test_json.cpp
TEST_PROTOCOL_SRC= $(SERVER_DIR)/test_protocol.cpp
TEST_PIPELINED_SRC= $(SERVER_DIR)/test_pipelined.cpp
INIT_DB_SRC= $(SERVER_DIR)/init_db.cpp

# Output executables
SERVER_BIN = server
CLIENT_BIN = client

# Default target - build both
all: server client

# Build server
server: $(SERVER_SRC) $(PROTOCOL_SRC) $(DB_MANAGER_SRC) $(FRIEND_SERVICE)
	$(CXX) $(CXXFLAGS) -pthread $(SERVER_SRC) $(PROTOCOL_SRC) $(DB_MANAGER_SRC) $(FRIEND_SERVICE) -o $(SERVER_BIN) $(LDFLAGS_SQLITE)

# Build client
client: $(CLIENT_SRC) $(PROTOCOL_SRC)
	$(CXX) $(CXXFLAGS) -pthread $(CLIENT_SRC) $(PROTOCOL_SRC) -o $(CLIENT_BIN) $(LDFLAGS)

# Clean compiled files
clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN) test test_json test_protocol test_pipelined init_db
	rm -f $(SERVER_DIR)/*.o $(CLIENT_DIR)/*.o

# Clean and rebuild
rebuild: clean all

test: $(TEST_SRC)
	$(CC) -o test $(TEST_SRC) $(LDFLAGS_SQLITE)

test-json: $(TEST_JSON_SRC)
	$(CXX) $(CXXFLAGS) -o test_json $(TEST_JSON_SRC) $(LDFLAGS_SQLITE)

test-protocol: $(TEST_PROTOCOL_SRC) $(PROTOCOL_SRC)
	$(CXX) $(CXXFLAGS) -pthread -o test_protocol $(TEST_PROTOCOL_SRC) $(PROTOCOL_SRC)

test-pipelined: $(TEST_PIPELINED_SRC) $(PROTOCOL_SRC)
	$(CXX) $(CXXFLAGS) -pthread -o test_pipelined $(TEST_PIPELINED_SRC) $(PROTOCOL_SRC)

init-db: $(INIT_DB_SRC)
	$(CXX) $(CXXFLAGS) -o init_db $(INIT_DB_SRC) $(LDFLAGS_SQLITE)

# Run server (example)
run-server: server
	./$(SERVER_BIN) 5550 storage

# Run client (example)
run-client: client
	./$(CLIENT_BIN) 127.0.0.1 5550

.PHONY: all server client clean rebuild run-server run-client test test-json test-protocol test-pipelined init-db
