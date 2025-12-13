#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "../utils/protocol.h"
#include "../../libs/json.hpp"

using json = nlohmann::json;

// Helper function to create a socket and connect to server
int connect_to_server(const char* host, int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &server_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(sock);
        return -1;
    }
    
    return sock;
}

// Send request and receive response
json send_and_receive(int sock, const json& request)
{
    std::cout << "\n📤 Sending: " << request.dump(2) << std::endl;
    send_json_packet(sock, request.dump().c_str());
    
    char buffer[4096];
    int len = receive_json_packet(sock, buffer, sizeof(buffer));
    if (len > 0)
    {
        json response = json::parse(buffer);
        std::cout << "📥 Response: " << response.dump(2) << std::endl;
        return response;
    }
    return json();
}

void test_register_user(int sock, const std::string& username, const std::string& password)
{
    std::cout << "\n=== REGISTER: " << username << " ===" << std::endl;
    
    json request = {
        {"type", 1000},
        {"data", {
            {"username", username},
            {"password", password}
        }}
    };
    
    send_and_receive(sock, request);
}

void test_login(int sock, const std::string& username, const std::string& password)
{
    std::cout << "\n=== LOGIN: " << username << " ===" << std::endl;
    
    json request = {
        {"type", 1001},
        {"data", {
            {"username", username},
            {"password", password}
        }}
    };
    
    send_and_receive(sock, request);
}

void test_send_friend_request(int sock, const std::string& target_username)
{
    std::cout << "\n=== SEND FRIEND REQUEST TO: " << target_username << " ===" << std::endl;
    
    json request = {
        {"type", 1005},
        {"data", {
            {"target_username", target_username}
        }}
    };
    
    send_and_receive(sock, request);
}

void test_get_friend_requests(int sock)
{
    std::cout << "\n=== GET FRIEND REQUESTS ===" << std::endl;
    
    json request = {
        {"type", 9999},  // Custom type for testing - you may need to add this handler
        {"data", {}}
    };
    
    send_and_receive(sock, request);
}

void test_accept_friend_request(int sock, const std::string& sender_username)
{
    std::cout << "\n=== ACCEPT FRIEND REQUEST FROM: " << sender_username << " ===" << std::endl;
    
    json request = {
        {"type", 1006},
        {"data", {
            {"target_username", sender_username}
        }}
    };
    
    send_and_receive(sock, request);
}

void test_reject_friend_request(int sock, const std::string& sender_username)
{
    std::cout << "\n=== REJECT FRIEND REQUEST FROM: " << sender_username << " ===" << std::endl;
    
    json request = {
        {"type", 1007},
        {"data", {
            {"target_username", sender_username}
        }}
    };
    
    send_and_receive(sock, request);
}

void test_get_friend_list(int sock)
{
    std::cout << "\n=== GET FRIEND LIST ===" << std::endl;
    
    json request = {
        {"type", 1009},
        {"data", {}}
    };
    
    send_and_receive(sock, request);
}

void test_unfriend(int sock, const std::string& friend_username)
{
    std::cout << "\n=== UNFRIEND: " << friend_username << " ===" << std::endl;
    
    json request = {
        {"type", 1008},
        {"data", {
            {"target_username", friend_username}
        }}
    };
    
    send_and_receive(sock, request);
}

void test_logout(int sock)
{
    std::cout << "\n=== LOGOUT ===" << std::endl;
    
    json request = {
        {"type", 1002},
        {"data", {}}
    };
    
    send_and_receive(sock, request);
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <host> <port>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 127.0.0.1 8080" << std::endl;
        return 1;
    }
    
    const char* host = argv[1];
    int port = atoi(argv[2]);
    
    std::cout << "╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║   FRIEND SERVICE TEST SUITE            ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;
    
    // Create two users for testing
    int alice_sock = connect_to_server(host, port);
    int bob_sock = connect_to_server(host, port);
    
    if (alice_sock < 0 || bob_sock < 0)
    {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }
    
    std::cout << "\n✅ Connected to server at " << host << ":" << port << std::endl;
    
    // Test 1: Register users
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 1: Register Users" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_register_user(alice_sock, "alice", "password123");
    sleep(1);
    test_register_user(bob_sock, "bob", "password456");
    sleep(1);
    
    // Test 2: Login users
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 2: Login Users" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_login(alice_sock, "alice", "password123");
    sleep(1);
    test_login(bob_sock, "bob", "password456");
    sleep(1);
    
    // Test 3: Alice sends friend request to Bob
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 3: Send Friend Request (Alice -> Bob)" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_send_friend_request(alice_sock, "bob");
    sleep(1);
    
    // Test 4: Get friend list (should be empty)
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 4: Get Friend List (Should be empty)" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_get_friend_list(alice_sock);
    sleep(1);
    
    // Test 5: Bob accepts friend request
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 5: Accept Friend Request (Bob accepts Alice)" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_accept_friend_request(bob_sock, "alice");
    sleep(1);
    
    // Test 6: Get friend list (should have each other)
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 6: Get Friend List (Should have friends)" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_get_friend_list(alice_sock);
    sleep(1);
    test_get_friend_list(bob_sock);
    sleep(1);
    
    // Test 7: Unfriend
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 7: Unfriend (Alice unfriends Bob)" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_unfriend(alice_sock, "bob");
    sleep(1);
    
    // Test 8: Get friend list again (should be empty)
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 8: Get Friend List (Should be empty again)" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_get_friend_list(alice_sock);
    sleep(1);
    test_get_friend_list(bob_sock);
    sleep(1);
    
    // Test 9: Test reject friend request
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 9: Reject Friend Request" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_send_friend_request(alice_sock, "bob");
    sleep(1);
    test_reject_friend_request(bob_sock, "alice");
    sleep(1);
    
    // Test 10: Logout
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 10: Logout Users" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_logout(alice_sock);
    sleep(1);
    test_logout(bob_sock);
    
    close(alice_sock);
    close(bob_sock);
    
    std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║   ALL TESTS COMPLETED                  ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;
    
    return 0;
}
