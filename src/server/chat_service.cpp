#include<string.h>
#include "chat_service.h"
#include<ctime>
#include<friend_service.h>
#include<group_service.h>


int send_direct_message(sqlite3* db, int sender_id, int receiver_id, const std::string& content, bool is_offline) {

    const char *query;

    query = R"(
    SELECT friendship_id from friendships WHERE user_id1 = ? and user_id2 = ?
    )";


    // There's no is_read column in here because, we let it be default
    // if the receiver is online and is at the chat box with the sender
    // the client app will immidiatelly send an update request
    query = R"(
    INSERT INTO message (sender_id, receiver_id, content,  is_offline)
    VALUES (?, ?, ?, ?);
    )";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        return DB_ERROR;
    }
    sqlite3_bind_int64(stmt, 1, sender_id);
    sqlite3_bind_int64(stmt, 2, receiver_id);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, is_offline);


}