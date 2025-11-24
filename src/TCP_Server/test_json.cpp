#include <sqlite3.h>
#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

int main()
{
    sqlite3 *db;
    int rc = sqlite3_open("database/chat.db", &db);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return 1;
    }

    std::cout << "=== Querying accounts and wrapping in JSON ===\n" << std::endl;

    // Query all accounts
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, "SELECT id, username, password FROM accounts", -1, &stmt, NULL);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return 1;
    }

    // Create JSON array to hold all accounts
    json accounts_json = json::array();

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int id = sqlite3_column_int(stmt, 0);
        const char *username = (const char *)sqlite3_column_text(stmt, 1);
        const char *password = (const char *)sqlite3_column_text(stmt, 2);

        // Create JSON object for each account
        json account = {
            {"id", id},
            {"username", username},
            {"password", password}
        };

        accounts_json.push_back(account);
    }

    sqlite3_finalize(stmt);

    // Print the JSON array
    std::cout << "Accounts JSON (pretty):" << std::endl;
    std::cout << accounts_json.dump(4) << std::endl;  // Pretty print with 4 spaces

    std::cout << "\nAccounts JSON (compact):" << std::endl;
    std::cout << accounts_json.dump() << std::endl;  // Compact format

    // Example: Query messages between two users and wrap in JSON
    std::cout << "\n=== Messages between alice and bob (JSON) ===\n" << std::endl;

    sqlite3_stmt *msg_stmt;
    rc = sqlite3_prepare_v2(db,
        "SELECT m.message_id, a1.username AS sender, a2.username AS receiver, m.content, m.timestamp "
        "FROM messages m "
        "JOIN accounts a1 ON m.sender_id = a1.id "
        "LEFT JOIN accounts a2 ON m.receiver_id = a2.id "
        "WHERE (m.sender_id = ? AND m.receiver_id = ?) OR (m.sender_id = ? AND m.receiver_id = ?) "
        "ORDER BY m.timestamp",
        -1, &msg_stmt, NULL);

    if (rc == SQLITE_OK)
    {
        int user1 = 1;  // alice
        int user2 = 2;  // bob

        sqlite3_bind_int(msg_stmt, 1, user1);
        sqlite3_bind_int(msg_stmt, 2, user2);
        sqlite3_bind_int(msg_stmt, 3, user2);
        sqlite3_bind_int(msg_stmt, 4, user1);

        json messages_json = json::array();

        while (sqlite3_step(msg_stmt) == SQLITE_ROW)
        {
            int msg_id = sqlite3_column_int(msg_stmt, 0);
            const char *sender = (const char *)sqlite3_column_text(msg_stmt, 1);
            const char *receiver = (const char *)sqlite3_column_text(msg_stmt, 2);
            const char *content = (const char *)sqlite3_column_text(msg_stmt, 3);
            int timestamp = sqlite3_column_int(msg_stmt, 4);

            json message = {
                {"message_id", msg_id},
                {"from", sender},
                {"to", receiver},
                {"content", content},
                {"timestamp", timestamp}
            };

            messages_json.push_back(message);
        }

        std::cout << messages_json.dump(4) << std::endl;
    }

    sqlite3_finalize(msg_stmt);

    // Example: Create a response packet (like your protocol)
    std::cout << "\n=== Example: LOGIN Response (JSON) ===\n" << std::endl;

    json login_response = {
        {"type", 2000},  // RESPONSE
        {"status", 200}, // SUCCESS
        {"data", {
            {"user_id", 1},
            {"username", "alice"}
        }}
    };

    std::cout << login_response.dump(4) << std::endl;

    // Converting to string to send over socket
    std::string json_str = login_response.dump();
    std::cout << "\nAs string to send: " << json_str << std::endl;
    std::cout << "Size: " << json_str.length() << " bytes" << std::endl;

    // Parsing JSON from string
    std::cout << "\n=== Parsing JSON from string ===\n" << std::endl;
    json parsed = json::parse(json_str);
    std::cout << "Type: " << parsed["type"] << std::endl;
    std::cout << "Status: " << parsed["status"] << std::endl;
    std::cout << "User ID: " << parsed["data"]["user_id"] << std::endl;
    std::cout << "Username: " << parsed["data"]["username"] << std::endl;

    sqlite3_close(db);

    return 0;
}
