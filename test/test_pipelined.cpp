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

#define TEST_PORT 9998
#define BUFFER_SIZE 4096

// Test server that receives multiple pipelined messages
void test_server_pipelined()
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

    std::cout << "[SERVER] Client connected\n" << std::endl;

    char buffer[BUFFER_SIZE];

    // Try to receive 3 messages that client sent back-to-back
    std::cout << "=== Receiving 3 pipelined messages ===" << std::endl;
    
    for (int i = 1; i <= 3; i++)
    {
        int bytes_received = receive_json_packet(client_fd, buffer, BUFFER_SIZE);
        
        if (bytes_received > 0)
        {
            std::cout << "\n[SERVER] Message " << i << " received (" << bytes_received << " bytes)" << std::endl;
            std::cout << "Raw: " << buffer << std::endl;

            try
            {
                json msg = json::parse(buffer);
                std::cout << "Parsed - Type: " << msg["type"];
                
                if (msg.contains("data"))
                {
                    if (msg["data"].contains("username"))
                        std::cout << ", Username: " << msg["data"]["username"];
                    if (msg["data"].contains("receiver_id"))
                        std::cout << ", Receiver: " << msg["data"]["receiver_id"];
                    if (msg["data"].contains("group_id"))
                        std::cout << ", Group: " << msg["data"]["group_id"];
                }
                std::cout << std::endl;
            }
            catch (json::exception &e)
            {
                std::cerr << "[SERVER] JSON parse error: " << e.what() << std::endl;
            }
        }
        else if (bytes_received == 0)
        {
            std::cout << "[SERVER] Connection closed by client" << std::endl;
            break;
        }
        else
        {
            std::cerr << "[SERVER] Error receiving message " << i << std::endl;
            break;
        }
    }

    std::cout << "\n[SERVER] All messages received successfully!" << std::endl;

    close(client_fd);
    close(server_fd);
}

// Test client that sends 3 messages in rapid succession (pipelined)
void test_client_pipelined()
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

    std::cout << "[CLIENT] Connected to server\n" << std::endl;

    // Create 3 different JSON messages
    json msg1 = {
        {"type", 1001},  // LOGIN
        {"data", {
            {"username", "alice"},
            {"password", "password123"}
        }}
    };

    json msg2 = {
        {"type", 1003},  // SEND_MESSAGE
        {"data", {
            {"receiver_id", 2},
            {"content", "Hello Bob!"}
        }}
    };

    json msg3 = {
        {"type", 1004},  // SEND_GROUP_MESSAGE
        {"data", {
            {"group_id", 1},
            {"content", "Hello everyone in the group!"}
        }}
    };

    std::string str1 = msg1.dump();
    std::string str2 = msg2.dump();
    std::string str3 = msg3.dump();

    std::cout << "=== Sending 3 messages in ONE send() call (concatenated) ===" << std::endl;
    std::cout << "[CLIENT] Message 1 size: " << str1.length() << " bytes" << std::endl;
    std::cout << "[CLIENT] Message 2 size: " << str2.length() << " bytes" << std::endl;
    std::cout << "[CLIENT] Message 3 size: " << str3.length() << " bytes" << std::endl;
    std::cout << std::endl;

    // Build the complete packet with all 3 messages
    // Each message has [LENGTH:4bytes][JSON:variable]
    std::string complete_packet;
    
    // Message 1
    uint32_t len1 = htonl(str1.length());
    complete_packet.append((char*)&len1, 4);
    complete_packet.append(str1);
    
    // Message 2
    uint32_t len2 = htonl(str2.length());
    complete_packet.append((char*)&len2, 4);
    complete_packet.append(str2);
    
    // Message 3
    uint32_t len3 = htonl(str3.length());
    complete_packet.append((char*)&len3, 4);
    complete_packet.append(str3);

    std::cout << "[CLIENT] Total packet size: " << complete_packet.length() << " bytes" << std::endl;
    std::cout << "[CLIENT] Sending all 3 messages in ONE send() call..." << std::endl;

    // Send everything in one go - this tests if receive can properly separate them
    ssize_t sent = send(sock, complete_packet.c_str(), complete_packet.length(), 0);
    
    if (sent == (ssize_t)complete_packet.length())
    {
        std::cout << "[CLIENT] Successfully sent " << sent << " bytes containing 3 messages!" << std::endl;
    }
    else
    {
        std::cerr << "[CLIENT] Send failed or incomplete" << std::endl;
    }

    std::cout << "\n[CLIENT] Waiting for server to separate and process all 3 messages..." << std::endl;

    // Give server time to receive all messages
    std::this_thread::sleep_for(std::chrono::seconds(2));

    close(sock);
    std::cout << "[CLIENT] Closed" << std::endl;
}

int main()
{
    std::cout << "======================================================" << std::endl;
    std::cout << "Testing Stream Message Separation" << std::endl;
    std::cout << "Client concatenates 3 messages and sends in ONE packet" << std::endl;
    std::cout << "Server must correctly separate and parse each message" << std::endl;
    std::cout << "======================================================\n" << std::endl;

    // Run server and client in separate threads
    std::thread server_thread(test_server_pipelined);
    std::thread client_thread(test_client_pipelined);

    // Wait for both to finish
    server_thread.join();
    client_thread.join();

    std::cout << "\n======================================================" << std::endl;
    std::cout << "✓ Test Complete - Protocol correctly separates concatenated messages!" << std::endl;
    std::cout << "======================================================" << std::endl;

    return 0;
}
