#ifndef WRITE_LOG_H
#define WRITE_LOG_H

#include <string>
#include <vector>
#include <sqlite3.h>

// ============================================================================
// ACTION TYPE CODES (Request Types)
// ============================================================================

// Authentication actions
#define ACT_REGISTER                "register"
#define ACT_LOGIN                   "login"
#define ACT_LOGOUT                  "logout"

// Message actions
#define ACT_SEND_MESSAGE            "send_message"
#define ACT_GET_DIRECT_MESSAGES    "get_direct_messages"
#define ACT_SEND_GROUP_MESSAGE      "send_group_message"
#define ACT_GET_GROUP_MESSAGES      "get_group_messages"

// Friend request actions
#define ACT_GET_FRIEND_REQUESTS     "get_friend_requests"
#define ACT_SEND_FRIEND_REQUEST     "send_friend_request"
#define ACT_ACCEPT_FRIEND_REQUEST   "accept_friend_request"
#define ACT_REJECT_FRIEND_REQUEST   "reject_friend_request"
#define ACT_UNFRIEND                "unfriend"
#define ACT_GET_FRIEND_LIST         "get_friend_list"

// Group actions
#define ACT_CREATE_GROUP            "create_group"
#define ACT_ADD_TO_GROUP            "add_to_group"
#define ACT_REMOVE_FROM_GROUP       "remove_from_group"
#define ACT_LEAVE_GROUP             "leave_group"
#define ACT_GET_GROUP_LIST          "get_group_list"

// ============================================================================
// HTTP-LIKE RESPONSE CODES
// ============================================================================

// Success responses
#define RESP_SUCCESS                "Success"
#define RESP_CREATED                "Created"

// ============================================================================
// ERROR MESSAGES
// ============================================================================

// Authentication errors
#define ERR_WRONG_PASSWORD          "Incorrect password"
#define ERR_USER_NOT_FOUND          "User not found"
#define ERR_USER_EXISTS             "Username already exists"
#define ERR_USER_BANNED             "Account is banned"
#define ERR_NOT_AUTHENTICATED       "Not authenticated"
#define ERR_NOT_LOGGED_IN          "Not logged in"
#define ERR_ALREADY_LOGGED_IN       "Already logged in"
#define ERR_USER_LOGGED_IN_ELSEWHERE "User already logged in from another connection"

// Permission errors
#define ERR_PERMISSION_DENIED       "Permission denied"

// Request errors
#define ERR_INVALID_REQUEST         "Invalid request format"
#define ERR_INVALID_JSON            "Invalid JSON format"
#define ERR_MISSING_PARAMETER       "Missing required parameter"

// Friend-related errors
#define ERR_ALREADY_FRIEND          "Already friends with this user"
#define ERR_NOT_FRIEND              "Not friends with this user"
#define ERR_FRIEND_REQUEST_EXISTS   "Friend request already exists"
#define ERR_SELF_REQUEST            "Cannot send friend request to yourself"
#define ERR_FRIEND_REQUEST_NOT_FOUND "Friend request not found"

// Group-related errors
#define ERR_GROUP_NOT_FOUND         "Group not found"
#define ERR_CANNOT_REMOVE_SELF      "Cannot remove yourself. Please use LEAVE_GROUP"
#define ERR_NOT_GROUP_MEMBER        "Not a member of this group"
#define ERR_NOT_GROUP_OWNER         "Only group owner can perform this action"

// Server errors
#define ERR_BAD_REQUEST            "Bad request"
#define ERR_DATABASE_ERROR          "Database error"

/**
 * Log user activity to database
 * @param db Database connection
 * @param user_id User ID performing the action
 * @param action_type Action type string
 * @param details Additional details about the action
 */
void log_activity(sqlite3* db, const int user_id, const std::string& action_type, const int target_id, const std::string& result);

#endif // WRITE_LOG_H
