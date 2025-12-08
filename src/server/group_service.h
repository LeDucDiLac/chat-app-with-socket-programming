#ifndef GROUP_SERVICE_H
#define GROUP_SERVICE_H

#include <string>
#include <vector>
#include <sqlite3.h>

extern sqlite3 *g_db;

struct GroupInfo
{
  int group_id;
  std::string group_name;
  int owner_id;
  std::string created_at;
};

int getGroupIdByName(const std::string& group_name);
bool isGroupOwner(int group_id, int user_id);
bool isGroupMember(int group_id, int user_id);

// Group operations
bool createGroup(const std::string& group_name, int creator_id);
bool addGroupMember(int group_id, int user_id);
bool removeGroupMember(int group_id, int user_id);
bool deleteGroup(int group_id);

#endif // GROUP_SERVICE_H
