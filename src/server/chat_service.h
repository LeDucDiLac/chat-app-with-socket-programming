#ifndef CHAT_SERVICE_H
#define CHAT_SERVICE_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include <cstdint>
#include "friend_service.h"


// Message structure matching database schema
struct Message {
    int message_id;
    int sender_id;
    std::string sender_username;
    
    // For direct messages
    int receiver_id;              // 0 if group message
    std::string receiver_username; // empty if group message
    
    // For group messages
    int group_id;                 // 0 if direct message
    std::string group_name;       // empty if direct message
    
    std::string content;
    std::string timestamp;            // Unix timestamp
    bool is_read;
};

// Conversation info (for listing active chats)
struct Conversation {
    int user_id;                  // 0 if group conversation
    std::string username;         // empty if group
    int group_id;                 // 0 if direct conversation
    std::string group_name;       // empty if direct
    std::string last_message;
    int64_t last_timestamp;
    int unread_count;
};

// Error codes
#define DB_SUCCESS 0
#define DB_ERROR -1
#define DB_USERNAME_NOT_EXIST -2
#define DB_NOT_FRIENDS_TO_SEND -3
#define DB_NOT_GROUP_MEMBER -4
#define DB_GROUP_NOT_FOUND -5
#define DB_MESSAGE_NOT_FOUND -6
#define DB_PERMISSION_DENIED -7
#define DB_RECEIVER_BLOCKED -8


/**
 * Send a direct message from one user to another
 * 
 * @param db Database connection handle
 * @param sender_id ID of the user sending the message
 * @param receiver_id ID of the user receiving the message
 * @param content Text content of the message
 * 
 * @return DB_SUCCESS on success
 *         DB_ERROR on database error
 */
int send_direct_message(sqlite3* db, int sender_id, int receiver_id, 
                       const std::string& content);

/**
 * Retrieve message history between two users
 * 
 * @param db Database connection handle
 * @param user_id1 ID of first user in conversation
 * @param user_id2 ID of second user in conversation
 * @param messages Output vector to store retrieved messages (sorted by timestamp ascending)
 * @param limit Maximum number of messages to retrieve (default: 10)
 * @param offset Number of messages to skip for pagination (default: 0)
 * 
 * @return DB_SUCCESS on success
 *         DB_ERROR on database error
 * 
 * @note Messages are returned in chronological order (oldest first)
 * @note Use limit and offset for pagination (e.g., limit=10, offset=0 for first page)
 */
int get_direct_message_history(sqlite3* db, int user_id1, int user_id2, 
                               std::vector<Message>& messages, 
                               int limit = 10, int offset = 0);




/**
 * Mark all messages from a specific sender to a user as read
 * 
 * @param db Database connection handle
 * @param user_id ID of the user who read the messages (receiver)
 * @param sender_id ID of the user who sent the messages
 * 
 * @return DB_SUCCESS on success
 *         DB_ERROR on database error
 * 
 * @note Updates is_read=1 and is_offline=0 for matching messages
 */
int mark_messages_read(sqlite3* db, int user_id, int sender_id);



/**
 * Get list of active conversations (both direct and group) for a user
 * 
 * @param db Database connection handle
 * @param user_id ID of the user
 * @param conversations Output vector to store conversation summaries
 * 
 * @return DB_SUCCESS on success
 *         DB_ERROR on database error
 * 
 * @note Returns conversations sorted by last_timestamp descending (most recent first)
 * @note Includes both direct message conversations (friends) and group chats
 * @note Each conversation includes last message preview and unread count
 */
int get_active_conversations(sqlite3* db, int user_id, 
                             std::vector<Conversation>& conversations);


/**
 * Get total count of unread messages for a user
 * 
 * @param db Database connection handle
 * @param user_id ID of the user
 * @param total_unread Output parameter to store total unread count
 * 
 * @return DB_SUCCESS on success
 *         DB_ERROR on database error
 * 
 * @note Counts all messages where receiver_id=user_id and is_read=0
 * @note Includes both direct and offline messages
 */
int get_unread_count(sqlite3* db, int user_id, int* total_unread);

#endif // CHAT_SERVICE_H