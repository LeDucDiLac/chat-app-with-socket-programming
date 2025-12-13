#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../utils/protocol.h"
#include "../../libs/json.hpp"

using json = nlohmann::json;

#define TEST_PORT 9999
#define BUFFER_SIZE 4096

// Simple test server
void test_server()
{
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0)
    {
        perror("socket failed");
        return;
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        close(server_fd);
        return;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(TEST_PORT);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        close(server_fd);
        return;
    }

    // Listen
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        close(server_fd);
        return;
    }

    std::cout << "[SERVER] Listening on port " << TEST_PORT << std::endl;

    // Accept connection
    client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    if (client_fd < 0)
    {
        perror("accept");
        close(server_fd);
        return;
    }

    std::cout << "[SERVER] Client connected" << std::endl;

    char buffer[BUFFER_SIZE];

    // Receive JSON packet from client
    int bytes_received = receive_json_packet(client_fd, buffer, BUFFER_SIZE);
    if (bytes_received > 0)
    {
        std::cout << "[SERVER] Received JSON: " << buffer << std::endl;

        // Parse JSON
        try
        {
            json request = json::parse(buffer);
            std::cout << "[SERVER] Parsed - Type: " << request["type"] 
                      << ", Username: " << request["data"]["username"] << std::endl;

            // Create response
            json response = {
                {"type", 2000},
                {"data", {
                    {"status", 200},
                    {"message", "Login successful"},
                    {"user_id", 1},
                    {"username", request["data"]["username"]}
                }}
            };

            std::string response_str = response.dump();

            // Send response back
            if (send_json_packet(client_fd, response_str.c_str()) == 0)
            {
                std::cout << "[SERVER] Response sent" << std::endl;
            }
        }
        catch (json::exception &e)
        {
            std::cerr << "[SERVER] JSON parse error: " << e.what() << std::endl;
        }
    }

    // Wait a bit before closing
    std::this_thread::sleep_for(std::chrono::seconds(1));

    close(client_fd);
    close(server_fd);
    std::cout << "[SERVER] Closed" << std::endl;
}

// Simple test client
void test_client()
{
    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    int sock = 0;
    struct sockaddr_in serv_addr;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "[CLIENT] Socket creation error" << std::endl;
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TEST_PORT);

    // Convert IPv4 address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        std::cerr << "[CLIENT] Invalid address" << std::endl;
        close(sock);
        return;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        std::cerr << "[CLIENT] Connection failed" << std::endl;
        close(sock);
        return;
    }

    std::cout << "[CLIENT] Connected to server" << std::endl;

    // Create login request JSON
    json login_request = {
        {"type", 1001},
        {"data", {
            {"username", "alice"},
            {"password", "password123"}
        }}
    };

    std::string request_str = login_request.dump();

    // Send JSON packet
    if (send_json_packet(sock, request_str.c_str()) == 0)
    {
        std::cout << "[CLIENT] Login request sent" << std::endl;

        // Receive response
        char buffer[BUFFER_SIZE];
        int bytes_received = receive_json_packet(sock, buffer, BUFFER_SIZE);

        if (bytes_received > 0)
        {
            std::cout << "[CLIENT] Received response: " << buffer << std::endl;

            // Parse response
            try
            {
                json response = json::parse(buffer);
                std::cout << "[CLIENT] Status: " << response["data"]["status"] 
                          << ", Message: " << response["data"]["message"] << std::endl;
            }
            catch (json::exception &e)
            {
                std::cerr << "[CLIENT] JSON parse error: " << e.what() << std::endl;
            }
        }
    }

    close(sock);
    std::cout << "[CLIENT] Closed" << std::endl;
}

int main()
{
    std::cout << "=== Testing Protocol Module (Stream Handling + Socket I/O) ===\n" << std::endl;

    // Run server and client in separate threads
    std::thread server_thread(test_server);
    std::thread client_thread(test_client);

    // Wait for both to finish
    server_thread.join();
    client_thread.join();

    std::cout << "\n=== Test Complete ===" << std::endl;

    return 0;
}
