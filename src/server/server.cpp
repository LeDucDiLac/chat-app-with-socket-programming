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
#include "../../libs/json.hpp"

#include "account_service.h"
#include "friend_service.h"
#include "group_service.h"
#include "chat_service.h"
#include "write_log.h"

using json = nlohmann::json;

#define BACKLOG 10
#define BUFFER_SIZE 65536

// Session management (in-memory)
// If a session exists here, the user is authenticated
struct Session
{
    int socket_fd;
    int user_id;
    std::string username;
};

std::vector<Session> active_sessions;
pthread_mutex_t sessions_mutex = PTHREAD_MUTEX_INITIALIZER;

// Global database connection
sqlite3 *g_db = nullptr;

// Placeholder function prototypes
void handle_register(int client_sock, const json &request);
void handle_login(int client_sock, const json &request);
void handle_logout(int client_sock, const json &request);
void handle_send_message(int client_sock, const json &request);
void handle_send_group_message(int client_sock, const json &request);
void handle_get_friend_request(int client_sock, const json &request);
void handle_send_friend_request(int client_sock, const json &request);
void handle_accept_friend_request(int client_sock, const json &request);
void handle_reject_friend_request(int client_sock, const json &request);
void handle_unfriend(int client_sock, const json &request);
void handle_get_friend_list(int client_sock, const json &request);
void handle_create_group(int client_sock, const json &request);
void handle_add_to_group(int client_sock, const json &request);
void handle_remove_from_group(int client_sock, const json &request);
void handle_get_group_list(int client_sock, const json &request);
void handle_leave_group(int client_sock, const json &request);
void handle_get_offline_messages(int client_sock, const json &request);
void handle_get_group_messages(int client_sock, const json &request);

void handle_get_message_history(int client_sock, const json& request);

// Helper function to find session by socket
Session *find_session_by_socket(int socket_fd)
{
    for (auto &session : active_sessions)
    {
        if (session.socket_fd == socket_fd)
        {
            return &session;
        }
    }
    return nullptr;
}

// Send JSON response helper
void send_response(int client_sock, int status, const std::string &message)
{
    json response = {
        {"type", 2000},
        {"data", {{"status", status}, {"message", message}}}};

    std::string response_str = response.dump();
    send_json_packet(client_sock, response_str.c_str());
}

// Process client requests
void process_client_request(int client_sock, const json &request)
{
    try
    {
        int type = request["type"];

        std::cout << "[SERVER] Processing request type: " << type << std::endl;

        switch (type)
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
        case 1202: // SEND_GROUP_MESSAGE
            handle_send_group_message(client_sock, request);
            break;
        case 1300: // SEND_FRIEND_REQUEST
            handle_get_friend_request(client_sock, request);
            break;
        case 1301: // SEND_FRIEND_REQUEST
            handle_send_friend_request(client_sock, request);
            break;
        case 1302: // ACCEPT_FRIEND_REQUEST
            handle_accept_friend_request(client_sock, request);
            break;
        case 1303: // REJECT_FRIEND_REQUEST
            handle_reject_friend_request(client_sock, request);
            break;
        case 1304: // UNFRIEND
            handle_unfriend(client_sock, request);
            break;
        case 1305: // GET_FRIEND_LIST
            handle_get_friend_list(client_sock, request);
            break;
        case 1400: // CREATE_GROUP
            handle_create_group(client_sock, request);
            break;
        case 1401: // ADD_TO_GROUP
            handle_add_to_group(client_sock, request);
            break;
        case 1402: // REMOVE_FROM_GROUP
            handle_remove_from_group(client_sock, request);
            break;
        case 1403: // LEAVE_GROUP
            handle_leave_group(client_sock, request);
            break;
        case 1404: // GET_GROUP_LIST
            handle_get_group_list(client_sock, request);
            break;
        case 1405: // GET_GROUP_MESSAGES
            handle_get_group_messages(client_sock, request);
            break;
        case 1014: // GET_OFFLINE_MESSAGES
            handle_get_offline_messages(client_sock, request);
            break;
        default:
            send_response(client_sock, 400, "Unknown request type");
            break;
        }
    }
    catch (json::exception &e)
    {
        std::cerr << "[SERVER] JSON error: " << e.what() << std::endl;
        send_response(client_sock, 400, "Invalid JSON format");
    }
}

// Client handler thread
void *client_handler(void *arg)
{
    int client_sock = *(int *)arg;
    delete (int *)arg;

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
        catch (json::exception &e)
        {
            std::cerr << "[SERVER] JSON parse error: " << e.what() << std::endl;
            send_response(client_sock, 400, "Invalid JSON");
        }
    }

    // Cleanup: remove from sessions and update database
    pthread_mutex_lock(&sessions_mutex);
    for (auto it = active_sessions.begin(); it != active_sessions.end(); ++it)
    {
        if (it->socket_fd == client_sock)
        {
            int user_id = it->user_id;
            std::string username = it->username;
            
            active_sessions.erase(it);
            pthread_mutex_unlock(&sessions_mutex);
            
            // Update user state to offline in database
            update_user_state(g_db, user_id, "offline");
            
            std::cout << "[SERVER] Client disconnected without logout. Updated database: " 
                      << username << " (ID: " << user_id << ") set to offline" << std::endl;
            
            close(client_sock);
            log_activity(g_db, user_id, ACT_LOGOUT, -1, RESP_SUCCESS);
            return nullptr;
        }
    }
    pthread_mutex_unlock(&sessions_mutex);

    close(client_sock);
    return nullptr;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    // Initialize database
    g_db = init_database("database/chat.db");
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

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
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
        int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len);

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
        int *client_sock_ptr = new int(client_sock);

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
    close_database(g_db);
    return 0;
}

// ============================================================================
// PLACEHOLDER IMPLEMENTATIONS - To be implemented later
// ============================================================================

void handle_register(int client_sock, const json &request)
{
    try
    {
        std::string username = request["data"]["username"];
        std::string password = request["data"]["password"];

        std::cout << "[REGISTER] Request from username: " << username << std::endl;

        int user_id;
        int result = register_user(g_db, username, password, &user_id);

        if (result == DB_USER_EXISTS)
        {
            send_response(client_sock, 409, "Username already exists");
            log_activity(g_db, -1, ACT_REGISTER, -1, ERR_USER_EXISTS);
        }
        else if (result == DB_ERROR)
        {
            send_response(client_sock, 500, "Database error");
            log_activity(g_db, -1, ACT_REGISTER, -1, ERR_DATABASE_ERROR);
        }
        else
        {
            // Log registration activity
            log_activity(g_db, user_id, ACT_REGISTER, -1, RESP_CREATED);

            json response = {
                {"type", 2000},
                {"data", {{"status", 201}, {"message", "Registration successful"}, {"user_id", user_id}}}};
            send_json_packet(client_sock, response.dump().c_str());

            std::cout << "[REGISTER] Success: " << username << " (ID: " << user_id << ")" << std::endl;
        }
    }
    catch (json::exception &e)
    {
        std::cerr << "[REGISTER] JSON error: " << e.what() << std::endl;
        send_response(client_sock, 400, "Invalid request format");
        log_activity(g_db, -1, ACT_REGISTER, -1, ERR_INVALID_REQUEST);
    }
}

void handle_login(int client_sock, const json &request)
{
    try
    {
        std::string username = request["data"]["username"];
        std::string password = request["data"]["password"];

        std::cout << "[LOGIN] Request from username: " << username << std::endl;

        int user_id;
        int result = verify_login(g_db, username, password, &user_id);

        if (result == DB_USER_NOT_FOUND)
        {
            send_response(client_sock, 404, "User not found");
            log_activity(g_db, -1, ACT_LOGIN, -1, ERR_USER_NOT_FOUND);
        }
        else if (result == DB_INVALID_PASSWORD)
        {
            send_response(client_sock, 401, "Invalid password");
            log_activity(g_db, user_id, ACT_LOGIN, -1, ERR_WRONG_PASSWORD);
        }
        else if (result == DB_USER_BANNED)
        {
            send_response(client_sock, 403, "Account is banned");
            log_activity(g_db, user_id, ACT_LOGIN, -1, ERR_USER_BANNED);
        }
        else if (result == DB_ERROR)
        {
            send_response(client_sock, 500, "Database error");
            log_activity(g_db, user_id, ACT_LOGIN, -1, ERR_DATABASE_ERROR);
        }
        else
        {
            pthread_mutex_lock(&sessions_mutex);

            // Check if this socket already has a session
            if (find_session_by_socket(client_sock) != nullptr)
            {
                pthread_mutex_unlock(&sessions_mutex);
                send_response(client_sock, 409, "Already logged in. Please logout first");
                log_activity(g_db, user_id, ACT_LOGIN, -1, ERR_ALREADY_LOGGED_IN);
                return;
            }

            // Check if user is already logged in from another connection
            bool already_logged_in = false;
            for (const auto &session : active_sessions)
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
                log_activity(g_db, user_id, ACT_LOGIN, -1, ERR_USER_LOGGED_IN_ELSEWHERE);
                return;
            }

            // Create session
            Session new_session = {client_sock, user_id, username};
            active_sessions.push_back(new_session);
            pthread_mutex_unlock(&sessions_mutex);

            // Update user state to online
            update_user_state(g_db, user_id, "online");

            // Log login activity
            log_activity(g_db, user_id, ACT_LOGIN, -1, RESP_SUCCESS);

            json response = {
                {"type", 2000},
                {"data", {{"status", 200}, {"message", "Login successful"}, {"user_id", user_id}, {"username", username}}}};
            send_json_packet(client_sock, response.dump().c_str());
            
            // Send all unread offline messages
            std::vector<Message> unread_messages;
            if (get_unread_messages(g_db, user_id, unread_messages) == DB_SUCCESS) {
                if (!unread_messages.empty()) {
                    std::cout << "[LOGIN] Sending " << unread_messages.size() 
                              << " unread messages to " << username << std::endl;
                    
                    for (const auto& msg : unread_messages) {
                        json notification = {
                            {"type", 2001},  // MESSAGE_RECEIVED
                            {"data", {
                                {"sender_id", msg.sender_id},
                                {"sender_username", msg.sender_username},
                                {"content", msg.content},
                                {"timestamp", msg.timestamp}
                            }}
                        };
                        send_json_packet(client_sock, notification.dump().c_str());
                        
                    }
                }
            }

            std::cout << "[LOGIN] Success: " << username << " (ID: " << user_id << ")" << std::endl;
        }
    }
    catch (json::exception &e)
    {
        std::cerr << "[LOGIN] JSON error: " << e.what() << std::endl;
        send_response(client_sock, 400, "Invalid request format");
        log_activity(g_db, -1, ACT_LOGIN, -1, ERR_INVALID_REQUEST);
    }
}

void handle_logout(int client_sock, const json &request)
{
    std::cout << "[LOGOUT] Request from socket " << client_sock << std::endl;

    pthread_mutex_lock(&sessions_mutex);

    // Find and remove session
    auto it = active_sessions.begin();
    while (it != active_sessions.end())
    {
        if (it->socket_fd == client_sock)
        {
            int user_id = it->user_id;
            std::string username = it->username;

            active_sessions.erase(it);
            pthread_mutex_unlock(&sessions_mutex);

            // Update user state to offline
            update_user_state(g_db, user_id, "offline");

            // Log logout activity
            log_activity(g_db, user_id, ACT_LOGOUT, -1, RESP_SUCCESS);

            send_response(client_sock, 200, "Logout successful");

            std::cout << "[LOGOUT] Success: " << username << " (ID: " << user_id << ")" << std::endl;
            return;
        }
        ++it;
    }

    pthread_mutex_unlock(&sessions_mutex);
    send_response(client_sock, 401, "Not logged in");
    log_activity(g_db, -1, ACT_LOGOUT, -1, ERR_NOT_LOGGED_IN);
}

void handle_send_message(int client_sock, const json &request)
{
    Session* session = find_session_by_socket(client_sock);
    if (!session) {
        send_response(client_sock, 401, "You are not logged in");
        log_activity(g_db, -1, ACT_SEND_MESSAGE, -1, ERR_NOT_LOGGED_IN);
        return;
    }

    if (!request.contains("data") || !request["data"].contains("receiver_id") || !request["data"].contains("content")) {
        send_response(client_sock, 400, "Missing receiver_id or content");
        log_activity(g_db, session->user_id, ACT_SEND_MESSAGE, -1, ERR_INVALID_REQUEST);
        return;
    }

    int sender_id = session->user_id;
    int receiver_id = request["data"]["receiver_id"];
    std::string content = request["data"]["content"];

    int result = send_direct_message(g_db, sender_id, receiver_id, content);

    if (result == DB_SUCCESS) {
        send_response(client_sock, 200, "Message sent successfully");
        log_activity(g_db, sender_id, ACT_SEND_MESSAGE, receiver_id, RESP_SUCCESS);
        // Check if receiver is online
        int receiver_sock = -1;
        pthread_mutex_lock(&sessions_mutex);
        for (const auto& s : active_sessions) {
            if (s.user_id == receiver_id) {
                receiver_sock = s.socket_fd;
                break;
            }
        }
        pthread_mutex_unlock(&sessions_mutex);

        if (receiver_sock != -1) {
            // Send push notification
            json notification = {
                {"type", 2001},
                {"data", {
                    {"sender_id", sender_id},
                    {"sender_username", session->username},
                    {"content", content},
                    {"timestamp", std::to_string(std::time(nullptr))}
                }}
            };
            std::string notif_str = notification.dump();
            send_json_packet(receiver_sock, notif_str.c_str());
        }
    } else if (result == DB_NOT_FRIENDS_TO_SEND) {
        send_response(client_sock, 403, "You are not friends with this user");
        log_activity(g_db, sender_id, ACT_SEND_MESSAGE, receiver_id, ERR_NOT_FRIEND);
    } else {
        send_response(client_sock, 500, "Failed to send message");
        log_activity(g_db, sender_id, ACT_SEND_MESSAGE, receiver_id, ERR_DATABASE_ERROR);
    }
}

void handle_get_message_history(int client_sock, const json& request)
{
    Session* session = find_session_by_socket(client_sock);
    if (!session) {
        send_response(client_sock, 401, "You are not logged in");
        log_activity(g_db, -1, ACT_GET_DIRECT_MESSAGES, -1, ERR_NOT_LOGGED_IN);
        return;
    }

    if (!request.contains("data") || !request["data"].contains("target_id")) {
        send_response(client_sock, 400, "Missing target_id");
        log_activity(g_db, session->user_id, ACT_GET_DIRECT_MESSAGES, -1, ERR_INVALID_REQUEST);
        return;
    }

    int user_id1 = session->user_id;
    int user_id2 = request["data"]["target_id"];
    int limit = request["data"]["limit"];
    int offset = request["data"].value("offset", 0);

    std::vector<Message> messages;
    int result = get_direct_message_history(g_db, user_id1, user_id2, messages, limit, offset);

    if (result == DB_SUCCESS) {
        json response;
        response["type"] = 2005; // MESSAGE_HISTORY
        response["data"]["messages"] = json::array();
        
        // Mark messages as read
        mark_messages_read(g_db, user_id1, user_id2);

        for (const auto& msg : messages) {
            response["data"]["messages"].push_back({
                {"id", msg.message_id},
                {"sender_id", msg.sender_id},
                {"content", msg.content},
                {"timestamp", msg.timestamp},
                {"is_read", msg.is_read}
            });
        }
        std::string response_str = response.dump();
        send_json_packet(client_sock, response_str.c_str());
        log_activity(g_db, user_id1, ACT_GET_DIRECT_MESSAGES, user_id2, RESP_SUCCESS);
    } else if (result == DB_NOT_FRIENDS_TO_SEND) {
        send_response(client_sock, 403, "You are not friends with this user");
        log_activity(g_db, user_id1, ACT_GET_DIRECT_MESSAGES, user_id2, ERR_NOT_FRIEND);
    } else {
        send_response(client_sock, 500, "Failed to retrieve messages");
        log_activity(g_db, user_id1, ACT_GET_DIRECT_MESSAGES, user_id2, ERR_DATABASE_ERROR);
    }
}

void handle_send_group_message(int client_sock, const json &request)
{
    // 1. Get session of sender
    Session *session = find_session_by_socket(client_sock);
    if (!session)
    {
        send_response(client_sock, 401, "You are not logged in");
        log_activity(g_db, -1, ACT_SEND_GROUP_MESSAGE, -1, ERR_NOT_LOGGED_IN);
        return;
    }

    int sender_id = session->user_id;

    // 2. Validate request parameters
    if (!request.contains("data") ||
        !request["data"].contains("group_name") ||
        !request["data"].contains("content"))
    {
        send_response(client_sock, 400, "Invalid request format: Missing group_name or content");
        log_activity(g_db, sender_id, ACT_SEND_GROUP_MESSAGE, -1, ERR_INVALID_REQUEST);
        return;
    }

    std::string group_name = request["data"]["group_name"];
    std::string content = request["data"]["content"];

    // 3. Get group ID from group name
    int group_id = get_group_id_by_name(g_db, group_name);
    if (group_id == -1)
    {
        send_response(client_sock, 404, "Group not found");
        log_activity(g_db, sender_id, ACT_SEND_GROUP_MESSAGE, -1, ERR_GROUP_NOT_FOUND);
        return;
    }

    // 4. Check if sender is a member of the group
    if (!is_group_member(g_db, group_id, sender_id))
    {
        send_response(client_sock, 403, "You are not a member of this group");
        log_activity(g_db, sender_id, ACT_SEND_GROUP_MESSAGE, group_id, ERR_NOT_GROUP_MEMBER);
        return;
    }

    if (!send_group_message(g_db, sender_id, group_id, content)){
        send_response(client_sock, 500, "Failed to send message");
        log_activity(g_db, sender_id, ACT_SEND_GROUP_MESSAGE, group_id, ERR_DATABASE_ERROR);
        return;
    };

    send_response(client_sock, 200, "Message sent successfully");
    log_activity(g_db, sender_id, ACT_SEND_GROUP_MESSAGE, group_id, RESP_SUCCESS);
}

void handle_get_friend_request(int client_sock, const json &request)
{
    // 1. Lấy session người gửi
    Session *session = find_session_by_socket(client_sock);
    if (!session)
    {
        send_response(client_sock, 401, "You are not logged in");
        log_activity(g_db, -1, ACT_GET_FRIEND_REQUESTS, -1, ERR_NOT_LOGGED_IN);
        return;
    }
    int receiver_id = session->user_id;
    auto requests = get_all_friend_requests(g_db, receiver_id);
    json response;
    response["type"] = 2003;
    response["data"]["friend_requests"] = json::array();

    for (const auto &req : requests)
    {
        response["data"]["friend_requests"].push_back({{"username", req.sender_username},
                                                       {"timestamp", req.timestamp}});
    }

    std::string response_str = response.dump();
    send_json_packet(client_sock, response_str.c_str());
    log_activity(g_db, receiver_id, ACT_GET_FRIEND_REQUESTS, -1, RESP_SUCCESS);
}

void handle_send_friend_request(int client_sock, const json &request)
{
    // 1. Lấy session người gửi
    Session *senderSession = find_session_by_socket(client_sock);

    if (!senderSession)
    {
        send_response(client_sock, 401, "Session not found");
        log_activity(g_db, -1, ACT_SEND_FRIEND_REQUEST, -1, ERR_NOT_LOGGED_IN);
        return;
    }

    int senderId = senderSession->user_id;

    // 2. Lấy username người nhận từ JSON
    if (!request.contains("data") || !request["data"].contains("target_username"))
    {
        send_response(client_sock, 400, "Missing receiver username");
        log_activity(g_db, senderId, ACT_SEND_FRIEND_REQUEST, -1, ERR_INVALID_REQUEST);
        return;
    }

    std::string receiverUsername = request["data"]["target_username"];

    // 3. Lấy user id từ DB
    int receiverId = get_user_id_by_username(g_db, receiverUsername);

    if (receiverId == -1)
    {
        send_response(client_sock, 404, "Receiver not found");
        log_activity(g_db, senderId, ACT_SEND_FRIEND_REQUEST, receiverId, ERR_USER_NOT_FOUND);
        return;
    }

    if (senderId == receiverId)
    {
        send_response(client_sock, 400, "You cannot send friend request to yourself");
        log_activity(g_db, senderId, ACT_SEND_FRIEND_REQUEST, receiverId, ERR_SELF_REQUEST);
        return;
    }

    // 4. Kiểm tra đã gửi chưa
    if (friend_request_exists(g_db, senderId, receiverId))
    {
        send_response(client_sock, 409, "Friend request already exists");
        log_activity(g_db, senderId, ACT_SEND_FRIEND_REQUEST, receiverId, ERR_FRIEND_REQUEST_EXISTS);
        return;
    }

    if (friendship_exists(g_db, senderId, receiverId))
    {
        send_response(client_sock, 409, "Already friend");
        log_activity(g_db, senderId, ACT_SEND_FRIEND_REQUEST, receiverId, ERR_ALREADY_FRIEND);
        return;
    }

    // 5. Thêm vào database
    if (!add_friend_request(g_db, senderId, receiverId))
    {
        send_response(client_sock, 500, "Failed to create friend request");
        log_activity(g_db, senderId, ACT_SEND_FRIEND_REQUEST, receiverId, ERR_DATABASE_ERROR);
        return;
    }

    // 6. Thành công
    send_response(client_sock, 200, "Friend request sent successfully");
    log_activity(g_db, senderId, ACT_SEND_FRIEND_REQUEST, receiverId, RESP_SUCCESS);
}

void handle_accept_friend_request(int client_sock, const json &request)
{
    // 1. Lấy session
    Session *session = find_session_by_socket(client_sock);
    if (!session)
    {
        send_response(client_sock, 401, "You are not authenticated");
        log_activity(g_db, -1, ACT_ACCEPT_FRIEND_REQUEST, -1, ERR_NOT_LOGGED_IN);
        return;
    }

    int receiver_id = session->user_id; // người accept
    std::string receiver_username = session->username;

    // 2. Lấy username của người gửi lời mời
    if (!request.contains("data") ||
        !request["data"].contains("target_username"))
    {
        send_response(client_sock, 400, "Invalid request format");
        log_activity(g_db, receiver_id, ACT_ACCEPT_FRIEND_REQUEST, -1, ERR_INVALID_REQUEST);
        return;
    }

    std::string sender_username = request["data"]["target_username"];

    // 3. Lấy sender_id từ username
    int sender_id = get_user_id_by_username(g_db, sender_username);
    if (sender_id == -1)
    {
        send_response(client_sock, 404, "User not found");
        log_activity(g_db, receiver_id, ACT_ACCEPT_FRIEND_REQUEST, -1, ERR_USER_NOT_FOUND);
        return;
    }

    // 4. Kiểm tra đã là bạn bè chưa
    if (friendship_exists(g_db, sender_id, receiver_id))
    {
        send_response(client_sock, 409, "You are already friends");
        log_activity(g_db, receiver_id, ACT_ACCEPT_FRIEND_REQUEST, sender_id, ERR_ALREADY_FRIEND);
        return;
    }

    // 5. Kiểm tra có request tồn tại không (sender -> receiver)
    if (!friend_request_exists(g_db, sender_id, receiver_id))
    {
        send_response(client_sock, 404, "Friend request not found");
        log_activity(g_db, receiver_id, ACT_ACCEPT_FRIEND_REQUEST, sender_id, ERR_FRIEND_REQUEST_NOT_FOUND);
        return;
    }

    // 6. Thêm vào bảng friends
    if (!add_friendship(g_db, sender_id, receiver_id))
    {
        send_response(client_sock, 500, "Failed to add friend");
        log_activity(g_db, receiver_id, ACT_ACCEPT_FRIEND_REQUEST, sender_id, ERR_DATABASE_ERROR);
        return;
    }

    // 7. Xóa friend request
    if (!remove_friend_request(g_db, sender_id, receiver_id))
    {
        send_response(client_sock, 500, "Failed to delete friend request");
        log_activity(g_db, receiver_id, ACT_ACCEPT_FRIEND_REQUEST, sender_id, ERR_DATABASE_ERROR);

        return;
    }

    send_response(client_sock, 200, "Friend added");
    log_activity(g_db, receiver_id, ACT_ACCEPT_FRIEND_REQUEST, sender_id, RESP_SUCCESS);
}

void handle_reject_friend_request(int client_sock, const json &request)
{
    // 1. Lấy session
    Session *session = find_session_by_socket(client_sock);
    if (!session)
    {
        send_response(client_sock, 401, "You are not authenticated");
        log_activity(g_db, -1, ACT_ACCEPT_FRIEND_REQUEST, -1, ERR_NOT_LOGGED_IN);
        return;
    }

    int receiver_id = session->user_id; // người accept
    std::string receiver_username = session->username;

    // 2. Lấy username của người gửi lời mời
    if (!request.contains("data") ||
        !request["data"].contains("target_username"))
    {
        send_response(client_sock, 400, "Invalid request format");
        log_activity(g_db, receiver_id, ACT_REJECT_FRIEND_REQUEST, -1, ERR_INVALID_REQUEST);
        return;
    }

    std::string sender_username = request["data"]["target_username"];

    // 3. Lấy sender_id từ username
    int sender_id = get_user_id_by_username(g_db, sender_username);
    if (sender_id == -1)
    {
        send_response(client_sock, 404, "User not found");
        log_activity(g_db, receiver_id, ACT_REJECT_FRIEND_REQUEST, -1, ERR_USER_NOT_FOUND);
        return;
    }

    // 4. Kiểm tra đã là bạn bè chưa
    if (friendship_exists(g_db, sender_id, receiver_id))
    {
        send_response(client_sock, 409, "You are already friends");
        log_activity(g_db, receiver_id, ACT_REJECT_FRIEND_REQUEST, sender_id, ERR_ALREADY_FRIEND);
        return;
    }

    // 5. Kiểm tra có request tồn tại không (sender -> receiver)
    if (!friend_request_exists(g_db, sender_id, receiver_id))
    {
        send_response(client_sock, 404, "Friend request not found");
        log_activity(g_db, receiver_id, ACT_REJECT_FRIEND_REQUEST, sender_id, ERR_FRIEND_REQUEST_NOT_FOUND);
        return;
    }

    // 7. Xóa friend request
    if (!remove_friend_request(g_db, sender_id, receiver_id))
    {
        send_response(client_sock, 500, "Failed to delete friend request");
        log_activity(g_db, receiver_id, ACT_REJECT_FRIEND_REQUEST, sender_id, ERR_DATABASE_ERROR);
        return;
    }

    send_response(client_sock, 200, "Friend request rejected");
    log_activity(g_db, receiver_id, ACT_REJECT_FRIEND_REQUEST, sender_id, RESP_SUCCESS);
}

void handle_unfriend(int client_sock, const json &request)
{
    // 1. Lấy session
    Session *session = find_session_by_socket(client_sock);
    if (!session)
    {
        send_response(client_sock, 401, "You are not authenticated");
        log_activity(g_db, -1, ACT_UNFRIEND, -1, ERR_NOT_LOGGED_IN);
        return;
    }

    int user_id1 = session->user_id;

    if (!request.contains("data") ||
        !request["data"].contains("target_username"))
    {
        send_response(client_sock, 400, "Invalid request format");
        log_activity(g_db, user_id1, ACT_UNFRIEND, -1, ERR_INVALID_REQUEST);
        return;
    }

    std::string target_username = request["data"]["target_username"];

    // 3. Lấy sender_id từ username
    int user_id2 = get_user_id_by_username(g_db, target_username);
    if (user_id2 == -1)
    {
        send_response(client_sock, 404, "User not found");
        log_activity(g_db, user_id1, ACT_UNFRIEND, -1, ERR_USER_NOT_FOUND);
        return;
    }

    // 4. Kiểm tra đã là bạn bè chưa
    if (!friendship_exists(g_db, user_id1, user_id2))
    {
        send_response(client_sock, 409, "You are not friends");
        log_activity(g_db, user_id1, ACT_UNFRIEND, user_id2, ERR_NOT_FRIEND);
        return;
    }

    if (!remove_friendship(g_db, user_id1, user_id2))
    {
        send_response(client_sock, 500, "Failed to unfriend");
        log_activity(g_db, user_id1, ACT_UNFRIEND, user_id2, ERR_DATABASE_ERROR);
        return;
    }

    send_response(client_sock, 200, "Unfriend successfully");
    log_activity(g_db, user_id1, ACT_UNFRIEND, user_id2, RESP_SUCCESS);
}

void handle_get_friend_list(int client_sock, const json &request)
{
    Session *session = find_session_by_socket(client_sock);
    if (!session)
    {
        send_response(client_sock, 401, "You are not logged in");
        log_activity(g_db, -1, ACT_GET_FRIEND_LIST, -1, ERR_NOT_LOGGED_IN);
        return;
    }
    int user_id = session->user_id;
    auto friends = get_all_friends(g_db, user_id);

    json response;
    response["type"] = 2004;
    response["data"]["friends"] = json::array();

    for (const auto &f : friends)
    {
        response["data"]["friends"].push_back({{"id", f.id},
                                               {"username", f.username},
                                               {"user_state", f.status}});
    }
    std::string response_str = response.dump();
    send_json_packet(client_sock, response_str.c_str());
    log_activity(g_db, user_id, ACT_GET_FRIEND_LIST, -1, RESP_SUCCESS);
}

void handle_create_group(int client_sock, const json &request)
{
    // 1. Lấy session người tạo
    Session *session = find_session_by_socket(client_sock);
    if (!session)
    {
        send_response(client_sock, 401, "You are not logged in");
        log_activity(g_db, -1, ACT_CREATE_GROUP, -1, ERR_NOT_LOGGED_IN);
        return;
    }

    int creator_id = session->user_id;

    // 2. Lấy tên nhóm từ JSON
    if (!request.contains("data") ||
        !request["data"].contains("group_name"))
    {
        send_response(client_sock, 400, "Invalid request format: Missing group_name");
        log_activity(g_db, creator_id, ACT_CREATE_GROUP, -1, ERR_INVALID_REQUEST);
        return;
    }

    std::string group_name = request["data"]["group_name"];

    std::cout << "[CREATE_GROUP] Request from user ID " << creator_id << " for group: " << group_name << std::endl;

    if (create_group(g_db, group_name, creator_id))
    {
        send_response(client_sock, 200, "Group created successfully");
        log_activity(g_db, creator_id, ACT_CREATE_GROUP, -1, RESP_SUCCESS);
        std::cerr << "[CREATE_GROUP] Success" << std::endl;
    }
    else
    {
        send_response(client_sock, 500, "Failed to create group (database error)");
        log_activity(g_db, creator_id, ACT_CREATE_GROUP, -1, ERR_DATABASE_ERROR);
        std::cerr << "[CREATE_GROUP] Failure: Database operation failed" << std::endl;
    }
}

void handle_add_to_group(int client_sock, const json &request)
{
    // 1. Lấy session người yêu cầu (phải là owner/admin)
    Session *session = find_session_by_socket(client_sock);
    if (!session)
    {
        send_response(client_sock, 401, "You are not logged in");
        log_activity(g_db, -1, ACT_ADD_TO_GROUP, -1, ERR_NOT_LOGGED_IN);
        return;
    }

    int requester_id = session->user_id; // Người thực hiện hành động thêm

    // 2. Kiểm tra và lấy tham số từ JSON
    if (!request.contains("data") ||
        !request["data"].contains("group_name") ||
        !request["data"].contains("target_username"))
    {
        send_response(client_sock, 400, "Invalid request format: Missing group_name or target_username");
        log_activity(g_db, requester_id, ACT_ADD_TO_GROUP, -1, ERR_INVALID_REQUEST);
        return;
    }

    std::string group_name = request["data"]["group_name"];
    std::string target_username = request["data"]["target_username"];

    // 3. Lấy Group ID
    int group_id = get_group_id_by_name(g_db, group_name);
    if (group_id == -1)
    {
        send_response(client_sock, 404, "Group not found");
        log_activity(g_db, requester_id, ACT_ADD_TO_GROUP, group_id, ERR_GROUP_NOT_FOUND);
        return;
    }

    // 4. Lấy Target User ID
    int target_user_id = get_user_id_by_username(g_db, target_username);
    if (target_user_id == -1)
    {
        send_response(client_sock, 404, "Target user not found");
        log_activity(g_db, requester_id, ACT_ADD_TO_GROUP, target_user_id, ERR_USER_NOT_FOUND);
        return;
    }

    // 5. Kiểm tra quyền (Chỉ Owner mới được thêm thành viên - Có thể mở rộng là Admin)
    if (!is_group_owner(g_db, group_id, requester_id))
    {
        send_response(client_sock, 403, "Permission denied: Only the group owner can add members");
        log_activity(g_db, requester_id, ACT_ADD_TO_GROUP, group_id, ERR_NOT_GROUP_OWNER);
        return;
    }

    // 6. Thêm thành viên vào nhóm
    if (add_group_member(g_db, group_id, target_user_id))
    {
        send_response(client_sock, 200, "User added to group successfully");
        log_activity(g_db, requester_id, ACT_ADD_TO_GROUP, group_id, RESP_SUCCESS);
    }
    else
    {
        send_response(client_sock, 500, "Failed to add user to group (database error)");
        log_activity(g_db, requester_id, ACT_ADD_TO_GROUP, group_id, ERR_DATABASE_ERROR);
    }
}

void handle_remove_from_group(int client_sock, const json &request)
{
    // 1. Lấy session người yêu cầu (requester)
    Session *session = find_session_by_socket(client_sock);
    if (!session)
    {
        send_response(client_sock, 401, "You are not logged in");
        log_activity(g_db, -1, ACT_REMOVE_FROM_GROUP, -1, ERR_NOT_LOGGED_IN);
        return;
    }
    int requester_id = session->user_id;

    // 2. Kiểm tra và lấy tham số từ JSON
    if (!request.contains("data") ||
        !request["data"].contains("group_name") ||
        !request["data"].contains("target_username"))
    {
        send_response(client_sock, 400, "Invalid request format: Missing group_name or target_username");
        log_activity(g_db, requester_id, ACT_REMOVE_FROM_GROUP, -1, ERR_INVALID_REQUEST);
        return;
    }

    std::string group_name = request["data"]["group_name"];
    std::string target_username = request["data"]["target_username"];

    // 3. Lấy Group ID và Target User ID
    int group_id = get_group_id_by_name(g_db, group_name);
    if (group_id == -1)
    {
        send_response(client_sock, 404, "Group not found");
        log_activity(g_db, requester_id, ACT_REMOVE_FROM_GROUP, group_id, ERR_GROUP_NOT_FOUND);
        return;
    }

    int target_user_id = get_user_id_by_username(g_db, target_username);
    if (target_user_id == -1)
    {
        send_response(client_sock, 404, "Target user not found");
        log_activity(g_db, requester_id, ACT_REMOVE_FROM_GROUP, target_user_id, ERR_USER_NOT_FOUND);
        return;
    }

    // Kiểm tra xem người dùng có đang cố tự xóa mình không
    if (requester_id == target_user_id)
    {
        send_response(client_sock, 400, "You cannot remove yourself. Please use the LEAVE_GROUP command.");
        log_activity(g_db, requester_id, ACT_REMOVE_FROM_GROUP, target_user_id, ERR_CANNOT_REMOVE_SELF);
        return;
    }

    // 4. Kiểm tra quyền của người yêu cầu
    if (!is_group_owner(g_db, group_id, requester_id))
    {
        send_response(client_sock, 403, "Permission denied: Only group admin or owner can remove members");
        log_activity(g_db, requester_id, ACT_REMOVE_FROM_GROUP, group_id, ERR_NOT_GROUP_OWNER);
        return;
    }

    // 5. Kiểm tra Target User có phải là thành viên nhóm không
    if (!is_group_member(g_db, group_id, target_user_id))
    {
        send_response(client_sock, 404, "Target user is not a member of this group");
        log_activity(g_db, requester_id, ACT_REMOVE_FROM_GROUP, target_user_id, ERR_NOT_GROUP_MEMBER);
        return;
    }

    // 7. Thực hiện xóa thành viên
    if (remove_group_member(g_db, group_id, target_user_id))
    {
        send_response(client_sock, 200, "User removed from group successfully");
        log_activity(g_db, requester_id, ACT_REMOVE_FROM_GROUP, group_id, RESP_SUCCESS);
    }
    else
    {
        send_response(client_sock, 500, "Failed to remove user from group (database error)");
        log_activity(g_db, requester_id, ACT_REMOVE_FROM_GROUP, group_id, ERR_DATABASE_ERROR);
    }
}

void handle_leave_group(int client_sock, const json &request)
{
    // 1. Lấy session người yêu cầu (requester)
    Session *session = find_session_by_socket(client_sock);
    if (!session)
    {
        send_response(client_sock, 401, "You are not logged in");
        log_activity(g_db, -1, ACT_LEAVE_GROUP, -1, ERR_NOT_LOGGED_IN);
        return;
    }
    int user_id = session->user_id;

    // 2. Kiểm tra và lấy tên nhóm từ JSON
    if (!request.contains("data") ||
        !request["data"].contains("group_name"))
    {
        send_response(client_sock, 400, "Invalid request format: Missing group_name");
        log_activity(g_db, user_id, ACT_LEAVE_GROUP, -1, ERR_INVALID_REQUEST);
        return;
    }

    std::string group_name = request["data"]["group_name"];

    // 3. Lấy Group ID
    int group_id = get_group_id_by_name(g_db, group_name);
    if (group_id == -1)
    {
        send_response(client_sock, 404, "Group not found");
        log_activity(g_db, user_id, ACT_LEAVE_GROUP, -1, ERR_GROUP_NOT_FOUND);
        return;
    }

    // 4. Kiểm tra xem người dùng có phải là thành viên không
    if (!is_group_member(g_db, group_id, user_id))
    {
        send_response(client_sock, 404, "You are not a member of this group");
        log_activity(g_db, user_id, ACT_LEAVE_GROUP, group_id, ERR_NOT_GROUP_MEMBER);
        return;
    }

    // 5. Kiểm tra quyền sở hữu
    if (is_group_owner(g_db, group_id, user_id))
    {
        // 5a. Nếu là OWNER, xóa toàn bộ nhóm
        if (delete_group(g_db, group_id))
        {
            send_response(client_sock, 200, "You left the group and the group was dissolved (Owner left)");
            log_activity(g_db, user_id, ACT_LEAVE_GROUP, group_id, RESP_SUCCESS);
        }
        else
        {
            send_response(client_sock, 500, "Failed to dissolve the group");
            log_activity(g_db, user_id, ACT_LEAVE_GROUP, group_id, ERR_DATABASE_ERROR);
        }
    }
    else
    {
        // 5b. Nếu không phải là OWNER, chỉ xóa thành viên
        if (remove_group_member(g_db, group_id, user_id))
        {
            send_response(client_sock, 200, "You left the group successfully");
            log_activity(g_db, user_id, ACT_LEAVE_GROUP, group_id, RESP_SUCCESS);
        }
        else
        {
            send_response(client_sock, 500, "Failed to leave the group");
            log_activity(g_db, user_id, ACT_LEAVE_GROUP, group_id, ERR_DATABASE_ERROR);
        }
    }
}

void handle_get_group_list(int client_sock, const json &request)
{
    Session *session = find_session_by_socket(client_sock);
    if (!session)
    {
        send_response(client_sock, 401, "You are not logged in");
        log_activity(g_db, -1, ACT_GET_GROUP_LIST, -1, ERR_NOT_LOGGED_IN);
        return;
    }
    int user_id = session->user_id;
    auto groups = get_all_group(g_db, user_id);

    json response;
    response["type"] = 2400;
    response["data"]["groups"] = json::array();

    for (const auto &g : groups)
    {
        response["data"]["groups"].push_back({{"id", g.id},
                                               {"group_name", g.username},
                                               {"role", g.role}});
    }
    std::string response_str = response.dump();
    send_json_packet(client_sock, response_str.c_str());
    log_activity(g_db, user_id, ACT_GET_GROUP_LIST, -1, RESP_SUCCESS);
}

void handle_get_group_messages(int client_sock, const json &request)
{
    Session *session = find_session_by_socket(client_sock);
    if (!session)
    {
        send_response(client_sock, 401, "You are not logged in");
        log_activity(g_db, -1, ACT_GET_GROUP_MESSAGES, -1, ERR_NOT_LOGGED_IN);
        return;
    }

    int user_id = session->user_id;

    if (!request.contains("data") || !request["data"].contains("group_name"))
    {
        send_response(client_sock, 400, "Invalid request format: Missing group_name");
        log_activity(g_db, user_id, ACT_GET_GROUP_MESSAGES, -1, ERR_INVALID_REQUEST);
        return;
    }

    std::string group_name = request["data"]["group_name"];

    int group_id = get_group_id_by_name(g_db, group_name);
    if (group_id == -1)
    {
        send_response(client_sock, 404, "Group not found");
        log_activity(g_db, user_id, ACT_GET_GROUP_MESSAGES, -1, ERR_GROUP_NOT_FOUND);
        return;
    }

    if (!is_group_member(g_db, group_id, user_id))
    {
        send_response(client_sock, 403, "You are not a member of this group");
        log_activity(g_db, user_id, ACT_GET_GROUP_MESSAGES, group_id, ERR_NOT_GROUP_MEMBER);
        return;
    }

    auto messages = get_group_messages(g_db, user_id, group_id);

    json response;
    response["type"] = 2405;
    response["data"]["group_name"] = group_name;
    response["data"]["messages"] = json::array();

    for (const auto &msg : messages)
    {
        response["data"]["messages"].push_back({
            {"sender_username", msg.sender_username},
            {"content", msg.content},
            {"timestamp", msg.timestamp}
        });
    }

    std::string response_str = response.dump();
    send_json_packet(client_sock, response_str.c_str());
    log_activity(g_db, user_id, ACT_GET_GROUP_MESSAGES, group_id, RESP_SUCCESS);
}

void handle_get_offline_messages(int client_sock, const json &request)
{
    // TODO: Implement get offline messages
    std::cout << "[PLACEHOLDER] GET_OFFLINE_MESSAGES called" << std::endl;
    send_response(client_sock, 200, "Get offline messages feature not implemented yet");
}
