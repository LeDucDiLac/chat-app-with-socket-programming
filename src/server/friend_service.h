#ifndef FRIEND_SERVICE_H
#define FRIEND_SERVICE_H

#include <string>
#include <vector>
#include <sqlite3.h>

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
std::vector<FriendRequestInfo> get_all_friend_requests(sqlite3* db, const int receiver_id);
std::vector<FriendInfo> get_all_friends(sqlite3* db, const int user_id);
bool friend_request_exists(sqlite3* db, const int sender, const int receiver);
bool add_friend_request(sqlite3* db, const int sender, const int receiver);
bool remove_friend_request(sqlite3* db, const int sender, const int receiver);
bool friendship_exists(sqlite3* db, const int user1, const int user2);
bool add_friendship(sqlite3* db, const int user1, const int user2);
bool remove_friendship(sqlite3* db, const int user1, const int user2);

#endif // FRIEND_SERVICE_H
