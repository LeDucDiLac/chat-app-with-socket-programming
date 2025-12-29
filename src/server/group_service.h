#ifndef GROUP_SERVICE_H
#define GROUP_SERVICE_H

#include <string>
#include <vector>
#include <sqlite3.h>
#include "write_log.h"

struct Group
{
  int id;
  std::string username;  // group_name
  std::string role;    // "owner" or "member"
};

struct GroupMessage
{
  int message_id;
  int sender_id;
  std::string sender_username;
  std::string content;
  std::string timestamp;
};

int get_group_id_by_name(sqlite3* db, const std::string& group_name);
bool is_group_owner(sqlite3* db, int group_id, int user_id);
bool is_group_member(sqlite3* db, int group_id, int user_id);

// Group operations
bool create_group(sqlite3* db, const std::string& group_name, int creator_id);
bool add_group_member(sqlite3* db, int group_id, int user_id);
bool remove_group_member(sqlite3* db, int group_id, int user_id);
bool delete_group(sqlite3* db, int group_id);
std::vector<Group> get_all_group(sqlite3* db, int user_id);

bool send_group_message(sqlite3* db, const int sender_id, const int group_id, const std::string& content);
std::vector<GroupMessage> get_group_messages(sqlite3* db, int user_id, int group_id);

#endif // GROUP_SERVICE_H
