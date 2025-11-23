CC = gcc
CFLAGS = -Wall -Wextra -I$(SERVER_DIR)
LDFLAGS =

# Directories
SERVER_DIR = src/TCP_Server
CLIENT_DIR = src/TCP_Client

# Source files
SERVER_SRC = $(SERVER_DIR)/server.c
CLIENT_SRC = $(CLIENT_DIR)/client.c
UTILS_SRC = $(SERVER_DIR)/tcp_utils.c

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
	rm -f $(SERVER_BIN) $(CLIENT_BIN)
	rm -f $(SERVER_DIR)/*.o $(CLIENT_DIR)/*.o

# Clean and rebuild
rebuild: clean all

# Run server (example)
run-server: server
	./$(SERVER_BIN) 5550 storage

# Run client (example)
run-client: client
	./$(CLIENT_BIN) 127.0.0.1 5550

.PHONY: all server client clean rebuild run-server run-client