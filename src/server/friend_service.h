#ifndef FRIEND_SERVICE_H
#define FRIEND_SERVICE_H

#include <string>
#include <vector>
#include <sqlite3.h>

extern sqlite3 *g_db;

struct FriendInfo
{
  int id;
  std::string username;
  std::string status; // "online" | "offline"
};

struct FriendRequestInfo
{
  int sender_id;
  std::string sender_username;
  std::string timestamp;
};

// Friendship operations
std::vector<FriendRequestInfo> getAllFriendRequests(const int receiver_id);
std::vector<FriendInfo> getAllFriend(const int user_id);
bool friendRequestExists(const int sender, const int receiver);
bool addFriendRequest(const int sender, const int receiver);
bool removeFriendRequest(const int sender, const int receiver);
bool friendshipExists(const int user1, const int user2);
bool addFriendship(const int user1, const int user2);
bool removeFriendship(const int user1, const int user2);

#endif // FRIEND_SERVICE_H
