#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include "libs/json.hpp"

#define CHAT_DB "./database/chat.db"
int main()
{
    sqlite3 *db;

    int rc = sqlite3_open(CHAT_DB, &db);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    // Execute query
    char *err_msg = 0;
    rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS accounts (id INTEGER PRIMARY KEY, username TEXT, password TEXT)",
                      NULL, 0, &err_msg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    printf("Table created successfully\n");

    // Create messages table
    rc = sqlite3_exec(db,
                      "CREATE TABLE IF NOT EXISTS messages ("
                      "message_id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "sender_id INTEGER NOT NULL, "
                      "receiver_id INTEGER, "
                      "group_id INTEGER, "
                      "content TEXT NOT NULL, "
                      "timestamp INTEGER NOT NULL, "
                      "read_status TEXT DEFAULT 'unread', "
                      "is_offline INTEGER DEFAULT 0, "
                      "FOREIGN KEY(sender_id) REFERENCES accounts(id), "
                      "FOREIGN KEY(receiver_id) REFERENCES accounts(id), "
                      "FOREIGN KEY(group_id) REFERENCES groups(group_id)"
                      ")",
                      NULL, 0, &err_msg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error creating messages table: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    printf("Messages table created successfully\n");

    // INSERT data using prepared statements
    printf("\n=== Writing to database ===\n");
    sqlite3_stmt *insert_stmt;
    rc = sqlite3_prepare_v2(db, "INSERT INTO accounts (username, password) VALUES (?, ?)", -1, &insert_stmt, NULL);

    if (rc == SQLITE_OK)
    {
        // Insert alice
        sqlite3_bind_text(insert_stmt, 1, "alice", -1, SQLITE_STATIC);
        sqlite3_bind_text(insert_stmt, 2, "password123", -1, SQLITE_STATIC);

        if (sqlite3_step(insert_stmt) == SQLITE_DONE)
        {
            printf("Inserted user: alice\n");
        }

        sqlite3_reset(insert_stmt);

        // Insert bob
        sqlite3_bind_text(insert_stmt, 1, "bob", -1, SQLITE_STATIC);
        sqlite3_bind_text(insert_stmt, 2, "bobpass456", -1, SQLITE_STATIC);

        if (sqlite3_step(insert_stmt) == SQLITE_DONE)
        {
            printf("Inserted user: bob\n");
        }

        sqlite3_reset(insert_stmt);

        // Insert charlie
        sqlite3_bind_text(insert_stmt, 1, "charlie", -1, SQLITE_STATIC);
        sqlite3_bind_text(insert_stmt, 2, "charlie789", -1, SQLITE_STATIC);

        if (sqlite3_step(insert_stmt) == SQLITE_DONE)
        {
            printf("Inserted user: charlie\n");
        }
    }
    else
    {
        fprintf(stderr, "Failed to prepare insert statement: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(insert_stmt);

    // INSERT messages
    printf("\n=== Writing messages to database ===\n");
    sqlite3_stmt *msg_stmt;
    rc = sqlite3_prepare_v2(db,
                            "INSERT INTO messages (sender_id, receiver_id, group_id, content, timestamp, read_status, is_offline) "
                            "VALUES (?, ?, ?, ?, ?, ?, ?)",
                            -1, &msg_stmt, NULL);

    if (rc == SQLITE_OK)
    {
        // Message 1: alice to bob
        sqlite3_bind_int(msg_stmt, 1, 1); // sender_id (alice)
        sqlite3_bind_int(msg_stmt, 2, 2); // receiver_id (bob)
        sqlite3_bind_null(msg_stmt, 3);   // group_id (NULL for direct message)
        sqlite3_bind_text(msg_stmt, 4, "Hey Bob, how are you?", -1, SQLITE_STATIC);
        sqlite3_bind_int(msg_stmt, 5, 1732300000);
        sqlite3_bind_text(msg_stmt, 6, "read", -1, SQLITE_STATIC);
        sqlite3_bind_int(msg_stmt, 7, 0);

        if (sqlite3_step(msg_stmt) == SQLITE_DONE)
        {
            printf("Message 1: alice -> bob\n");
        }
        sqlite3_reset(msg_stmt);

        // Message 2: bob to alice
        sqlite3_bind_int(msg_stmt, 1, 2); // sender_id (bob)
        sqlite3_bind_int(msg_stmt, 2, 1); // receiver_id (alice)
        sqlite3_bind_null(msg_stmt, 3);
        sqlite3_bind_text(msg_stmt, 4, "I'm good Alice, thanks!", -1, SQLITE_STATIC);
        sqlite3_bind_int(msg_stmt, 5, 1732300100);
        sqlite3_bind_text(msg_stmt, 6, "read", -1, SQLITE_STATIC);
        sqlite3_bind_int(msg_stmt, 7, 0);

        if (sqlite3_step(msg_stmt) == SQLITE_DONE)
        {
            printf("Message 2: bob -> alice\n");
        }
        sqlite3_reset(msg_stmt);

        // Message 3: charlie to alice (offline)
        sqlite3_bind_int(msg_stmt, 1, 3); // sender_id (charlie)
        sqlite3_bind_int(msg_stmt, 2, 1); // receiver_id (alice)
        sqlite3_bind_null(msg_stmt, 3);
        sqlite3_bind_text(msg_stmt, 4, "Hi Alice! Are you there?", -1, SQLITE_STATIC);
        sqlite3_bind_int(msg_stmt, 5, 1732300200);
        sqlite3_bind_text(msg_stmt, 6, "unread", -1, SQLITE_STATIC);
        sqlite3_bind_int(msg_stmt, 7, 1); // is_offline

        if (sqlite3_step(msg_stmt) == SQLITE_DONE)
        {
            printf("Message 3: charlie -> alice (offline)\n");
        }
        sqlite3_reset(msg_stmt);

        // Message 4: alice to charlie
        sqlite3_bind_int(msg_stmt, 1, 1); // sender_id (alice)
        sqlite3_bind_int(msg_stmt, 2, 3); // receiver_id (charlie)
        sqlite3_bind_null(msg_stmt, 3);
        sqlite3_bind_text(msg_stmt, 4, "Hey Charlie!", -1, SQLITE_STATIC);
        sqlite3_bind_int(msg_stmt, 5, 1732300300);
        sqlite3_bind_text(msg_stmt, 6, "read", -1, SQLITE_STATIC);
        sqlite3_bind_int(msg_stmt, 7, 0);

        if (sqlite3_step(msg_stmt) == SQLITE_DONE)
        {
            printf("Message 4: alice -> charlie\n");
        }
    }
    else
    {
        fprintf(stderr, "Failed to prepare message insert: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(msg_stmt);

    // QUERY all data
    printf("\n=== Reading from database ===\n");
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, "SELECT id, username, password FROM accounts", -1, &stmt, NULL);

    if (rc == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int id = sqlite3_column_int(stmt, 0);
            const char *username = (const char *)sqlite3_column_text(stmt, 1);
            const char *password = (const char *)sqlite3_column_text(stmt, 2);
            printf("id=%d, username=%s, password=%s\n", id, username, password);
        }
    }
    else
    {
        fprintf(stderr, "Failed to prepare select statement: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);

    // QUERY specific user
    printf("\n=== Searching for specific user (alice) ===\n");
    sqlite3_stmt *search_stmt;
    rc = sqlite3_prepare_v2(db, "SELECT * FROM accounts WHERE username = ?", -1, &search_stmt, NULL);

    if (rc == SQLITE_OK)
    {
        sqlite3_bind_text(search_stmt, 1, "alice", -1, SQLITE_STATIC);

        if (sqlite3_step(search_stmt) == SQLITE_ROW)
        {
            int id = sqlite3_column_int(search_stmt, 0);
            const char *username = (const char *)sqlite3_column_text(search_stmt, 1);
            const char *password = (const char *)sqlite3_column_text(search_stmt, 2);
            printf("Found user: id=%d, username=%s, password=%s\n", id, username, password);
        }
        else
        {
            printf("User not found\n");
        }
    }

    sqlite3_finalize(search_stmt);

    // QUERY messages between two users (alice and bob)
    printf("\n=== Messages between alice (id=1) and bob (id=2) ===\n");
    sqlite3_stmt *msg_query;
    rc = sqlite3_prepare_v2(db,
                            "SELECT m.message_id, a1.username AS sender, a2.username AS receiver, m.content, m.timestamp, m.read_status "
                            "FROM messages m "
                            "JOIN accounts a1 ON m.sender_id = a1.id "
                            "LEFT JOIN accounts a2 ON m.receiver_id = a2.id "
                            "WHERE (m.sender_id = ? AND m.receiver_id = ?) OR (m.sender_id = ? AND m.receiver_id = ?) "
                            "ORDER BY m.timestamp",
                            -1, &msg_query, NULL);

    if (rc == SQLITE_OK)
    {
        int user1 = 1; // alice
        int user2 = 2; // bob

        sqlite3_bind_int(msg_query, 1, user1);
        sqlite3_bind_int(msg_query, 2, user2);
        sqlite3_bind_int(msg_query, 3, user2);
        sqlite3_bind_int(msg_query, 4, user1);

        while (sqlite3_step(msg_query) == SQLITE_ROW)
        {
            int msg_id = sqlite3_column_int(msg_query, 0);
            const char *sender = (const char *)sqlite3_column_text(msg_query, 1);
            const char *receiver = (const char *)sqlite3_column_text(msg_query, 2);
            const char *content = (const char *)sqlite3_column_text(msg_query, 3);
            int timestamp = sqlite3_column_int(msg_query, 4);
            const char *status = (const char *)sqlite3_column_text(msg_query, 5);

            printf("[%d] %s -> %s: \"%s\" (timestamp=%d, status=%s)\n",
                   msg_id, sender, receiver, content, timestamp, status);
        }
    }
    else
    {
        fprintf(stderr, "Failed to prepare message query: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(msg_query);

    // QUERY offline messages for alice
    printf("\n=== Offline messages for alice (id=1) ===\n");
    sqlite3_stmt *offline_query;
    rc = sqlite3_prepare_v2(db,
                            "SELECT m.message_id, a.username AS sender, m.content, m.timestamp "
                            "FROM messages m "
                            "JOIN accounts a ON m.sender_id = a.id "
                            "WHERE m.receiver_id = ? AND m.is_offline = 1 "
                            "ORDER BY m.timestamp",
                            -1, &offline_query, NULL);

    if (rc == SQLITE_OK)
    {
        sqlite3_bind_int(offline_query, 1, 1); // alice's id

        while (sqlite3_step(offline_query) == SQLITE_ROW)
        {
            int msg_id = sqlite3_column_int(offline_query, 0);
            const char *sender = (const char *)sqlite3_column_text(offline_query, 1);
            const char *content = (const char *)sqlite3_column_text(offline_query, 2);
            int timestamp = sqlite3_column_int(offline_query, 3);

            printf("[%d] From %s: \"%s\" (timestamp=%d)\n", msg_id, sender, content, timestamp);
        }
    }
    else
    {
        fprintf(stderr, "Failed to prepare offline query: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(offline_query);

    sqlite3_close(db);

    return 0;
}