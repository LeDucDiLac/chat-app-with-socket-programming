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
DATABASE_DIR = database
TEST_DIR = test

# Source files
SERVER_SRC = $(SERVER_DIR)/server.cpp
CLIENT_SRC = $(CLIENT_DIR)/client.cpp
PROTOCOL_SRC = $(UTILS_DIR)/protocol.cpp
ACCOUNT_SERVICE = $(SERVER_DIR)/account_service.cpp
FRIEND_SERVICE = $(SERVER_DIR)/friend_service.cpp
GROUP_SERVICE = $(SERVER_DIR)/group_service.cpp
DATABASE_RESET = $(DATABASE_DIR)/reset_db.cpp # Vẫn giữ khai báo này

TEST_JSON_SRC= $(TEST_DIR)/test_json.cpp
TEST_PROTOCOL_SRC= $(TEST_DIR)/test_protocol.cpp
TEST_PIPELINED_SRC= $(TEST_DIR)/test_pipelined.cpp
TEST_FRIENDS_SRC= $(TEST_DIR)/test_friends.cpp
TEST_GROUPS_SRC= $(TEST_DIR)/test_groups.cpp
TEST_COMPLETE_SRC= $(TEST_DIR)/test_complete.cpp

# Output executables
SERVER_BIN = server
CLIENT_BIN = client

# Default target - build both
all: server client

# Build server
server: $(SERVER_SRC) $(PROTOCOL_SRC) $(ACCOUNT_SERVICE) $(FRIEND_SERVICE) $(GROUP_SERVICE)
	$(CXX) $(CXXFLAGS) -pthread $(SERVER_SRC) $(PROTOCOL_SRC) $(ACCOUNT_SERVICE) $(FRIEND_SERVICE) $(GROUP_SERVICE) -o $(SERVER_BIN) $(LDFLAGS_SQLITE)

# Build client
client: $(CLIENT_SRC) $(PROTOCOL_SRC)
	$(CXX) $(CXXFLAGS) -pthread $(CLIENT_SRC) $(PROTOCOL_SRC) -o $(CLIENT_BIN) $(LDFLAGS)

# Clean compiled files
clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN)  test_json test_protocol test_pipelined init_db
	rm -f test_friends test_groups test_complete
	rm -f $(SERVER_DIR)/*.o $(CLIENT_DIR)/*.o
	rm -f reset-db reset-db.* # Cập nhật: thêm lệnh xóa reset-db

# Clean and rebuild
rebuild: clean all


test-json: $(TEST_JSON_SRC)
	$(CXX) $(CXXFLAGS) -o test_json $(TEST_JSON_SRC) $(LDFLAGS_SQLITE)

test-protocol: $(TEST_PROTOCOL_SRC) $(PROTOCOL_SRC)
	$(CXX) $(CXXFLAGS) -pthread -o test_protocol $(TEST_PROTOCOL_SRC) $(PROTOCOL_SRC)

test-pipelined: $(TEST_PIPELINED_SRC) $(PROTOCOL_SRC)
	$(CXX) $(CXXFLAGS) -pthread -o test_pipelined $(TEST_PIPELINED_SRC) $(PROTOCOL_SRC)

test-friends: $(TEST_FRIENDS_SRC) $(PROTOCOL_SRC)
	$(CXX) $(CXXFLAGS) -pthread -o test_friends $(TEST_FRIENDS_SRC) $(PROTOCOL_SRC)

test-groups: $(TEST_GROUPS_SRC) $(PROTOCOL_SRC)
	$(CXX) $(CXXFLAGS) -pthread -o test_groups $(TEST_GROUPS_SRC) $(PROTOCOL_SRC)

test-complete: $(TEST_COMPLETE_SRC) $(PROTOCOL_SRC)
	$(CXX) $(CXXFLAGS) -pthread -o test_complete $(TEST_COMPLETE_SRC) $(PROTOCOL_SRC)

# Build all tests
build-tests: test-friends test-groups test-complete
	@echo "All test executables built successfully!"

init-db: $(INIT_DB_SRC)
	$(CXX) $(CXXFLAGS) -o init_db $(INIT_DB_SRC) $(LDFLAGS_SQLITE)

# Run server (example)
run-server: server
	./$(SERVER_BIN) 5550 storage

# Run client (example)
run-client: client
	./$(CLIENT_BIN) 127.0.0.1 5550

# =======================================================
# LỆNH BIÊN DỊCH VÀ CHẠY reset_db.cpp
# =======================================================

# Luật biên dịch tường minh cho reset_db.cpp (Không bắt buộc nếu chỉ có 1 file)
# $(DATABASE_DIR)/reset_db.o: $(DATABASE_RESET)
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# Target để biên dịch và liên kết (tạo executable)
reset-db: $(DATABASE_RESET)
	$(CXX) $(CXXFLAGS) -o reset-db $(DATABASE_RESET) $(LDFLAGS_SQLITE)

.PHONY: all server client clean rebuild run-server run-client test test-json test-protocol test-pipelined test-friends test-groups test-complete build-tests init-db reset-db # Thêm reset-db vào .PHONY
