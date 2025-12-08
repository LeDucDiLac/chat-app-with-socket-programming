#include <sqlite3.h>
#include <iostream>
#include <ctime>

#define DB_PATH "database/chat.db"

void execute_sql(sqlite3 *db, const char *sql, const char *description)
{
    char *err_msg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Error " << description << ": " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }
    else
    {
        std::cout << "✓ " << description << std::endl;
    }
}

void create_tables(sqlite3 *db)
{
    std::cout << "\n=== Creating Database Tables ===" << std::endl;

    // accounts table
    execute_sql(db,
                "CREATE TABLE IF NOT EXISTS accounts ("
                "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "    username TEXT UNIQUE NOT NULL,"
                "    password TEXT NOT NULL,"
                "    account_status TEXT DEFAULT 'active' CHECK(account_status IN ('active', 'banned')),"
                "    user_state TEXT DEFAULT 'offline' CHECK(user_state IN ('online', 'offline', 'away')),"
                "    created_at INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP"
                ");",
                "accounts table created");

    // friend_requests table
    execute_sql(db,
                "CREATE TABLE IF NOT EXISTS friend_requests ("
                "    request_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "    sender_id INTEGER NOT NULL,"
                "    receiver_id INTEGER NOT NULL,"
                "    timestamp INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                "    FOREIGN KEY(sender_id) REFERENCES accounts(id) ON DELETE CASCADE,"
                "    FOREIGN KEY(receiver_id) REFERENCES accounts(id) ON DELETE CASCADE,"
                "    UNIQUE(sender_id, receiver_id),"
                "    CHECK(sender_id != receiver_id)"
                ");",
                "friend_requests table created");

    // friendships table
    execute_sql(db,
                "CREATE TABLE IF NOT EXISTS friendships ("
                "    friendship_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "    user_id1 INTEGER NOT NULL,"
                "    user_id2 INTEGER NOT NULL,"
                "    created_at INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                "    FOREIGN KEY(user_id1) REFERENCES accounts(id) ON DELETE CASCADE,"
                "    FOREIGN KEY(user_id2) REFERENCES accounts(id) ON DELETE CASCADE,"
                "    UNIQUE(user_id1, user_id2),"
                "    CHECK(user_id1 < user_id2)"
                ");",
                "friendships table created");

    // groups table
    execute_sql(db,
                "CREATE TABLE IF NOT EXISTS groups ("
                "    group_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "    group_name TEXT UNIQUE NOT NULL,"
                "    created_by INTEGER NOT NULL,"
                "    created_at INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                "    FOREIGN KEY(created_by) REFERENCES accounts(id) ON DELETE CASCADE"
                ");",
                "groups table created");

    // group_members table
    execute_sql(db,
                "CREATE TABLE IF NOT EXISTS group_members ("
                "    group_id INTEGER NOT NULL,"
                "    user_id INTEGER NOT NULL,"
                "    joined_at INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                "    PRIMARY KEY(group_id, user_id),"
                "    FOREIGN KEY(group_id) REFERENCES groups(group_id) ON DELETE CASCADE,"
                "    FOREIGN KEY(user_id) REFERENCES accounts(id) ON DELETE CASCADE,"
                "    UNIQUE(group_id, user_id)"
                ");",
                "group_members table created");

    // messages table
    execute_sql(db,
                "CREATE TABLE IF NOT EXISTS messages ("
                "    message_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "    sender_id INTEGER NOT NULL,"
                "    receiver_id INTEGER,"
                "    group_id INTEGER,"
                "    content TEXT NOT NULL,"
                "    timestamp INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                "    is_read INTEGER DEFAULT 0 CHECK(is_read IN (0, 1)),"
                "    is_offline INTEGER DEFAULT 0 CHECK(is_offline IN (0, 1)),"
                "    FOREIGN KEY(sender_id) REFERENCES accounts(id) ON DELETE CASCADE,"
                "    FOREIGN KEY(receiver_id) REFERENCES accounts(id) ON DELETE CASCADE,"
                "    FOREIGN KEY(group_id) REFERENCES groups(group_id) ON DELETE CASCADE,"
                "    CHECK((receiver_id IS NULL) != (group_id IS NULL))"
                ");",
                "messages table created");


    // activity_logs table
    execute_sql(db,
                "CREATE TABLE IF NOT EXISTS activity_logs ("
                "    log_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "    user_id INTEGER,"
                "    action_type TEXT NOT NULL,"
                "    details TEXT,"
                "    timestamp INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                "    FOREIGN KEY(user_id) REFERENCES accounts(id) ON DELETE SET NULL"
                ");",
                "activity_logs table created");
}

void seed_data(sqlite3 *db)
{
    std::cout << "\n=== Seeding Database ===" << std::endl;

    time_t now = time(nullptr);
    time_t day_ago = now - 86400;
    time_t week_ago = now - 604800;

    // Insert accounts
    std::string insert_accounts =
        "INSERT OR IGNORE INTO accounts (username, password, account_status, user_state, created_at) VALUES "
        "('alice', 'password123', 'active', 'offline', " +
        std::to_string(week_ago) + "),"
                                   "('bob', 'bobpass456', 'active', 'online', " +
        std::to_string(week_ago) + "),"
                                   "('charlie', 'charlie789', 'active', 'online', " +
        std::to_string(week_ago) + "),"
                                   "('diana', 'diana2024', 'active', 'offline', " +
        std::to_string(day_ago) + "),"
                                  "('eve', 'eve12345', 'banned', 'offline', " +
        std::to_string(week_ago) + ");";
    execute_sql(db, insert_accounts.c_str(), "accounts seeded");

    // Insert friendships
    std::string insert_friendships =
        "INSERT OR IGNORE INTO friendships (user_id1, user_id2, created_at) VALUES "
        "(1, 2, " +
        std::to_string(week_ago) + ")," // alice <-> bob
                                   "(1, 3, " +
        std::to_string(week_ago) + ")," // alice <-> charlie
                                   "(2, 3, " +
        std::to_string(week_ago) + ");"; // bob <-> charlie
    execute_sql(db, insert_friendships.c_str(), "friendships seeded");

    // Insert friend requests
    std::string insert_friend_requests =
        "INSERT OR IGNORE INTO friend_requests (sender_id, receiver_id, timestamp) VALUES "
        "(1, 4, " +
        std::to_string(day_ago) + ")," // alice -> diana (pending)
                                  "(2, 5," +
        std::to_string(week_ago) + ")," // bob -> eve (rejected)
                                   "(4, 1, " +
        std::to_string(week_ago) + ");"; // diana -> alice (accepted)
    execute_sql(db, insert_friend_requests.c_str(), "friend_requests seeded");

    // Insert groups
    std::string insert_groups =
        "INSERT OR IGNORE INTO groups (group_name, created_by, created_at) VALUES "
        "('Study Group', 1, " +
        std::to_string(week_ago) + "),"
                                   "('Gaming Squad', 2, " +
        std::to_string(week_ago) + "),"
                                   "('Work Team', 3, " +
        std::to_string(day_ago) + ");";
    execute_sql(db, insert_groups.c_str(), "groups seeded");

    // Insert group members
    std::string insert_group_members =
        "INSERT OR IGNORE INTO group_members (group_id, user_id, joined_at) VALUES "
        "(1, 1, " +
        std::to_string(week_ago) + ")," // Study Group: alice (admin)
                                   "(1, 2, " +
        std::to_string(week_ago) + ")," // Study Group: bob
                                   "(1, 3, " +
        std::to_string(week_ago) + ")," // Study Group: charlie
                                   "(2, 2, " +
        std::to_string(week_ago) + ")," // Gaming Squad: bob (admin)
                                   "(2, 3, " +
        std::to_string(week_ago) + ")," // Gaming Squad: charlie
                                   "(3, 3, " +
        std::to_string(day_ago) + ")," // Work Team: charlie (admin)
                                  "(3, 1, " +
        std::to_string(day_ago) + ")," // Work Team: alice
                                  "(3, 4, " +
        std::to_string(day_ago) + ");"; // Work Team: diana
    execute_sql(db, insert_group_members.c_str(), "group_members seeded");

    // Insert messages
    std::string insert_messages =
        "INSERT OR IGNORE INTO messages (sender_id, receiver_id, group_id, content, timestamp, is_read, is_offline) VALUES "
        "(1, 2, NULL, 'Hey Bob, how are you?', " +
        std::to_string(week_ago) + ", 1, 0),"
                                   "(2, 1, NULL, 'I''m good Alice, thanks!', " +
        std::to_string(week_ago) + ", 1, 0),"
                                   "(3, 1, NULL, 'Hi Alice!', " +
        std::to_string(day_ago) + ", 0, 1)," // offline message
                                  "(1, NULL, 1, 'Welcome to Study Group everyone', " +
        std::to_string(week_ago) + ", 1, 0),"
                                   "(2, NULL, 1, 'Thanks for adding me!', " +
        std::to_string(week_ago) + ", 1, 0),"
                                   "(4, 1, NULL, 'Can we talk later?', " +
        std::to_string(day_ago) + ", 0, 1)," // offline message
                                  "(2, NULL, 2, 'Ready to play?', " +
        std::to_string(week_ago) + ", 1, 0),"
                                   "(3, NULL, 2, 'Let''s go!', " +
        std::to_string(week_ago) + ", 1, 0);";
    execute_sql(db, insert_messages.c_str(), "messages seeded");

    // Insert activity logs
    std::string insert_activity_logs =
        "INSERT OR IGNORE INTO activity_logs (user_id, action_type, details, timestamp) VALUES "
        "(2, 'login', 'User logged in from 127.0.0.1', " +
        std::to_string(now - 3600) + "),"
                                     "(1, 'send_message', 'Sent message to user 2', " +
        std::to_string(now - 3500) + "),"
                                     "(3, 'join_group', 'Joined group 1', " +
        std::to_string(week_ago) + "),"
                                   "(2, 'send_friend_request', 'Sent friend request to user 5', " +
        std::to_string(week_ago) + "),"
                                   "(4, 'create_group', 'Created group 3', " +
        std::to_string(day_ago) + "),"
                                  "(1, 'accept_friend_request', 'Accepted friend request from user 4', " +
        std::to_string(week_ago) + "),"
                                   "(3, 'logout', 'User logged out', " +
        std::to_string(now - 7200) + ");";
    execute_sql(db, insert_activity_logs.c_str(), "activity_logs seeded");
}

void display_summary(sqlite3 *db)
{
    std::cout << "\n=== Database Summary ===" << std::endl;

    sqlite3_stmt *stmt;

    // Count tables
    const char *tables[] = {"accounts", "friend_requests", "friendships", "groups",
                            "group_members", "messages", "activity_logs"};

    for (const char *table : tables)
    {
        std::string query = "SELECT COUNT(*) FROM " + std::string(table);
        sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int count = sqlite3_column_int(stmt, 0);
            std::cout << table << ": " << count << " rows" << std::endl;
        }
        sqlite3_finalize(stmt);
    }
}

int main()
{
    sqlite3 *db;

    if (std::remove(DB_PATH) == 0)
    {
        std::cout << "Database annihilated\n";
    }
    else
    {
        std::perror("Error deleting file");
    }

    std::cout << "=== Database Initialization ===" << std::endl;
    std::cout << "Database: " << DB_PATH << std::endl;
    // Open/create database
    int rc = sqlite3_open(DB_PATH, &db);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    // Enable foreign keys
    execute_sql(db, "PRAGMA foreign_keys = ON;", "Foreign keys enabled");

    // Create tables
    create_tables(db);

    // Seed data
    seed_data(db);

    // Display summary
    display_summary(db);

    std::cout << "\n✓ Database initialization complete!" << std::endl;

    sqlite3_close(db);
    return 0;
}
