#include<string.h>
#include "chat_service.h"
#include<ctime>
#include "friend_service.h"
#include "group_service.h"
#include "account_service.h"


int send_direct_message(sqlite3* db, int sender_id, int receiver_id, const std::string& content) {

    if (!friendship_exists(db, sender_id, receiver_id)) {
        return DB_NOT_FRIENDS_TO_SEND;
    }

    // There's no is_read column in here because, we let it be default (= 0)
    // if the receiver is online and is at the chat box with the sender
    // the client app will immediatelly send an update request
    const char* query = R"(
    INSERT INTO direct_messages (sender_id, receiver_id, content)
    VALUES (?, ?, ?);
    )";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        return DB_ERROR;
    }
    sqlite3_bind_int(stmt, 1, sender_id);
    sqlite3_bind_int(stmt, 2, receiver_id);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        return DB_ERROR;
    }
    sqlite3_finalize(stmt);
    return DB_SUCCESS;
}

int get_direct_message_history(sqlite3* db, int user_id1, int user_id2, std::vector<Message>& messages, int limit, int offset) {
    
    if (!friendship_exists(db, user_id1, user_id2)) {
        return DB_NOT_FRIENDS_TO_SEND;
    }
    
    const char *query = R"(
    SELECT message_id, sender_id, receiver_id, content, timestamp, is_read 
    FROM direct_messages WHERE 
    ((sender_id = ?) AND (receiver_id = ?) ) OR
    ((sender_id = ?) AND (receiver_id = ?))
    ORDER BY timestamp ASC
    LIMIT ? OFFSET ?
    )";
    
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        return DB_ERROR;
    }
    sqlite3_bind_int(stmt, 1, user_id1);
    sqlite3_bind_int(stmt, 2, user_id2);
    sqlite3_bind_int(stmt, 3, user_id2);
    sqlite3_bind_int(stmt, 4, user_id1);
    sqlite3_bind_int(stmt, 5, limit);
    sqlite3_bind_int(stmt, 6, offset);
    
    int rc;
    Message message;
    while ((rc =sqlite3_step(stmt)) ==SQLITE_ROW) {
        message = Message();
        message.message_id = sqlite3_column_int(stmt, 0);
        message.sender_id = sqlite3_column_int(stmt, 1);
        message.receiver_id = sqlite3_column_int(stmt, 2);

        const unsigned char* text = sqlite3_column_text(stmt, 3);
        // elvis operation
        message.content = text ? reinterpret_cast<const char*>(text) : "";

        message.group_id = 0;
        const unsigned char *ts = sqlite3_column_text(stmt, 4);
        message.timestamp = ts ? reinterpret_cast<const char *>(ts) : "";

        message.is_read = sqlite3_column_int(stmt, 5);
        messages.push_back(message);
    } 

    if(rc != SQLITE_DONE) {
        return DB_ERROR;
    }
    sqlite3_finalize(stmt);
    return DB_SUCCESS;
}




int mark_messages_read(sqlite3* db, int user_id, int sender_id) {
    const char* query = "UPDATE direct_messages SET is_read = 1 WHERE receiver_id = ? AND sender_id = ? AND is_read = 0";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        return DB_ERROR;
    }
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, sender_id);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return DB_ERROR;
    }
    sqlite3_finalize(stmt);
    return DB_SUCCESS;
}
