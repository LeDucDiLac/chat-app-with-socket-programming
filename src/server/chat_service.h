#ifndef CHAT_SERVICE_H
#define CHAT_SERVICE_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include <cstdint>
#include "friend_service.h"

extern sqlite3 *g_db;

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
    int64_t timestamp;            // Unix timestamp
    bool is_read;
    bool is_offline;
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

/**
 * Send a direct message from one user to another
 * 
 * @param db Database connection handle
 * @param sender_id ID of the user sending the message
 * @param receiver_id ID of the user receiving the message
 * @param content Text content of the message
 * @param is_offline Whether the receiver is offline (message will be queued)
 * 
 * @return DB_SUCCESS on success
 *         DB_NOT_FRIENDS_TO_SEND if sender and receiver are not friends
 *         DB_USERNAME_NOT_EXIST if sender or receiver doesn't exist
 *         DB_ERROR on database error
 */
int send_direct_message(sqlite3* db, int sender_id, int receiver_id, 
                       const std::string& content, bool is_offline);

/**
 * Retrieve message history between two users
 * 
 * @param db Database connection handle
 * @param user_id1 ID of first user in conversation
 * @param user_id2 ID of second user in conversation
 * @param messages Output vector to store retrieved messages (sorted by timestamp ascending)
 * @param limit Maximum number of messages to retrieve (default: 50)
 * @param offset Number of messages to skip for pagination (default: 0)
 * 
 * @return DB_SUCCESS on success
 *         DB_ERROR on database error
 * 
 * @note Messages are returned in chronological order (oldest first)
 * @note Use limit and offset for pagination (e.g., limit=50, offset=0 for first page)
 */
int get_direct_message_history(sqlite3* db, int user_id1, int user_id2, 
                               std::vector<Message>& messages, 
                               int limit = 50, int offset = 0);

/**
 * Send a message to a group chat
 * 
 * @param db Database connection handle
 * @param sender_id ID of the user sending the message
 * @param group_id ID of the group to send the message to
 * @param content Text content of the message
 * 
 * @return DB_SUCCESS on success
 *         DB_NOT_GROUP_MEMBER if sender is not a member of the group
 *         DB_GROUP_NOT_FOUND if group doesn't exist
 *         DB_ERROR on database error
 */
int send_group_message(sqlite3* db, int sender_id, int group_id, 
                      const std::string& content);

/**
 * Retrieve message history for a group chat
 * 
 * @param db Database connection handle
 * @param group_id ID of the group
 * @param messages Output vector to store retrieved messages (sorted by timestamp ascending)
 * @param limit Maximum number of messages to retrieve (default: 50)
 * @param offset Number of messages to skip for pagination (default: 0)
 * 
 * @return DB_SUCCESS on success
 *         DB_GROUP_NOT_FOUND if group doesn't exist
 *         DB_ERROR on database error
 * 
 * @note Messages are returned in chronological order (oldest first)
 */
int get_group_message_history(sqlite3* db, int group_id, 
                              std::vector<Message>& messages,
                              int limit = 50, int offset = 0);

/**
 * Retrieve all offline messages for a user (messages received while offline)
 * 
 * @param db Database connection handle
 * @param user_id ID of the user
 * @param messages Output vector to store retrieved offline messages
 * 
 * @return DB_SUCCESS on success
 *         DB_ERROR on database error
 * 
 * @note This retrieves messages where is_offline=1 and receiver_id=user_id
 * @note Messages should be marked as read after retrieval using mark_messages_read
 */
int get_offline_messages(sqlite3* db, int user_id, 
                        std::vector<Message>& messages);

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
 * Mark all group messages as read for a specific user in a group
 * 
 * @param db Database connection handle
 * @param user_id ID of the user who read the messages
 * @param group_id ID of the group
 * 
 * @return DB_SUCCESS on success
 *         DB_NOT_GROUP_MEMBER if user is not in the group
 *         DB_ERROR on database error
 * 
 * @note This doesn't modify the messages table (group messages don't track individual read status)
 * @note Implementation may track read receipts in a separate table if needed
 */
int mark_group_messages_read(sqlite3* db, int user_id, int group_id);

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
 * Check if two users are friends
 * 
 * @param db Database connection handle
 * @param user_id1 ID of first user
 * @param user_id2 ID of second user
 * 
 * @return true if users are friends (friendship exists in friend_lists table)
 *         false otherwise
 * 
 * @note Friendship is bidirectional - checks both (id1, id2) and (id2, id1)
 */
bool are_friends(sqlite3* db, int user_id1, int user_id2);

/**
 * Check if a user is a member of a group
 * 
 * @param db Database connection handle
 * @param user_id ID of the user
 * @param group_id ID of the group
 * 
 * @return true if user is a member (exists in group_members table)
 *         false otherwise
 */
bool is_group_member(sqlite3* db, int user_id, int group_id);

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