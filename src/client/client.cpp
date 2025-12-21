// ...existing code...
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "../utils/protocol.h"
#include "../../libs/json.hpp"

using json = nlohmann::json;

#define BUFFER_SIZE 65536

// Global socket
int server_sock = -1;
bool is_running = true;
bool is_logged_in = false;

// Receive thread - listens for push notifications from server
void *receive_thread(void *arg)
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
            switch (type)
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
                std::cout << "\n[FRIEND REQUEST] From " << std::endl;
                for (const auto &req : response["data"]["friend_requests"])
                {
                    std::cout << "  - " << req["username"].get<std::string>()
                              << " [" << req["timestamp"].get<std::string>() << "]" << std::endl;
                }
                break;

            case 2004: // FRIEND_LIST_DATA
                std::cout << "\n[FRIEND LIST]:" << std::endl;
                for (const auto &friend_data : response["data"]["friends"])
                {
                    std::cout << "  - " << friend_data["username"].get<std::string>()
                              << " (" << friend_data["user_state"].get<std::string>() << ")" << std::endl;
                }
                break;
            
                case 2400: // FRIEND_LIST_DATA
                std::cout << "\n[GROUP LIST]:" << std::endl;
                for (const auto &group_data : response["data"]["groups"])
                {
                    std::cout << "  - " << group_data["group_name"].get<std::string>()
                              << " (" << group_data["role"].get<std::string>() << ")" << std::endl;
                }
                break;

            case 2005: // OFFLINE_MESSAGES_DATA
                std::cout << "\n[OFFLINE MESSAGES]:" << std::endl;
                for (const auto &msg : response["data"]["messages"])
                {
                    std::cout << "  From " << msg["sender_username"]
                              << ": " << msg["content"] << std::endl;
                }
                break;

            case 2405: // GROUP_MESSAGES_DATA
                std::cout << "\n[GROUP MESSAGES - " << response["data"]["group_name"].get<std::string>() << "]:" << std::endl;
                for (const auto &msg : response["data"]["messages"])
                {
                    std::cout << "  " << msg["sender_username"].get<std::string>()
                              << ": " << msg["content"].get<std::string>()
                              << " [" << msg["timestamp"].get<std::string>() << "]" << std::endl;
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
        catch (json::exception &e)
        {
            std::cerr << "\n[ERROR] Failed to parse response: " << e.what() << std::endl;
        }
    }

    return nullptr;
}

// ...existing code...

// --- New: sectioned menus and handlers ---
void send_request(const json &request)
{
    std::string request_str = request.dump();
    if (send_json_packet(server_sock, request_str.c_str()) != 0)
    {
        std::cerr << "[ERROR] Failed to send request" << std::endl;
    }
}

void show_main_menu()
{
    std::cout << "\n=== Chat Client ===" << std::endl;
    std::cout << "1. Account" << std::endl;
    std::cout << "2. Messages" << std::endl;
    std::cout << "3. Friends" << std::endl;
    std::cout << "4. Groups" << std::endl;
    std::cout << "5. Misc" << std::endl;
    std::cout << "0. Exit" << std::endl;
    std::cout << "> ";
}

void handle_account_menu()
{
    std::cout << "\n--- Account ---" << std::endl;
    std::cout << "1. Register" << std::endl;
    std::cout << "2. Login" << std::endl;
    std::cout << "3. Logout" << std::endl;
    std::cout << "0. Back" << std::endl;
    std::cout << "> ";

    int choice;
    if (!(std::cin >> choice))
    {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Invalid input" << std::endl;
        return;
    }
    std::cin.ignore(10000, '\n');

    json request;
    std::string input;
    switch (choice)
    {
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

    case 0:
    default:
        break;
    }
}

void handle_message_menu()
{
    std::cout << "\n--- Messages ---" << std::endl;
    std::cout << "1. Send Message" << std::endl;
    std::cout << "2. Send Group Message" << std::endl;
    std::cout << "0. Back" << std::endl;
    std::cout << "> ";

    int choice;
    if (!(std::cin >> choice))
    {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Invalid input" << std::endl;
        return;
    }
    std::cin.ignore(10000, '\n');

    json request;
    std::string input;
    switch (choice)
    {
    case 1: // Send Message
    {
        int receiver_id;
        std::cout << "Receiver ID: ";
        if (!(std::cin >> receiver_id))
        {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Invalid input\n";
            return;
        }
        std::cin.ignore(10000, '\n');
        request["type"] = 1003;
        request["data"]["receiver_id"] = receiver_id;
        std::cout << "Message: ";
        std::getline(std::cin, input);
        request["data"]["content"] = input;
        send_request(request);
        break;
    }

    case 2: // Send Group Message
    {
        std::string group_name;
        std::cout << "Group's name: ";
        std::getline(std::cin, group_name);
        std::cout << "Message: ";
        std::getline(std::cin, input);
        request["type"] = 1202;
        request["data"]["group_name"] = group_name;
        request["data"]["content"] = input;
        send_request(request);
        break;
    }

    case 0:
    default:
        break;
    }
}

void handle_friends_menu()
{
    std::cout << "\n--- Friends ---" << std::endl;
    std::cout << "1. Get Friend Request" << std::endl;
    std::cout << "2. Send Friend Request" << std::endl;
    std::cout << "3. Accept Friend Request" << std::endl;
    std::cout << "4. Reject Friend Request" << std::endl;
    std::cout << "5. Unfriend" << std::endl;
    std::cout << "6. Get Friend List" << std::endl;
    std::cout << "0. Back" << std::endl;
    std::cout << "> ";

    int choice;
    if (!(std::cin >> choice))
    {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Invalid input" << std::endl;
        return;
    }
    std::cin.ignore(10000, '\n');

    json request;
    std::string input;
    switch (choice)
    {
    case 1:
        request["type"] = 1300;
        request["data"] = json::object();
        send_request(request);
        break;

    case 2: // Send Friend Request
        std::cout << "Target username: ";
        std::getline(std::cin, input);
        request["type"] = 1301;
        request["data"]["target_username"] = input;
        send_request(request);
        break;

    case 3: // Accept Friend Request
        std::cout << "Target username: ";
        std::getline(std::cin, input);
        request["type"] = 1302;
        request["data"]["target_username"] = input;
        send_request(request);
        break;

    case 4: // Reject Friend Request
        std::cout << "Target username: ";
        std::getline(std::cin, input);
        request["type"] = 1303;
        request["data"]["target_username"] = input;
        send_request(request);
        break;

    case 5: // Unfriend
        std::cout << "Friend username: ";
        std::getline(std::cin, input);
        request["type"] = 1304;
        request["data"]["target_username"] = input;
        send_request(request);
        break;

    case 6: // Get Friend List
        request["type"] = 1305;
        request["data"] = json::object();
        send_request(request);
        break;

    case 0:
    default:
        break;
    }
}

void handle_group_menu()
{
    std::cout << "\n--- Groups ---" << std::endl;
    std::cout << "1. Create Group" << std::endl;
    std::cout << "2. Add User to Group" << std::endl;
    std::cout << "3. Remove User from Group" << std::endl;
    std::cout << "4. Leave Group" << std::endl;
    std::cout << "5. My Groups" << std::endl;
    std::cout << "6. Get Group Messages" << std::endl;
    std::cout << "0. Back" << std::endl;
    std::cout << "> ";

    int choice;
    if (!(std::cin >> choice))
    {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Invalid input" << std::endl;
        return;
    }
    std::cin.ignore(10000, '\n');

    json request;
    std::string input;
    switch (choice)
    {
    case 1: // Create Group
        std::cout << "Group name: ";
        std::getline(std::cin, input);
        request["type"] = 1400;
        request["data"]["group_name"] = input;
        send_request(request);
        break;

    case 2: // Add to Group
    {
        std::string group_name, username;
        std::cout << "Group's name: ";
        std::getline(std::cin, group_name);
        std::cout << "Username: ";
        std::getline(std::cin, username);
        request["type"] = 1401;
        request["data"]["group_name"] = group_name;
        request["data"]["target_username"] = username;
        send_request(request);
        break;
    }

    case 3: // Remove from Group
    {
        std::string group_name, username;
        std::cout << "Group's name: ";
        std::getline(std::cin, group_name);
        std::cout << "Username: ";
        std::getline(std::cin, username);
        request["type"] = 1402;
        request["data"]["group_name"] = group_name;
        request["data"]["target_username"] = username;
        send_request(request);
        break;
    }

    case 4: // Leave Group
    {
        std::string group_name;
        std::cout << "Group's name: ";
        std::getline(std::cin, group_name);
        request["type"] = 1403;
        request["data"]["group_name"] = group_name;
        send_request(request);
        break;
    }

    case 5: // Get Group List
    {
        request["type"] = 1404;
        request["data"] = json::object();
        send_request(request);
        break;
    }

    case 6: // Get Group Messages
    {
        std::string group_name;
        std::cout << "Group's name: ";
        std::getline(std::cin, group_name);
        request["type"] = 1405;
        request["data"]["group_name"] = group_name;
        send_request(request);
        break;
    }

    case 0:
    default:
        break;
    }
}

void handle_misc_menu()
{
    std::cout << "\n--- Misc ---" << std::endl;
    std::cout << "1. Get Offline Messages" << std::endl;
    std::cout << "0. Back" << std::endl;
    std::cout << "> ";

    int choice;
    if (!(std::cin >> choice))
    {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Invalid input" << std::endl;
        return;
    }
    std::cin.ignore(10000, '\n');

    json request;
    switch (choice)
    {
    case 1:
        request["type"] = 1014;
        request["data"] = json::object();
        send_request(request);
        break;
    case 0:
    default:
        break;
    }
}
// --- End new menus ---

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port>" << std::endl;
        return 1;
    }

    const char *server_ip = argv[1];
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

    if (connect(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
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

    // Main menu loop (uses sectioned handlers)
    while (is_running)
    {
        show_main_menu();

        int section;
        if (!(std::cin >> section))
        {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Invalid input" << std::endl;
            continue;
        }
        std::cin.ignore(10000, '\n');

        switch (section)
        {
        case 0:
            is_running = false;
            break;
        case 1:
            handle_account_menu();
            break;
        case 2:
            handle_message_menu();
            break;
        case 3:
            handle_friends_menu();
            break;
        case 4:
            handle_group_menu();
            break;
        case 5:
            handle_misc_menu();
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
// ...existing code...
