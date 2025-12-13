#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "protocol.h"
#include "../../libs/json.hpp"

using json = nlohmann::json;

void test_register(int sock)
{
    std::cout << "\n=== Testing REGISTER ===" << std::endl;
    
    json request = {
        {"type", 1000},
        {"data", {
            {"username", "testuser"},
            {"password", "testpass123"}
        }}
    };
    
    std::cout << "Sending: " << request.dump() << std::endl;
    send_json_packet(sock, request.dump().c_str());
    
    char buffer[4096];
    int len = receive_json_packet(sock, buffer, sizeof(buffer));
    if (len > 0)
    {
        json response = json::parse(buffer);
        std::cout << "Response: " << response.dump(2) << std::endl;
    }
}

void test_login(int sock)
{
    std::cout << "\n=== Testing LOGIN ===" << std::endl;
    
    json request = {
        {"type", 1001},
        {"data", {
            {"username", "testuser"},
            {"password", "testpass123"}
        }}
    };
    
    std::cout << "Sending: " << request.dump() << std::endl;
    send_json_packet(sock, request.dump().c_str());
    
    char buffer[4096];
    int len = receive_json_packet(sock, buffer, sizeof(buffer));
    if (len > 0)
    {
        json response = json::parse(buffer);
        std::cout << "Response: " << response.dump(2) << std::endl;
    }
}

void test_logout(int sock)
{
    std::cout << "\n=== Testing LOGOUT ===" << std::endl;
    
    json request = {
        {"type", 1002},
        {"data", {}}
    };
    
    std::cout << "Sending: " << request.dump() << std::endl;
    send_json_packet(sock, request.dump().c_str());
    
    char buffer[4096];
    int len = receive_json_packet(sock, buffer, sizeof(buffer));
    if (len > 0)
    {
        json response = json::parse(buffer);
        std::cout << "Response: " << response.dump(2) << std::endl;
    }
}

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(sock);
        return 1;
    }
    
    std::cout << "Connected to server on port 8080" << std::endl;
    
    // Test sequence
    test_register(sock);
    sleep(1);
    
    test_login(sock);
    sleep(1);
    
    test_logout(sock);
    sleep(1);
    
    // Test login again
    test_login(sock);
    sleep(1);
    
    test_logout(sock);
    
    close(sock);
    return 0;
}
