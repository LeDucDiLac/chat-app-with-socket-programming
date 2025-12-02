#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <sqlite3.h>
#include <string>

// Database initialization
sqlite3* db_init(const char* db_path);
void db_close(sqlite3* db);

// Account operations
int getUserIdByUsername(const std::string& username);
int db_register_user(sqlite3* db, const std::string& username, const std::string& password, int* user_id);
int db_verify_login(sqlite3* db, const std::string& username, const std::string& password, int* user_id);
int db_update_user_state(sqlite3* db, int user_id, const std::string& state);

// Activity logging
int db_log_activity(sqlite3* db, int user_id, const std::string& action_type, const std::string& details);

// Status codes
#define DB_SUCCESS 0
#define DB_ERROR -1
#define DB_USER_EXISTS -2
#define DB_USER_NOT_FOUND -3
#define DB_INVALID_PASSWORD -4
#define DB_USER_BANNED -5

#endif // DB_MANAGER_H
