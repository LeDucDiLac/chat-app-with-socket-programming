#include "db_manager.h"
#include <cstring>
#include <ctime>
#include <iostream>

/**
 * Initialize database connection
 */
sqlite3* db_init(const char* db_path)
{
    sqlite3* db;
    
    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK)
    {
        std::cerr << "[DB] Failed to open database: " << sqlite3_errmsg(db) << std::endl;
        return nullptr;
    }
    
    // Enable foreign keys
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    
    std::cout << "[DB] Database initialized: " << db_path << std::endl;
    return db;
}

/**
 * Close database connection
 */
void db_close(sqlite3* db)
{
    if (db)
    {
        sqlite3_close(db);
        std::cout << "[DB] Database closed" << std::endl;
    }
}

/**
 * Register a new user
 * Returns: DB_SUCCESS, DB_USER_EXISTS, or DB_ERROR
 */
int db_register_user(sqlite3* db, const std::string& username, const std::string& password, int* user_id)
{
    // Check if username already exists
    const char* check_sql = "SELECT id FROM accounts WHERE username = ?;";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "[DB] Failed to prepare check statement: " << sqlite3_errmsg(db) << std::endl;
        return DB_ERROR;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc == SQLITE_ROW)
    {
        // User already exists
        return DB_USER_EXISTS;
    }
    
    // Insert new user
    const char* insert_sql = 
        "INSERT INTO accounts (username, password, account_status, user_state, created_at) "
        "VALUES (?, ?, 'active', 'offline', ?);";
    
    if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "[DB] Failed to prepare insert statement: " << sqlite3_errmsg(db) << std::endl;
        return DB_ERROR;
    }
    
    time_t now = time(nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, now);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE)
    {
        std::cerr << "[DB] Failed to insert user: " << sqlite3_errmsg(db) << std::endl;
        return DB_ERROR;
    }
    
    // Get the new user's ID
    *user_id = sqlite3_last_insert_rowid(db);
    
    std::cout << "[DB] User registered: " << username << " (ID: " << *user_id << ")" << std::endl;
    return DB_SUCCESS;
}

/**
 * Verify login credentials
 * Returns: DB_SUCCESS, DB_USER_NOT_FOUND, DB_INVALID_PASSWORD, DB_USER_BANNED, or DB_ERROR
 */
int db_verify_login(sqlite3* db, const std::string& username, const std::string& password, int* user_id)
{
    const char* sql = "SELECT id, password, account_status FROM accounts WHERE username = ?;";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "[DB] Failed to prepare login statement: " << sqlite3_errmsg(db) << std::endl;
        return DB_ERROR;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    
    int rc = sqlite3_step(stmt);
    
    if (rc != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return DB_USER_NOT_FOUND;
    }
    
    // Get user data
    int id = sqlite3_column_int(stmt, 0);
    const char* stored_password = (const char*)sqlite3_column_text(stmt, 1);
    const char* status = (const char*)sqlite3_column_text(stmt, 2);
    
    // Check if account is banned
    if (strcmp(status, "banned") == 0)
    {
        sqlite3_finalize(stmt);
        return DB_USER_BANNED;
    }
    
    // Verify password
    if (strcmp(stored_password, password.c_str()) != 0)
    {
        sqlite3_finalize(stmt);
        return DB_INVALID_PASSWORD;
    }
    
    *user_id = id;
    sqlite3_finalize(stmt);
    
    std::cout << "[DB] Login verified: " << username << " (ID: " << *user_id << ")" << std::endl;
    return DB_SUCCESS;
}

/**
 * Update user state (online/offline/away)
 * Returns: DB_SUCCESS or DB_ERROR
 */
int db_update_user_state(sqlite3* db, int user_id, const std::string& state)
{
    const char* sql = "UPDATE accounts SET user_state = ? WHERE id = ?;";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "[DB] Failed to prepare update statement: " << sqlite3_errmsg(db) << std::endl;
        return DB_ERROR;
    }
    
    sqlite3_bind_text(stmt, 1, state.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, user_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE)
    {
        std::cerr << "[DB] Failed to update user state: " << sqlite3_errmsg(db) << std::endl;
        return DB_ERROR;
    }
    
    std::cout << "[DB] User " << user_id << " state updated to: " << state << std::endl;
    return DB_SUCCESS;
}

/**
 * Log user activity
 * Returns: DB_SUCCESS or DB_ERROR
 */
int db_log_activity(sqlite3* db, int user_id, const std::string& action_type, const std::string& details)
{
    const char* sql = 
        "INSERT INTO activity_logs (user_id, action_type, details, timestamp) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "[DB] Failed to prepare log statement: " << sqlite3_errmsg(db) << std::endl;
        return DB_ERROR;
    }
    
    time_t now = time(nullptr);
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, action_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, details.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, now);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE)
    {
        std::cerr << "[DB] Failed to log activity: " << sqlite3_errmsg(db) << std::endl;
        return DB_ERROR;
    }
    
    return DB_SUCCESS;
}
