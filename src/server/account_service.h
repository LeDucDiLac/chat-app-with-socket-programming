#ifndef ACCOUNT_SERVICE_H
#define ACCOUNT_SERVICE_H

#include <sqlite3.h>
#include <string>

// Database initialization
sqlite3* init_database(const char* db_path);
void close_database(sqlite3* db);

// Account operations
int get_user_id_by_username(sqlite3* db, const std::string& username);
int register_user(sqlite3* db, const std::string& username, const std::string& password, int* user_id);
int verify_login(sqlite3* db, const std::string& username, const std::string& password, int* user_id);
int update_user_state(sqlite3* db, int user_id, const std::string& state);

// Activity logging
int log_activity(sqlite3* db, int user_id, const std::string& action_type, const std::string& details);

// Status codes
#define DB_SUCCESS 0
#define DB_ERROR -1
#define DB_USER_EXISTS -2
#define DB_USER_NOT_FOUND -3
#define DB_INVALID_PASSWORD -4
#define DB_USER_BANNED -5

#endif // ACCOUNT_SERVICE_H
