#ifndef ACTION_TYPES_H
#define ACTION_TYPES_H

#include <string>
#include <unordered_map>

/**
 * Action type conversion utility for logging server activities
 * Maps action type strings to human-readable descriptions
 */

// Action type strings
static const std::unordered_map<std::string, std::string> ACTION_TYPE_DESCRIPTIONS = {
    // Authentication actions
    {"register", "User Registration"},
    {"login", "User Login"},
    {"logout", "User Logout"},
    
    // Message actions
    {"send_message", "Send Direct Message"},
    {"send_group_message", "Send Group Message"},
    {"get_offline_messages", "Retrieve Offline Messages"},
    {"get_group_messages", "Retrieve Group Messages"},
    
    // Friend request actions
    {"get_friend_requests", "Get Friend Requests"},
    {"send_friend_request", "Send Friend Request"},
    {"accept_friend_request", "Accept Friend Request"},
    {"reject_friend_request", "Reject Friend Request"},
    {"unfriend", "Unfriend User"},
    {"get_friend_list", "Retrieve Friend List"},
    
    // Group actions
    {"create_group", "Create Group"},
    {"add_to_group", "Add Member to Group"},
    {"remove_from_group", "Remove Member from Group"},
    {"leave_group", "Leave Group"},
    {"get_group_list", "Retrieve Group List"},
};

/**
 * Convert an action type string to a human-readable description
 * 
 * @param action_type The action type string (e.g., "login", "send_message")
 * @return Human-readable description, or the original string if not found
 */
inline std::string action_type_to_string(const std::string& action_type)
{
    auto it = ACTION_TYPE_DESCRIPTIONS.find(action_type);
    if (it != ACTION_TYPE_DESCRIPTIONS.end())
    {
        return it->second;
    }
    return action_type; // Return original if not found
}

#endif // ACTION_TYPES_H
