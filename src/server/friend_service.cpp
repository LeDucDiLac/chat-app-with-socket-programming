#include "friend_service.h"
#include "db_manager.h" // Assuming db_manager handles database queries
#include <iostream>

std::vector<FriendRequestInfo> getAllFriendRequests(const int receiver_id)
{
    std::vector<FriendRequestInfo> requests;
    sqlite3_stmt *stmt;

    const char *sql = R"(
        SELECT 
            u.id,
            u.username,
            fr.timestamp
        FROM friend_requests fr
        JOIN accounts u ON fr.sender_id = u.id
        WHERE fr.receiver_id = ?
        ORDER BY fr.timestamp DESC
    )";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (getAllFriendRequests): "
                  << sqlite3_errmsg(g_db) << std::endl;
        return requests;
    }

    sqlite3_bind_int(stmt, 1, receiver_id);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        FriendRequestInfo fr;

        fr.sender_id = sqlite3_column_int(stmt, 0);
        fr.sender_username = reinterpret_cast<const char*>(
            sqlite3_column_text(stmt, 1)
        );

        const unsigned char *ts = sqlite3_column_text(stmt, 2);
        fr.timestamp = ts ? reinterpret_cast<const char *>(ts) : "";

        requests.push_back(fr);
    }

    sqlite3_finalize(stmt);
    return requests;
}

bool friendRequestExists(const int sender, const int receiver)
{
  const char *sql =
      "SELECT 1 FROM friend_requests "
      "WHERE sender_id = ? AND receiver_id = ? AND status = 'pending' "
      "LIMIT 1;";

  sqlite3_stmt *stmt;
  bool exists = false;

  // Prepare statement
  if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
  {
    std::cerr << "Failed to prepare statement: "
              << sqlite3_errmsg(g_db) << std::endl;
    return false;
  }

  // Bind parameters
  sqlite3_bind_int(stmt, 1, sender);
  sqlite3_bind_int(stmt, 2, receiver);

  // Execute
  if (sqlite3_step(stmt) == SQLITE_ROW)
  {
    exists = true;
  }

  // Cleanup
  sqlite3_finalize(stmt);
  return exists;
}

bool addFriendRequest(const int sender, const int receiver)
{
  const char *sql =
      "INSERT INTO friend_requests (sender_id, receiver_id, status, timestamp) "
      "VALUES (?, ?, 'pending', CURRENT_TIMESTAMP);";

  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
  {
    std::cerr << "Prepare failed: " << sqlite3_errmsg(g_db) << std::endl;
    return false;
  }

  sqlite3_bind_int(stmt, 1, sender);
  sqlite3_bind_int(stmt, 2, receiver);

  int rc = sqlite3_step(stmt);

  if (rc != SQLITE_DONE)
  {
    std::cerr << "Execution failed: " << sqlite3_errmsg(g_db) << std::endl;
    sqlite3_finalize(stmt);
    return false;
  }

  sqlite3_finalize(stmt);
  std::cout << "✅ Friend request added: " << sender << " -> " << receiver << std::endl;
  return true;
}

bool removeFriendRequest(const int sender, const int receiver)
{
  sqlite3_stmt *stmt;

  const char *sql =
      "DELETE FROM friend_requests "
      "WHERE sender_id = ? AND receiver_id = ?";

  if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
  {
    std::cerr << "❌ Prepare failed: " << sqlite3_errmsg(g_db) << std::endl;
    return false;
  }

  sqlite3_bind_int(stmt, 1, sender);
  sqlite3_bind_int(stmt, 2, receiver);

  bool success = false;

  if (sqlite3_step(stmt) == SQLITE_DONE)
  {
    if (sqlite3_changes(g_db) > 0)
    {
      std::cout << "✅ Friend request removed: "
                << sender << " -> " << receiver << std::endl;
      success = true;
    }
    else
    {
      std::cout << "⚠️ No friend request found for: "
                << sender << " -> " << receiver << std::endl;
    }
  }
  else
  {
    std::cerr << "❌ Delete failed: " << sqlite3_errmsg(g_db) << std::endl;
  }

  sqlite3_finalize(stmt);
  return success;
}

bool friendshipExists(int user1, int user2)
{
  sqlite3_stmt *stmt;

  const char *sql = R"(
        SELECT COUNT(*)
        FROM friendships
        WHERE 
            (user_id1 = ? AND user_id2 = ?)
            OR
            (user_id1 = ? AND user_id2 = ?);
    )";

  if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
  {
    std::cerr << "Failed to prepare statement\n";
    sqlite3_close(g_db);
    return false;
  }

  sqlite3_bind_int(stmt, 1, user1);
  sqlite3_bind_int(stmt, 2, user2);
  sqlite3_bind_int(stmt, 3, user2);
  sqlite3_bind_int(stmt, 4, user1);

  bool exists = false;

  if (sqlite3_step(stmt) == SQLITE_ROW)
  {
    int count = sqlite3_column_int(stmt, 0);
    exists = count > 0;
  }

  sqlite3_finalize(stmt);
  return exists;
}

bool addFriendship(const int user1, const int user2)
{
  // luôn lưu theo thứ tự tăng dần tránh trùng lặp
  int u1 = std::min(user1, user2);
  int u2 = std::max(user1, user2);

  const char *sql = R"(
        INSERT OR IGNORE INTO friendships (user_id1, user_id2)
        VALUES (?, ?)
    )";

  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
  {
    std::cerr << "[ERROR] Prepare failed: " << sqlite3_errmsg(g_db) << std::endl;
    return false;
  }

  sqlite3_bind_int(stmt, 1, u1);
  sqlite3_bind_int(stmt, 2, u2);

  int rc = sqlite3_step(stmt);

  if (rc != SQLITE_DONE)
  {
    std::cerr << "[ERROR] Insert failed: " << sqlite3_errmsg(g_db) << std::endl;
    sqlite3_finalize(stmt);
    return false;
  }

  sqlite3_finalize(stmt);

  std::cout << "[SUCCESS] Friendship added between "
            << u1 << " and " << u2 << std::endl;

  return true;
}

bool removeFriendship(const int user1, const int user2)
{
  sqlite3_stmt *stmt;

  const char *sql =
      "DELETE FROM friendships "
      "WHERE (user_id1 = ? AND user_id2 = ?) "
      "   OR (user_id1 = ? AND user_id2 = ?)";

  if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
  {
    std::cerr << "❌ Prepare failed (removeFriendship): "
              << sqlite3_errmsg(g_db) << std::endl;
    return false;
  }

  // Bind param
  sqlite3_bind_int(stmt, 1, user1);
  sqlite3_bind_int(stmt, 2, user2);
  sqlite3_bind_int(stmt, 3, user2);
  sqlite3_bind_int(stmt, 4, user1);

  bool success = false;

  if (sqlite3_step(stmt) == SQLITE_DONE)
  {
    if (sqlite3_changes(g_db) > 0)
    {
      std::cout << "✅ Friendship removed between "
                << user1 << " and " << user2 << std::endl;
      success = true;
    }
    else
    {
      std::cout << "⚠️ No friendship found to remove between "
                << user1 << " and " << user2 << std::endl;
    }
  }
  else
  {
    std::cerr << "❌ Delete failed (removeFriendship): "
              << sqlite3_errmsg(g_db) << std::endl;
  }

  sqlite3_finalize(stmt);
  return success;
}

std::vector<FriendInfo> getAllFriend(const int user_id)
{
    std::vector<FriendInfo> friends;
    sqlite3_stmt *stmt;

    const char *sql = R"(
        SELECT u.id, u.username, u.user_state
        FROM accounts u
        JOIN friendships f
        ON (
            (f.user_id1 = ? AND f.user_id2 = u.id)
            OR
            (f.user_id2 = ? AND f.user_id1 = u.id)
        )
    )";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "❌ Prepare failed (getAllFriend): "
                  << sqlite3_errmsg(g_db) << std::endl;
        return friends;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, user_id);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        FriendInfo f;

        f.id = sqlite3_column_int(stmt, 0);
        f.username = reinterpret_cast<const char *>(
            sqlite3_column_text(stmt, 1));
        f.status = reinterpret_cast<const char *>(
            sqlite3_column_text(stmt, 2));

        friends.push_back(f);
    }

    sqlite3_finalize(stmt);
    return friends;
}
