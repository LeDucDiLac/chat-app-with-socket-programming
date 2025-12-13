#ifndef GROUP_SERVICE_H
#define GROUP_SERVICE_H

#include <string>
#include <vector>
#include <sqlite3.h>

struct GroupInfo
{
  int group_id;
  std::string group_name;
  int owner_id;
  std::string created_at;
};

int get_group_id_by_name(sqlite3* db, const std::string& group_name);
bool is_group_owner(sqlite3* db, int group_id, int user_id);
bool is_group_member(sqlite3* db, int group_id, int user_id);

// Group operations
bool create_group(sqlite3* db, const std::string& group_name, int creator_id);
bool add_group_member(sqlite3* db, int group_id, int user_id);
bool remove_group_member(sqlite3* db, int group_id, int user_id);
bool delete_group(sqlite3* db, int group_id);

#endif // GROUP_SERVICE_H
