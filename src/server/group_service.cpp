#include "group_service.h"
#include "db_manager.h" // Giả sử db_manager có các hàm cần thiết (ví dụ: g_db)
#include <iostream>

// Lấy ID nhóm từ tên nhóm
int getGroupIdByName(const std::string& group_name)
{
    sqlite3_stmt *stmt;
    int group_id = -1;

    const char *sql = "SELECT group_id FROM groups WHERE group_name = ?;";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (getGroupIdByName): "
                  << sqlite3_errmsg(g_db) << std::endl;
        return -1;
    }

    sqlite3_bind_text(stmt, 1, group_name.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        group_id = sqlite3_column_int(stmt, 0);
    }
    else if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        std::cerr << "❌ Execution failed (getGroupIdByName): "
                  << sqlite3_errmsg(g_db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return group_id;
}

// Kiểm tra xem người dùng có phải là chủ sở hữu nhóm không
bool isGroupOwner(int group_id, int user_id)
{
    sqlite3_stmt *stmt;
    bool is_owner = false;

    const char *sql = "SELECT 1 FROM groups WHERE group_id = ? AND created_by = ?;";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (isGroupOwner): "
                  << sqlite3_errmsg(g_db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, user_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        is_owner = true;
    }

    sqlite3_finalize(stmt);
    return is_owner;
}

bool isGroupMember(int group_id, int user_id)
{
    sqlite3_stmt *stmt;
    bool is_member = false;

    const char *sql = "SELECT 1 FROM group_members WHERE group_id = ? AND user_id = ?;";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (isGroupMember): "
                  << sqlite3_errmsg(g_db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, user_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        is_member = true;
    }

    sqlite3_finalize(stmt);
    return is_member;
}

// Hàm tạo nhóm mới và trả về group_id
bool createGroup(const std::string& group_name, int creator_id)
{
    sqlite3_stmt *stmt;

    // 1. Chèn thông tin nhóm vào bảng 'groups'
    const char *sql_group = R"(
        INSERT INTO groups (group_name, created_by) VALUES (?, ?);
    )";

    if (sqlite3_prepare_v2(g_db, sql_group, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (createGroup - group): "
                  << sqlite3_errmsg(g_db) << std::endl;
        return -1;
    }

    sqlite3_bind_text(stmt, 1, group_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, creator_id);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        std::cerr << "❌ Execution failed (createGroup - group): "
                  << sqlite3_errmsg(g_db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);

    std::cout << "✅ Group created: " << group_name << std::endl;

    // 2. Thêm người tạo vào nhóm với quyền admin
    int group_id = sqlite3_last_insert_rowid(g_db);
    if (!addGroupMember(group_id, creator_id))
    {
        std::cerr << "❌ Failed to add creator to group: " << creator_id << " to " << group_id << std::endl;
        return false;
    }
    
    return true;
}

// Hàm thêm thành viên vào nhóm
bool addGroupMember(int group_id, int user_id)
{
    sqlite3_stmt *stmt;

    const char *sql = R"(
        INSERT OR IGNORE INTO group_members (group_id, user_id) 
        VALUES (?, ?);
    )";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (addGroupMember): "
                  << sqlite3_errmsg(g_db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, user_id);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        std::cerr << "❌ Execution failed (addGroupMember): "
                  << sqlite3_errmsg(g_db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    std::cout << "✅ User " << user_id << " added to group " << group_id << std::endl;
    return true;
}

bool removeGroupMember(int group_id, int user_id)
{
    sqlite3_stmt *stmt;

    const char *sql = "DELETE FROM group_members WHERE group_id = ? AND user_id = ?;";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (removeGroupMember): "
                  << sqlite3_errmsg(g_db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, user_id);

    bool success = false;

    if (sqlite3_step(stmt) == SQLITE_DONE)
    {
        if (sqlite3_changes(g_db) > 0)
        {
            std::cout << "✅ User " << user_id << " removed from group " << group_id << std::endl;
            success = true;
        }
        else
        {
            // Có thể user_id không tồn tại trong group_members
            std::cout << "⚠️ No member found to remove: " << user_id << " from group " << group_id << std::endl;
        }
    }
    else
    {
        std::cerr << "❌ Delete failed (removeGroupMember): "
                  << sqlite3_errmsg(g_db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return success;
}

bool deleteGroup(int group_id)
{
    sqlite3_stmt *stmt_group;

    // 2. Xóa nhóm khỏi bảng groups
    const char *sql_group = "DELETE FROM groups WHERE group_id = ?;";
    
    if (sqlite3_prepare_v2(g_db, sql_group, -1, &stmt_group, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (deleteGroup - group): " << sqlite3_errmsg(g_db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt_group, 1, group_id);
    if (sqlite3_step(stmt_group) == SQLITE_DONE)
    {
        if (sqlite3_changes(g_db) > 0)
        {
            std::cout << "✅ Group deleted: " << group_id << std::endl;
        }
    }
    else
    {
        std::cerr << "❌ Execution failed (deleteGroup - group): " << sqlite3_errmsg(g_db) << std::endl;
    }

    sqlite3_finalize(stmt_group);
    return true;
}
