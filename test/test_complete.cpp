#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include "../utils/protocol.h"
#include "../../libs/json.hpp"

using json = nlohmann::json;

// Test result tracking
struct TestResult {
    std::string test_name;
    bool passed;
    std::string message;
};

std::vector<TestResult> test_results;

void add_result(const std::string& test_name, bool passed, const std::string& message = "")
{
    test_results.push_back({test_name, passed, message});
    if (passed)
        std::cout << "✅ PASSED: " << test_name << std::endl;
    else
        std::cout << "❌ FAILED: " << test_name << " - " << message << std::endl;
}

void print_summary()
{
    std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║        TEST SUMMARY                    ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& result : test_results)
    {
        if (result.passed)
            passed++;
        else
            failed++;
    }
    
    std::cout << "\nTotal Tests: " << test_results.size() << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    std::cout << "Success Rate: " << (100.0 * passed / test_results.size()) << "%" << std::endl;
    
    if (failed > 0)
    {
        std::cout << "\nFailed Tests:" << std::endl;
        for (const auto& result : test_results)
        {
            if (!result.passed)
            {
                std::cout << "  - " << result.test_name << ": " << result.message << std::endl;
            }
        }
    }
}

int connect_to_server(const char* host, int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &server_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        close(sock);
        return -1;
    }
    
    return sock;
}

json send_and_receive(int sock, const json& request, bool verbose = false)
{
    if (verbose)
        std::cout << "📤 " << request.dump() << std::endl;
    
    send_json_packet(sock, request.dump().c_str());
    
    char buffer[4096];
    int len = receive_json_packet(sock, buffer, sizeof(buffer));
    if (len > 0)
    {
        json response = json::parse(buffer);
        if (verbose)
            std::cout << "📥 " << response.dump(2) << std::endl;
        return response;
    }
    return json();
}

bool check_response_status(const json& response, int expected_status)
{
    if (!response.contains("data") || !response["data"].contains("status"))
        return false;
    return response["data"]["status"] == expected_status;
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
    std::cout << "║   COMPLETE INTEGRATION TEST SUITE      ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;
    std::cout << "\nServer: " << host << ":" << port << std::endl;
    
    // ===== ACCOUNT SERVICE TESTS =====
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "ACCOUNT SERVICE TESTS" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    int sock1 = connect_to_server(host, port);
    if (sock1 < 0)
    {
        std::cerr << "❌ Failed to connect to server" << std::endl;
        return 1;
    }
    
    // Test: Register new user
    json reg_req = {{"type", 1000}, {"data", {{"username", "test_user1"}, {"password", "pass123"}}}};
    json reg_resp = send_and_receive(sock1, reg_req);
    add_result("Register User", check_response_status(reg_resp, 201) || check_response_status(reg_resp, 200));
    sleep(1);
    
    // Test: Register duplicate user (should fail)
    json reg_dup_resp = send_and_receive(sock1, reg_req);
    add_result("Reject Duplicate Registration", check_response_status(reg_dup_resp, 409));
    sleep(1);
    
    // Test: Login with correct credentials
    json login_req = {{"type", 1001}, {"data", {{"username", "test_user1"}, {"password", "pass123"}}}};
    json login_resp = send_and_receive(sock1, login_req);
    add_result("Login with Valid Credentials", check_response_status(login_resp, 200));
    sleep(1);
    
    // Test: Login with wrong password
    int sock_fail = connect_to_server(host, port);
    json login_fail = {{"type", 1001}, {"data", {{"username", "test_user1"}, {"password", "wrong"}}}};
    json fail_resp = send_and_receive(sock_fail, login_fail);
    add_result("Reject Wrong Password", check_response_status(fail_resp, 401));
    close(sock_fail);
    sleep(1);
    
    // Test: Logout
    json logout_req = {{"type", 1002}, {"data", {}}};
    json logout_resp = send_and_receive(sock1, logout_req);
    add_result("Logout", check_response_status(logout_resp, 200));
    close(sock1);
    sleep(1);
    
    // ===== FRIEND SERVICE TESTS =====
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "FRIEND SERVICE TESTS" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    int alice = connect_to_server(host, port);
    int bob = connect_to_server(host, port);
    
    // Register and login both users
    send_and_receive(alice, {{"type", 1000}, {"data", {{"username", "alice_test"}, {"password", "pass"}}}});
    sleep(1);
    send_and_receive(bob, {{"type", 1000}, {"data", {{"username", "bob_test"}, {"password", "pass"}}}});
    sleep(1);
    send_and_receive(alice, {{"type", 1001}, {"data", {{"username", "alice_test"}, {"password", "pass"}}}});
    sleep(1);
    send_and_receive(bob, {{"type", 1001}, {"data", {{"username", "bob_test"}, {"password", "pass"}}}});
    sleep(1);
    
    // Test: Send friend request
    json fr_req = {{"type", 1005}, {"data", {{"target_username", "bob_test"}}}};
    json fr_resp = send_and_receive(alice, fr_req);
    add_result("Send Friend Request", check_response_status(fr_resp, 200));
    sleep(1);
    
    // Test: Send duplicate friend request (should fail)
    json fr_dup = send_and_receive(alice, fr_req);
    add_result("Reject Duplicate Friend Request", check_response_status(fr_dup, 409));
    sleep(1);
    
    // Test: Accept friend request
    json accept_req = {{"type", 1006}, {"data", {{"target_username", "alice_test"}}}};
    json accept_resp = send_and_receive(bob, accept_req);
    add_result("Accept Friend Request", check_response_status(accept_resp, 200));
    sleep(1);
    
    // Test: Get friend list
    json get_friends = {{"type", 1009}, {"data", {}}};
    json friends_resp = send_and_receive(alice, get_friends);
    bool has_friends = friends_resp.contains("data") && friends_resp["data"].contains("friends") 
                       && friends_resp["data"]["friends"].size() > 0;
    add_result("Get Friend List", has_friends);
    sleep(1);
    
    // Test: Unfriend
    json unfriend_req = {{"type", 1008}, {"data", {{"target_username", "bob_test"}}}};
    json unfriend_resp = send_and_receive(alice, unfriend_req);
    add_result("Unfriend", check_response_status(unfriend_resp, 200));
    sleep(1);
    
    // Send another friend request for reject test
    send_and_receive(alice, fr_req);
    sleep(1);
    
    // Test: Reject friend request
    json reject_req = {{"type", 1007}, {"data", {{"target_username", "alice_test"}}}};
    json reject_resp = send_and_receive(bob, reject_req);
    add_result("Reject Friend Request", check_response_status(reject_resp, 200));
    sleep(1);
    
    send_and_receive(alice, logout_req);
    send_and_receive(bob, logout_req);
    close(alice);
    close(bob);
    
    // ===== GROUP SERVICE TESTS =====
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "GROUP SERVICE TESTS" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    int owner = connect_to_server(host, port);
    int member = connect_to_server(host, port);
    
    // Register and login
    send_and_receive(owner, {{"type", 1000}, {"data", {{"username", "owner_test"}, {"password", "pass"}}}});
    sleep(1);
    send_and_receive(member, {{"type", 1000}, {"data", {{"username", "member_test"}, {"password", "pass"}}}});
    sleep(1);
    send_and_receive(owner, {{"type", 1001}, {"data", {{"username", "owner_test"}, {"password", "pass"}}}});
    sleep(1);
    send_and_receive(member, {{"type", 1001}, {"data", {{"username", "member_test"}, {"password", "pass"}}}});
    sleep(1);
    
    // Test: Create group
    json create_group = {{"type", 1010}, {"data", {{"group_name", "Test Group"}}}};
    json create_resp = send_and_receive(owner, create_group);
    add_result("Create Group", check_response_status(create_resp, 200));
    sleep(1);
    
    // Test: Add member to group
    json add_member = {{"type", 1011}, {"data", {{"group_name", "Test Group"}, {"target_username", "member_test"}}}};
    json add_resp = send_and_receive(owner, add_member);
    add_result("Add Member to Group", check_response_status(add_resp, 200));
    sleep(1);
    
    // Test: Non-owner tries to add member (should fail)
    json add_fail = {{"type", 1011}, {"data", {{"group_name", "Test Group"}, {"target_username", "alice_test"}}}};
    json add_fail_resp = send_and_receive(member, add_fail);
    add_result("Reject Non-Owner Add Member", check_response_status(add_fail_resp, 403));
    sleep(1);
    
    // Test: Remove member from group
    json remove_member = {{"type", 1012}, {"data", {{"group_name", "Test Group"}, {"target_username", "member_test"}}}};
    json remove_resp = send_and_receive(owner, remove_member);
    add_result("Remove Member from Group", check_response_status(remove_resp, 200));
    sleep(1);
    
    // Test: Member leaves group
    send_and_receive(owner, add_member);  // Add member back
    sleep(1);
    json leave_group = {{"type", 1013}, {"data", {{"group_name", "Test Group"}}}};
    json leave_resp = send_and_receive(member, leave_group);
    add_result("Leave Group", check_response_status(leave_resp, 200));
    sleep(1);
    
    // Test: Owner leaves group (should dissolve)
    json owner_leave = send_and_receive(owner, leave_group);
    add_result("Owner Leaves Group (Dissolve)", check_response_status(owner_leave, 200));
    sleep(1);
    
    send_and_receive(owner, logout_req);
    send_and_receive(member, logout_req);
    close(owner);
    close(member);
    
    // Print final summary
    print_summary();
    
    return 0;
}
