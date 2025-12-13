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

void test_create_group(int sock, const std::string& group_name)
{
    std::cout << "\n=== CREATE GROUP: " << group_name << " ===" << std::endl;
    
    json request = {
        {"type", 1010},
        {"data", {
            {"group_name", group_name}
        }}
    };
    
    send_and_receive(sock, request);
}

void test_add_to_group(int sock, const std::string& group_name, const std::string& target_username)
{
    std::cout << "\n=== ADD TO GROUP: " << target_username << " -> " << group_name << " ===" << std::endl;
    
    json request = {
        {"type", 1011},
        {"data", {
            {"group_name", group_name},
            {"target_username", target_username}
        }}
    };
    
    send_and_receive(sock, request);
}

void test_remove_from_group(int sock, const std::string& group_name, const std::string& target_username)
{
    std::cout << "\n=== REMOVE FROM GROUP: " << target_username << " <- " << group_name << " ===" << std::endl;
    
    json request = {
        {"type", 1012},
        {"data", {
            {"group_name", group_name},
            {"target_username", target_username}
        }}
    };
    
    send_and_receive(sock, request);
}

void test_leave_group(int sock, const std::string& group_name)
{
    std::cout << "\n=== LEAVE GROUP: " << group_name << " ===" << std::endl;
    
    json request = {
        {"type", 1013},
        {"data", {
            {"group_name", group_name}
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
    std::cout << "║   GROUP SERVICE TEST SUITE             ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;
    
    // Create three users for testing
    int alice_sock = connect_to_server(host, port);
    int bob_sock = connect_to_server(host, port);
    int charlie_sock = connect_to_server(host, port);
    
    if (alice_sock < 0 || bob_sock < 0 || charlie_sock < 0)
    {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }
    
    std::cout << "\n✅ Connected to server at " << host << ":" << port << std::endl;
    
    // Test 1: Register users
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 1: Register Users" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_register_user(alice_sock, "alice_group", "password123");
    sleep(1);
    test_register_user(bob_sock, "bob_group", "password456");
    sleep(1);
    test_register_user(charlie_sock, "charlie_group", "password789");
    sleep(1);
    
    // Test 2: Login users
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 2: Login Users" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_login(alice_sock, "alice_group", "password123");
    sleep(1);
    test_login(bob_sock, "bob_group", "password456");
    sleep(1);
    test_login(charlie_sock, "charlie_group", "password789");
    sleep(1);
    
    // Test 3: Alice creates a group
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 3: Create Group (Alice creates 'Study Group')" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_create_group(alice_sock, "Study Group");
    sleep(1);
    
    // Test 4: Alice adds Bob to the group
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 4: Add Member (Alice adds Bob)" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_add_to_group(alice_sock, "Study Group", "bob_group");
    sleep(1);
    
    // Test 5: Alice adds Charlie to the group
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 5: Add Member (Alice adds Charlie)" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_add_to_group(alice_sock, "Study Group", "charlie_group");
    sleep(1);
    
    // Test 6: Bob tries to add someone (should fail - not owner)
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 6: Permission Test (Bob tries to add - should fail)" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    // Create another user to add
    int david_sock = connect_to_server(host, port);
    test_register_user(david_sock, "david_group", "password000");
    sleep(1);
    test_login(david_sock, "david_group", "password000");
    sleep(1);
    test_add_to_group(bob_sock, "Study Group", "david_group");
    sleep(1);
    
    // Test 7: Alice removes Charlie from the group
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 7: Remove Member (Alice removes Charlie)" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_remove_from_group(alice_sock, "Study Group", "charlie_group");
    sleep(1);
    
    // Test 8: Bob leaves the group
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 8: Leave Group (Bob leaves)" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_leave_group(bob_sock, "Study Group");
    sleep(1);
    
    // Test 9: Alice (owner) leaves the group (should dissolve group)
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 9: Owner Leaves (Should dissolve group)" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_leave_group(alice_sock, "Study Group");
    sleep(1);
    
    // Test 10: Create another group for final test
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 10: Create Second Group (Bob creates 'Coding Club')" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_create_group(bob_sock, "Coding Club");
    sleep(1);
    test_add_to_group(bob_sock, "Coding Club", "alice_group");
    sleep(1);
    
    // Test 11: Logout
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "TEST 11: Logout Users" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    test_logout(alice_sock);
    sleep(1);
    test_logout(bob_sock);
    sleep(1);
    test_logout(charlie_sock);
    sleep(1);
    test_logout(david_sock);
    
    close(alice_sock);
    close(bob_sock);
    close(charlie_sock);
    close(david_sock);
    
    std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║   ALL TESTS COMPLETED                  ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;
    
    return 0;
}
