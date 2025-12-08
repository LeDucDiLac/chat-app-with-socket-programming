#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sqlite3.h>
#include "protocol.h"
#include "db_manager.h"
#include "../../libs/json.hpp"

#include "friend_service.h"

using json = nlohmann::json;

#define BACKLOG 10
#define BUFFER_SIZE 65536

// Session management (in-memory)
// If a session exists here, the user is authenticated
struct Session {
    int socket_fd;
    int user_id;
    std::string username;
};

std::vector<Session> active_sessions;
pthread_mutex_t sessions_mutex = PTHREAD_MUTEX_INITIALIZER;

// Global database connection
sqlite3* g_db = nullptr;

// Placeholder function prototypes
void handle_register(int client_sock, const json& request);
void handle_login(int client_sock, const json& request);
void handle_logout(int client_sock, const json& request);
void handle_send_message(int client_sock, const json& request);
void handle_send_group_message(int client_sock, const json& request);
void handle_get_friend_request(int client_sock, const json& request);
void handle_send_friend_request(int client_sock, const json& request);
void handle_accept_friend_request(int client_sock, const json& request);
void handle_reject_friend_request(int client_sock, const json& request);
void handle_unfriend(int client_sock, const json& request);
void handle_get_friend_list(int client_sock, const json& request);
void handle_create_group(int client_sock, const json& request);
void handle_add_to_group(int client_sock, const json& request);
void handle_remove_from_group(int client_sock, const json& request);
void handle_leave_group(int client_sock, const json& request);
void handle_get_offline_messages(int client_sock, const json& request);

// Helper function to find session by socket
Session* find_session_by_socket(int socket_fd)
{
    for (auto& session : active_sessions)
    {
        if (session.socket_fd == socket_fd)
        {
            return &session;
        }
    }
    return nullptr;
}

// Send JSON response helper
void send_response(int client_sock, int status, const std::string& message)
{
    json response = {
        {"type", 2000},
        {"data", {
            {"status", status},
            {"message", message}
        }}
    };
    
    std::string response_str = response.dump();
    send_json_packet(client_sock, response_str.c_str());
}

// Process client requests
void process_client_request(int client_sock, const json& request)
{
    try
    {
        int type = request["type"];
        
        std::cout << "[SERVER] Processing request type: " << type << std::endl;
        
        switch(type)
        {
            case 1000: // REGISTER
                handle_register(client_sock, request);
                break;
            case 1001: // LOGIN
                handle_login(client_sock, request);
                break;
            case 1002: // LOGOUT
                handle_logout(client_sock, request);
                break;
            case 1003: // SEND_MESSAGE
                handle_send_message(client_sock, request);
                break;
            case 1004: // SEND_GROUP_MESSAGE
                handle_send_group_message(client_sock, request);
                break;
            case 3000: // SEND_FRIEND_REQUEST
                handle_get_friend_request(client_sock, request);
                break;
            case 3001: // SEND_FRIEND_REQUEST
                handle_send_friend_request(client_sock, request);
                break;
            case 3002: // ACCEPT_FRIEND_REQUEST
                handle_accept_friend_request(client_sock, request);
                break;
            case 3003: // REJECT_FRIEND_REQUEST
                handle_reject_friend_request(client_sock, request);
                break;
            case 3004: // UNFRIEND
                handle_unfriend(client_sock, request);
                break;
            case 3005: // GET_FRIEND_LIST
                handle_get_friend_list(client_sock, request);
                break;
            case 1010: // CREATE_GROUP
                handle_create_group(client_sock, request);
                break;
            case 1011: // ADD_TO_GROUP
                handle_add_to_group(client_sock, request);
                break;
            case 1012: // REMOVE_FROM_GROUP
                handle_remove_from_group(client_sock, request);
                break;
            case 1013: // LEAVE_GROUP
                handle_leave_group(client_sock, request);
                break;
            case 1014: // GET_OFFLINE_MESSAGES
                handle_get_offline_messages(client_sock, request);
                break;
            default:
                send_response(client_sock, 400, "Unknown request type");
                break;
        }
    }
    catch (json::exception& e)
    {
        std::cerr << "[SERVER] JSON error: " << e.what() << std::endl;
        send_response(client_sock, 400, "Invalid JSON format");
    }
}

// Client handler thread
void* client_handler(void* arg)
{
    int client_sock = *(int*)arg;
    delete (int*)arg;
    
    char buffer[BUFFER_SIZE];
    
    std::cout << "[SERVER] Client connected on socket " << client_sock << std::endl;
    
    // Main request-response loop
    while (true)
    {
        int bytes_received = receive_json_packet(client_sock, buffer, BUFFER_SIZE);
        
        if (bytes_received == 0)
        {
            std::cout << "[SERVER] Client disconnected (socket " << client_sock << ")" << std::endl;
            break;
        }
        else if (bytes_received < 0)
        {
            std::cerr << "[SERVER] Error receiving from client (socket " << client_sock << ")" << std::endl;
            break;
        }
        
        // Parse and process request
        try
        {
            json request = json::parse(buffer);
            process_client_request(client_sock, request);
        }
        catch (json::exception& e)
        {
            std::cerr << "[SERVER] JSON parse error: " << e.what() << std::endl;
            send_response(client_sock, 400, "Invalid JSON");
        }
    }
    
    // Cleanup: remove from sessions
    pthread_mutex_lock(&sessions_mutex);
    for (auto it = active_sessions.begin(); it != active_sessions.end(); ++it)
    {
        if (it->socket_fd == client_sock)
        {
            active_sessions.erase(it);
            break;
        }
    }
    pthread_mutex_unlock(&sessions_mutex);
    
    close(client_sock);
    return nullptr;
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }
    
    // Initialize database
    g_db = db_init("database/chat.db");
    if (g_db == nullptr)
    {
        std::cerr << "[ERROR] Failed to initialize database" << std::endl;
        return 1;
    }
    
    int server_port = atoi(argv[1]);
    int listen_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    
    // Create socket
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        return 1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt");
        close(listen_sock);
        return 1;
    }
    
    // Bind
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(listen_sock);
        return 1;
    }
    
    // Listen
    if (listen(listen_sock, BACKLOG) == -1)
    {
        perror("listen");
        close(listen_sock);
        return 1;
    }
    
    std::cout << "[SERVER] Chat server started on port " << server_port << std::endl;
    std::cout << "[SERVER] Waiting for connections..." << std::endl;
    
    // Accept connections loop
    while (true)
    {
        client_addr_len = sizeof(client_addr);
        int client_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &client_addr_len);
        
        if (client_sock == -1)
        {
            perror("accept");
            continue;
        }
        
        std::cout << "[SERVER] New connection from " 
                  << inet_ntoa(client_addr.sin_addr) << ":" 
                  << ntohs(client_addr.sin_port) << std::endl;
        
        // Create thread to handle client
        pthread_t thread_id;
        int* client_sock_ptr = new int(client_sock);
        
        if (pthread_create(&thread_id, nullptr, client_handler, client_sock_ptr) != 0)
        {
            perror("pthread_create");
            delete client_sock_ptr;
            close(client_sock);
            continue;
        }
        
        // Detach thread so it cleans up automatically
        pthread_detach(thread_id);
    }
    
    close(listen_sock);
    db_close(g_db);
    return 0;
}

// ============================================================================
// PLACEHOLDER IMPLEMENTATIONS - To be implemented later
// ============================================================================

void handle_register(int client_sock, const json& request)
{
    try
    {
        std::string username = request["data"]["username"];
        std::string password = request["data"]["password"];
        
        std::cout << "[REGISTER] Request from username: " << username << std::endl;
        
        int user_id;
        int result = db_register_user(g_db, username, password, &user_id);
        
        if (result == DB_USER_EXISTS)
        {
            send_response(client_sock, 409, "Username already exists");
        }
        else if (result == DB_ERROR)
        {
            send_response(client_sock, 500, "Database error");
        }
        else
        {
            // Log registration activity
            db_log_activity(g_db, user_id, "register", "User registered");
            
            json response = {
                {"type", 2000},
                {"data", {
                    {"status", 201},
                    {"message", "Registration successful"},
                    {"user_id", user_id}
                }}
            };
            send_json_packet(client_sock, response.dump().c_str());
            
            std::cout << "[REGISTER] Success: " << username << " (ID: " << user_id << ")" << std::endl;
        }
    }
    catch (json::exception& e)
    {
        std::cerr << "[REGISTER] JSON error: " << e.what() << std::endl;
        send_response(client_sock, 400, "Invalid request format");
    }
}

void handle_login(int client_sock, const json& request)
{
    try
    {
        std::string username = request["data"]["username"];
        std::string password = request["data"]["password"];
        
        std::cout << "[LOGIN] Request from username: " << username << std::endl;
        
        int user_id;
        int result = db_verify_login(g_db, username, password, &user_id);
        
        if (result == DB_USER_NOT_FOUND)
        {
            send_response(client_sock, 404, "User not found");
        }
        else if (result == DB_INVALID_PASSWORD)
        {
            send_response(client_sock, 401, "Invalid password");
        }
        else if (result == DB_USER_BANNED)
        {
            send_response(client_sock, 403, "Account is banned");
        }
        else if (result == DB_ERROR)
        {
            send_response(client_sock, 500, "Database error");
        }
        else
        {
            pthread_mutex_lock(&sessions_mutex);
            
            // Check if this socket already has a session
            bool socket_has_session = false;
            for (const auto& session : active_sessions)
            {
                if (session.socket_fd == client_sock)
                {
                    socket_has_session = true;
                    break;
                }
            }
            
            if (socket_has_session)
            {
                pthread_mutex_unlock(&sessions_mutex);
                send_response(client_sock, 409, "Already logged in. Please logout first");
                return;
            }
            
            // Check if user is already logged in from another connection
            bool already_logged_in = false;
            for (const auto& session : active_sessions)
            {
                if (session.user_id == user_id)
                {
                    already_logged_in = true;
                    break;
                }
            }
            
            if (already_logged_in)
            {
                pthread_mutex_unlock(&sessions_mutex);
                send_response(client_sock, 409, "User already logged in from another connection");
                return;
            }
            
            // Create session
            Session new_session = {client_sock, user_id, username};
            active_sessions.push_back(new_session);
            pthread_mutex_unlock(&sessions_mutex);
            
            // Update user state to online
            db_update_user_state(g_db, user_id, "online");
            
            // Log login activity
            db_log_activity(g_db, user_id, "login", "User logged in");
            
            json response = {
                {"type", 2000},
                {"data", {
                    {"status", 200},
                    {"message", "Login successful"},
                    {"user_id", user_id},
                    {"username", username}
                }}
            };
            send_json_packet(client_sock, response.dump().c_str());
            
            std::cout << "[LOGIN] Success: " << username << " (ID: " << user_id << ")" << std::endl;
        }
    }
    catch (json::exception& e)
    {
        std::cerr << "[LOGIN] JSON error: " << e.what() << std::endl;
        send_response(client_sock, 400, "Invalid request format");
    }
}

void handle_logout(int client_sock, const json& request)
{
    std::cout << "[LOGOUT] Request from socket " << client_sock << std::endl;
    
    pthread_mutex_lock(&sessions_mutex);
    Session* session = find_session_by_socket(client_sock);
    
    if (session == nullptr)
    {
        pthread_mutex_unlock(&sessions_mutex);
        send_response(client_sock, 401, "Not logged in");
        return;
    }
    
    int user_id = session->user_id;
    std::string username = session->username;
    
    // Remove session
    for (auto it = active_sessions.begin(); it != active_sessions.end(); ++it)
    {
        if (it->socket_fd == client_sock)
        {
            active_sessions.erase(it);
            break;
        }
    }
    pthread_mutex_unlock(&sessions_mutex);
    
    // Update user state to offline
    db_update_user_state(g_db, user_id, "offline");
    
    // Log logout activity
    db_log_activity(g_db, user_id, "logout", "User logged out");
    
    send_response(client_sock, 200, "Logout successful");
    
    std::cout << "[LOGOUT] Success: " << username << " (ID: " << user_id << ")" << std::endl;
}

void handle_send_message(int client_sock, const json& request)
{
    // TODO: Implement send message
    // - Check if receiver exists
    // - Store message in database
    // - If receiver online, push notification
    // - If offline, mark as offline message
    
    std::cout << "[PLACEHOLDER] SEND_MESSAGE called" << std::endl;
    send_response(client_sock, 200, "Send message feature not implemented yet");
}

void handle_send_group_message(int client_sock, const json& request)
{
    // TODO: Implement send group message
    std::cout << "[PLACEHOLDER] SEND_GROUP_MESSAGE called" << std::endl;
    send_response(client_sock, 200, "Send group message feature not implemented yet");
}

void handle_get_friend_request(int client_sock, const json& request)
{
    // 1. Lấy session người gửi
    Session* session = find_session_by_socket(client_sock);
    if (!session) {
        send_response(client_sock, 401, "You are not logged in");
        return;
    }
    int receiver_id = session->user_id;
    auto requests = getAllFriendRequests(receiver_id);
    json response;
    response["type"] = 2003;
    response["data"]["friend_requests"] = json::array();

    for (const auto &req : requests)
    {
        response["data"]["friend_requests"].push_back({
            {"username", req.sender_username},
            {"timestamp", req.timestamp}
        });
    }

    std::string response_str = response.dump();
    send_json_packet(client_sock, response_str.c_str());
}

void handle_send_friend_request(int client_sock, const json& request)
{
    // 1. Lấy session người gửi
    Session* senderSession = find_session_by_socket(client_sock);

    if (!senderSession) {
        send_response(client_sock, 401, "Session not found");
        return;
    }

    // 2. Lấy username người nhận từ JSON
    if (!request.contains("data") || !request["data"].contains("target_username")) {
        send_response(client_sock, 400, "Missing receiver username");
        return;
    }

    std::string receiverUsername = request["data"]["target_username"];

    // 3. Lấy user id từ DB
    int senderId   =  senderSession->user_id;
    int receiverId = getUserIdByUsername(receiverUsername);

    std::cout << senderId << receiverId << std::endl;

    if (receiverId == -1) {
        send_response(client_sock, 404, "Receiver not found");
        return;
    }

    if (senderId == receiverId) {
        send_response(client_sock, 400, "You cannot send friend request to yourself");
        return;
    }

    // 4. Kiểm tra đã gửi chưa
    if (friendRequestExists(senderId, receiverId)) {
        send_response(client_sock, 409, "Friend request already exists");
        return;
    }

    if (friendshipExists(senderId, receiverId)) {
        send_response(client_sock, 409, "Already friend");
        return;
    }

    // 5. Thêm vào database
    if (!addFriendRequest(senderId, receiverId)) {
        send_response(client_sock, 500, "Failed to create friend request");
        return;
    }

    // 6. Thành công
    send_response(client_sock, 200, "Friend request sent successfully");
}

void handle_accept_friend_request(int client_sock, const json& request)
{
    // 1. Lấy session
    Session* session = find_session_by_socket(client_sock);
    if (!session)
    {
        send_response(client_sock, 401, "You are not authenticated");
        return;
    }

    int receiver_id = session->user_id;            // người accept
    std::string receiver_username = session->username;

    // 2. Lấy username của người gửi lời mời
    if (!request.contains("data") || 
        !request["data"].contains("target_username"))
    {
        send_response(client_sock, 400, "Invalid request format");
        return;
    }

    std::string sender_username = request["data"]["target_username"];

    // 3. Lấy sender_id từ username
    int sender_id = getUserIdByUsername(sender_username);
    if (sender_id == -1)
    {
        send_response(client_sock, 404, "User not found");
        return;
    }

    // 4. Kiểm tra đã là bạn bè chưa
    if (friendshipExists(sender_id, receiver_id))
    {
        send_response(client_sock, 409, "You are already friends");
        return;
    }

    // 5. Kiểm tra có request tồn tại không (sender -> receiver)
    if (!friendRequestExists(sender_id, receiver_id))
    {
        send_response(client_sock, 404, "Friend request not found");
        return;
    }

    // 6. Thêm vào bảng friends
    if (!addFriendship(sender_id, receiver_id))
    {
        send_response(client_sock, 500, "Failed to add friend");
        return;
    }

    // 7. Xóa friend request
    if (!removeFriendRequest(sender_id, receiver_id))
    {
        send_response(client_sock, 500, "Failed to delete friend request");
        return;
    }

    send_response(client_sock, 200, "Friend added");
}

void handle_reject_friend_request(int client_sock, const json& request)
{
    // 1. Lấy session
    Session* session = find_session_by_socket(client_sock);
    if (!session)
    {
        send_response(client_sock, 401, "You are not authenticated");
        return;
    }

    int receiver_id = session->user_id;            // người accept
    std::string receiver_username = session->username;

    // 2. Lấy username của người gửi lời mời
    if (!request.contains("data") || 
        !request["data"].contains("target_username"))
    {
        send_response(client_sock, 400, "Invalid request format");
        return;
    }

    std::string sender_username = request["data"]["target_username"];

    // 3. Lấy sender_id từ username
    int sender_id = getUserIdByUsername(sender_username);
    if (sender_id == -1)
    {
        send_response(client_sock, 404, "User not found");
        return;
    }

    // 4. Kiểm tra đã là bạn bè chưa
    if (friendshipExists(sender_id, receiver_id))
    {
        send_response(client_sock, 409, "You are already friends");
        return;
    }

    // 5. Kiểm tra có request tồn tại không (sender -> receiver)
    if (!friendRequestExists(sender_id, receiver_id))
    {
        send_response(client_sock, 404, "Friend request not found");
        return;
    }

    // 7. Xóa friend request
    if (!removeFriendRequest(sender_id, receiver_id))
    {
        send_response(client_sock, 500, "Failed to delete friend request");
        return;
    }

    send_response(client_sock, 200, "Friend request rejected");
}

void handle_unfriend(int client_sock, const json& request)
{
    // 1. Lấy session
    Session* session = find_session_by_socket(client_sock);
    if (!session)
    {
        send_response(client_sock, 401, "You are not authenticated");
        return;
    }

    int user_id1 = session->user_id;

    if (!request.contains("data") || 
        !request["data"].contains("target_username"))
    {
        send_response(client_sock, 400, "Invalid request format");
        return;
    }

    std::string target_username = request["data"]["target_username"];

    // 3. Lấy sender_id từ username
    int user_id2 = getUserIdByUsername(target_username);
    if (user_id2 == -1)
    {
        send_response(client_sock, 404, "User not found");
        return;
    }

    // 4. Kiểm tra đã là bạn bè chưa
    if (!friendshipExists(user_id1, user_id2))
    {
        send_response(client_sock, 409, "You are not friends");
        return;
    }

    if (!removeFriendship(user_id1, user_id2))
    {
        send_response(client_sock, 500, "Failed to unfriend");
        return;
    }

    send_response(client_sock, 200, "Unfriend successfully");
}

void handle_get_friend_list(int client_sock, const json& request)
{
    Session* session = find_session_by_socket(client_sock);
    if (!session)
    {
        send_response(client_sock, 401, "You are not authenticated");
        return;
    }
    int user_id = session->user_id;
    auto friends = getAllFriend(user_id);

    json response;
    response["type"] = 2004;
    response["data"]["friends"] = json::array();

    for (const auto &f : friends)
    {
        response["data"]["friends"].push_back({
            {"id", f.id},
            {"username", f.username},
            {"user_state", f.status}
        });
    }
    std::string response_str = response.dump();
    send_json_packet(client_sock, response_str.c_str());
}

void handle_create_group(int client_sock, const json& request)
{
    // TODO: Implement create group
    std::cout << "[PLACEHOLDER] CREATE_GROUP called" << std::endl;
    send_response(client_sock, 200, "Create group feature not implemented yet");
}

void handle_add_to_group(int client_sock, const json& request)
{
    // TODO: Implement add to group
    std::cout << "[PLACEHOLDER] ADD_TO_GROUP called" << std::endl;
    send_response(client_sock, 200, "Add to group feature not implemented yet");
}

void handle_remove_from_group(int client_sock, const json& request)
{
    // TODO: Implement remove from group
    std::cout << "[PLACEHOLDER] REMOVE_FROM_GROUP called" << std::endl;
    send_response(client_sock, 200, "Remove from group feature not implemented yet");
}

void handle_leave_group(int client_sock, const json& request)
{
    // TODO: Implement leave group
    std::cout << "[PLACEHOLDER] LEAVE_GROUP called" << std::endl;
    send_response(client_sock, 200, "Leave group feature not implemented yet");
}

void handle_get_offline_messages(int client_sock, const json& request)
{
    // TODO: Implement get offline messages
    std::cout << "[PLACEHOLDER] GET_OFFLINE_MESSAGES called" << std::endl;
    send_response(client_sock, 200, "Get offline messages feature not implemented yet");
}
