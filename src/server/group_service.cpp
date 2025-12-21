#include "group_service.h"
#include "account_service.h"
#include <iostream>

// Lấy ID nhóm từ tên nhóm
int get_group_id_by_name(sqlite3 *db, const std::string &group_name)
{
    sqlite3_stmt *stmt;
    int group_id = -1;

    const char *sql = "SELECT group_id FROM groups WHERE group_name = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (get_group_id_by_name): "
                  << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    sqlite3_bind_text(stmt, 1, group_name.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        group_id = sqlite3_column_int(stmt, 0);
    }
    else if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        std::cerr << "❌ Execution failed (get_group_id_by_name): "
                  << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return group_id;
}

// Kiểm tra xem người dùng có phải là chủ sở hữu nhóm không
bool is_group_owner(sqlite3 *db, int group_id, int user_id)
{
    sqlite3_stmt *stmt;
    bool is_owner = false;

    const char *sql = "SELECT 1 FROM groups WHERE group_id = ? AND created_by = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (is_group_owner): "
                  << sqlite3_errmsg(db) << std::endl;
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

bool is_group_member(sqlite3 *db, int group_id, int user_id)
{
    sqlite3_stmt *stmt;
    bool is_member = false;

    const char *sql = "SELECT 1 FROM group_members WHERE group_id = ? AND user_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (is_group_member): "
                  << sqlite3_errmsg(db) << std::endl;
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
bool create_group(sqlite3 *db, const std::string &group_name, int creator_id)
{
    sqlite3_stmt *stmt;

    // 1. Chèn thông tin nhóm vào bảng 'groups'
    const char *sql_group = R"(
        INSERT INTO groups (group_name, created_by) VALUES (?, ?);
    )";

    if (sqlite3_prepare_v2(db, sql_group, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (create_group - group): "
                  << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    sqlite3_bind_text(stmt, 1, group_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, creator_id);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        std::cerr << "❌ Execution failed (create_group - group): "
                  << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);

    std::cout << "✅ Group created: " << group_name << std::endl;

    // 2. Thêm người tạo vào nhóm với quyền admin
    int group_id = sqlite3_last_insert_rowid(db);
    if (!add_group_member(db, group_id, creator_id))
    {
        std::cerr << "❌ Failed to add creator to group: " << creator_id << " to " << group_id << std::endl;
        return false;
    }

    return true;
}

// Hàm thêm thành viên vào nhóm
bool add_group_member(sqlite3 *db, int group_id, int user_id)
{
    sqlite3_stmt *stmt;

    const char *sql = R"(
        INSERT OR IGNORE INTO group_members (group_id, user_id) 
        VALUES (?, ?);
    )";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (add_group_member): "
                  << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, user_id);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        std::cerr << "❌ Execution failed (add_group_member): "
                  << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    std::cout << "✅ User " << user_id << " added to group " << group_id << std::endl;
    return true;
}

bool remove_group_member(sqlite3 *db, int group_id, int user_id)
{
    sqlite3_stmt *stmt;

    const char *sql = "DELETE FROM group_members WHERE group_id = ? AND user_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (remove_group_member): "
                  << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, user_id);

    bool success = false;

    if (sqlite3_step(stmt) == SQLITE_DONE)
    {
        if (sqlite3_changes(db) > 0)
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
        std::cerr << "❌ Delete failed (remove_group_member): "
                  << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return success;
}

bool delete_group(sqlite3 *db, int group_id)
{
    sqlite3_stmt *stmt_group;

    // 2. Xóa nhóm khỏi bảng groups
    const char *sql_group = "DELETE FROM groups WHERE group_id = ?;";

    if (sqlite3_prepare_v2(db, sql_group, -1, &stmt_group, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (delete_group - group): " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt_group, 1, group_id);
    if (sqlite3_step(stmt_group) == SQLITE_DONE)
    {
        if (sqlite3_changes(db) > 0)
        {
            std::cout << "✅ Group deleted: " << group_id << std::endl;
        }
    }
    else
    {
        std::cerr << "❌ Execution failed (delete_group - group): " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt_group);
    return true;
}

// Lấy tất cả các nhóm mà người dùng tham gia cùng với vai trò
std::vector<Group> get_all_group(sqlite3 *db, int user_id)
{
    std::vector<Group> groups;
    sqlite3_stmt *stmt;

    const char *sql = R"(
        SELECT g.group_id, g.group_name, g.created_by
        FROM groups g
        INNER JOIN group_members gm ON g.group_id = gm.group_id
        WHERE gm.user_id = ?;
    )";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (get_all_group): "
                  << sqlite3_errmsg(db) << std::endl;
        return groups;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int group_id = sqlite3_column_int(stmt, 0);
        std::string group_name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        int owner_id = sqlite3_column_int(stmt, 2);
        std::string role = (owner_id == user_id) ? "owner" : "member";

        groups.push_back({group_id, group_name, role});
    }

    sqlite3_finalize(stmt);
    return groups;
}

bool send_group_message(sqlite3 *db, const int sender_id, const int group_id, const std::string &content)
{
    // 5. Insert message into group_messages table
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO group_messages (sender_id, group_id, content) VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (send_group_message): " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, sender_id);
    sqlite3_bind_int(stmt, 2, group_id);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        std::cerr << "❌ Execution failed (send_group_message): " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    // 6. Send success response
    std::cout << "[SEND_GROUP_MESSAGE] User " << sender_id << " sent message to group " << group_id << std::endl;
    sqlite3_finalize(stmt);
    return true;
}

// Get all messages for a group
std::vector<GroupMessage> get_group_messages(sqlite3 *db, int group_id)
{
    std::vector<GroupMessage> messages;
    sqlite3_stmt *stmt;

    const char *sql = R"(
        SELECT gm.message_id, gm.sender_id, a.username, gm.content, gm.timestamp
        FROM group_messages gm
        INNER JOIN accounts a ON gm.sender_id = a.id
        WHERE gm.group_id = ?
        ORDER BY gm.timestamp ASC;
    )";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (get_group_messages): "
                  << sqlite3_errmsg(db) << std::endl;
        return messages;
    }

    sqlite3_bind_int(stmt, 1, group_id);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int message_id = sqlite3_column_int(stmt, 0);
        int sender_id = sqlite3_column_int(stmt, 1);
        std::string sender_username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        std::string content = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        std::string timestamp = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));

        messages.push_back({message_id, sender_id, sender_username, content, timestamp});
    }

    sqlite3_finalize(stmt);
    std::cout << "[GET_GROUP_MESSAGES] Retrieved " << messages.size()
              << " messages from group " << group_id << std::endl;

    return messages;
}
