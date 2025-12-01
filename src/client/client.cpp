#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../utils/protocol.h"
#include "../../libs/json.hpp"

using json = nlohmann::json;

#define BUFFER_SIZE 65536

// Global socket
int server_sock = -1;
bool is_running = true;
bool is_logged_in = false;

// Receive thread - listens for push notifications from server
void* receive_thread(void* arg)
{
    char buffer[BUFFER_SIZE];
    
    while (is_running)
    {
        int bytes_received = receive_json_packet(server_sock, buffer, BUFFER_SIZE);
        
        if (bytes_received == 0)
        {
            std::cout << "\n[DISCONNECTED] Server closed connection" << std::endl;
            is_running = false;
            break;
        }
        else if (bytes_received < 0)
        {
            std::cerr << "\n[ERROR] Failed to receive from server" << std::endl;
            is_running = false;
            break;
        }
        
        try
        {
            json response = json::parse(buffer);
            int type = response["type"];
            
            // Handle different message types
            switch(type)
            {
                case 2000: // RESPONSE
                    std::cout << "\n[SERVER] " << response["data"]["message"] << std::endl;
                    break;
                    
                case 2001: // MESSAGE_RECEIVED (push notification)
                    std::cout << "\n[NEW MESSAGE] From " << response["data"]["sender_username"] 
                              << ": " << response["data"]["content"] << std::endl;
                    break;
                    
                case 2002: // GROUP_MESSAGE_RECEIVED
                    std::cout << "\n[GROUP MESSAGE] [" << response["data"]["group_name"] << "] "
                              << response["data"]["sender_username"] << ": " 
                              << response["data"]["content"] << std::endl;
                    break;
                    
                case 2003: // FRIEND_REQUEST_RECEIVED
                    std::cout << "\n[FRIEND REQUEST] From " << response["data"]["sender_username"] << std::endl;
                    break;
                    
                case 2004: // FRIEND_LIST_DATA
                    std::cout << "\n[FRIEND LIST]:" << std::endl;
                    for (const auto& friend_data : response["data"]["friends"])
                    {
                        std::cout << "  - " << friend_data["username"] 
                                  << " (" << friend_data["user_state"] << ")" << std::endl;
                    }
                    break;
                    
                case 2005: // OFFLINE_MESSAGES_DATA
                    std::cout << "\n[OFFLINE MESSAGES]:" << std::endl;
                    for (const auto& msg : response["data"]["messages"])
                    {
                        std::cout << "  From " << msg["sender_username"] 
                                  << ": " << msg["content"] << std::endl;
                    }
                    break;
                    
                case 2006: // USER_STATUS_UPDATE
                    std::cout << "\n[STATUS] " << response["data"]["username"] 
                              << " is now " << response["data"]["user_state"] << std::endl;
                    break;
                    
                default:
                    std::cout << "\n[SERVER] Unknown response type: " << type << std::endl;
                    break;
            }
            
            std::cout << "> " << std::flush;
        }
        catch (json::exception& e)
        {
            std::cerr << "\n[ERROR] Failed to parse response: " << e.what() << std::endl;
        }
    }
    
    return nullptr;
}

void show_menu()
{
    std::cout << "\n=== Chat Client Menu ===" << std::endl;
    std::cout << "1.  Register" << std::endl;
    std::cout << "2.  Login" << std::endl;
    std::cout << "3.  Logout" << std::endl;
    std::cout << "4.  Send Message" << std::endl;
    std::cout << "5.  Send Group Message" << std::endl;
    std::cout << "6.  Send Friend Request" << std::endl;
    std::cout << "7.  Accept Friend Request" << std::endl;
    std::cout << "8.  Reject Friend Request" << std::endl;
    std::cout << "9.  Unfriend" << std::endl;
    std::cout << "10. Get Friend List" << std::endl;
    std::cout << "11. Create Group" << std::endl;
    std::cout << "12. Add User to Group" << std::endl;
    std::cout << "13. Remove User from Group" << std::endl;
    std::cout << "14. Leave Group" << std::endl;
    std::cout << "15. Get Offline Messages" << std::endl;
    std::cout << "0.  Exit" << std::endl;
    std::cout << "> ";
}

void send_request(const json& request)
{
    std::string request_str = request.dump();
    if (send_json_packet(server_sock, request_str.c_str()) != 0)
    {
        std::cerr << "[ERROR] Failed to send request" << std::endl;
    }
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port>" << std::endl;
        return 1;
    }
    
    const char* server_ip = argv[1];
    int server_port = atoi(argv[2]);
    
    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1)
    {
        perror("socket");
        return 1;
    }
    
    // Connect to server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        std::cerr << "Invalid address: " << server_ip << std::endl;
        close(server_sock);
        return 1;
    }
    
    std::cout << "Connecting to " << server_ip << ":" << server_port << "..." << std::endl;
    
    if (connect(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(server_sock);
        return 1;
    }
    
    std::cout << "Connected to server!" << std::endl;
    
    // Start receive thread
    pthread_t recv_thread;
    if (pthread_create(&recv_thread, nullptr, receive_thread, nullptr) != 0)
    {
        perror("pthread_create");
        close(server_sock);
        return 1;
    }
    
    pthread_detach(recv_thread);
    
    // Main menu loop
    while (is_running)
    {
        show_menu();
        
        int choice;
        if (!(std::cin >> choice))
        {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Invalid input" << std::endl;
            continue;
        }
        std::cin.ignore(10000, '\n');
        
        json request;
        std::string input;
        
        switch(choice)
        {
            case 0: // Exit
                is_running = false;
                break;
                
            case 1: // Register
                std::cout << "Username: ";
                std::getline(std::cin, input);
                request["type"] = 1000;
                request["data"]["username"] = input;
                std::cout << "Password: ";
                std::getline(std::cin, input);
                request["data"]["password"] = input;
                send_request(request);
                break;
                
            case 2: // Login
                std::cout << "Username: ";
                std::getline(std::cin, input);
                request["type"] = 1001;
                request["data"]["username"] = input;
                std::cout << "Password: ";
                std::getline(std::cin, input);
                request["data"]["password"] = input;
                send_request(request);
                is_logged_in = true;
                break;
                
            case 3: // Logout
                request["type"] = 1002;
                request["data"] = json::object();
                send_request(request);
                is_logged_in = false;
                break;
                
            case 4: // Send Message
                std::cout << "Receiver ID: ";
                std::cin >> choice;
                std::cin.ignore(10000, '\n');
                request["type"] = 1003;
                request["data"]["receiver_id"] = choice;
                std::cout << "Message: ";
                std::getline(std::cin, input);
                request["data"]["content"] = input;
                send_request(request);
                break;
                
            case 5: // Send Group Message
                std::cout << "Group ID: ";
                std::cin >> choice;
                std::cin.ignore(10000, '\n');
                request["type"] = 1004;
                request["data"]["group_id"] = choice;
                std::cout << "Message: ";
                std::getline(std::cin, input);
                request["data"]["content"] = input;
                send_request(request);
                break;
                
            case 6: // Send Friend Request
                std::cout << "Target username: ";
                std::getline(std::cin, input);
                request["type"] = 1005;
                request["data"]["target_username"] = input;
                send_request(request);
                break;
                
            case 7: // Accept Friend Request
                std::cout << "Request ID: ";
                std::cin >> choice;
                std::cin.ignore(10000, '\n');
                request["type"] = 1006;
                request["data"]["request_id"] = choice;
                send_request(request);
                break;
                
            case 8: // Reject Friend Request
                std::cout << "Request ID: ";
                std::cin >> choice;
                std::cin.ignore(10000, '\n');
                request["type"] = 1007;
                request["data"]["request_id"] = choice;
                send_request(request);
                break;
                
            case 9: // Unfriend
                std::cout << "Friend ID: ";
                std::cin >> choice;
                std::cin.ignore(10000, '\n');
                request["type"] = 1008;
                request["data"]["friend_id"] = choice;
                send_request(request);
                break;
                
            case 10: // Get Friend List
                request["type"] = 1009;
                request["data"] = json::object();
                send_request(request);
                break;
                
            case 11: // Create Group
                std::cout << "Group name: ";
                std::getline(std::cin, input);
                request["type"] = 1010;
                request["data"]["group_name"] = input;
                send_request(request);
                break;
                
            case 12: // Add to Group
                std::cout << "Group ID: ";
                int group_id;
                std::cin >> group_id;
                std::cout << "User ID: ";
                int user_id;
                std::cin >> user_id;
                std::cin.ignore(10000, '\n');
                request["type"] = 1011;
                request["data"]["group_id"] = group_id;
                request["data"]["user_id"] = user_id;
                send_request(request);
                break;
                
            case 13: // Remove from Group
                std::cout << "Group ID: ";
                std::cin >> group_id;
                std::cout << "User ID: ";
                std::cin >> user_id;
                std::cin.ignore(10000, '\n');
                request["type"] = 1012;
                request["data"]["group_id"] = group_id;
                request["data"]["user_id"] = user_id;
                send_request(request);
                break;
                
            case 14: // Leave Group
                std::cout << "Group ID: ";
                std::cin >> choice;
                std::cin.ignore(10000, '\n');
                request["type"] = 1013;
                request["data"]["group_id"] = choice;
                send_request(request);
                break;
                
            case 15: // Get Offline Messages
                request["type"] = 1014;
                request["data"] = json::object();
                send_request(request);
                break;
                
            default:
                std::cout << "Invalid choice" << std::endl;
                break;
        }
        
        // Give time for server response
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    close(server_sock);
    std::cout << "Disconnected from server" << std::endl;
    
    return 0;
}
