#include "write_log.h"
#include <iostream>
#include <ctime>

/**
 * Log user activity to database
 * Returns: void (logs errors to stderr if operation fails)
 */
void log_activity(sqlite3* db, const int user_id, const std::string& action_type, const int target_id, const std::string& result)
{
    const char* sql = 
        "INSERT INTO activity_logs (user_id, action_type, target_id, result) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "[DB] Failed to prepare log statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, action_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, target_id);
    sqlite3_bind_text(stmt, 4, result.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE)
    {
        std::cerr << "[DB] Failed to log activity: " << sqlite3_errmsg(db) << std::endl;
        return;
    }
    
    return;
}
