#include "message_store.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

MessageStore::MessageStore(const std::string& dbPath)
    : dbPath_(dbPath)
{
    initialize();
}

MessageStore::~MessageStore()
{
    if (db_) {
        sqlite3_close(db_);
    }
}

bool MessageStore::initialize()
{
    if (sqlite3_open(dbPath_.c_str(), &db_) != SQLITE_OK) {
        return false;
    }

    // 创建消息表
    const char* createTableSQL = 
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sender TEXT NOT NULL,"
        "content TEXT NOT NULL,"
        "timestamp TEXT NOT NULL,"
        "type INTEGER NOT NULL"
        ")";

    return executeQuery(createTableSQL);
}

bool MessageStore::storeMessage(const Message& msg)
{
    const char* sql = 
        "INSERT INTO messages (sender, content, timestamp, type) "
        "VALUES (?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, msg.getSender().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, msg.getContent().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, getCurrentTimestamp().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, static_cast<int>(msg.getType()));

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

std::vector<MessageStore::StoredMessage> MessageStore::getMessages(size_t limit)
{
    std::vector<StoredMessage> messages;
    const char* sql = 
        "SELECT id, sender, content, timestamp, type "
        "FROM messages ORDER BY timestamp DESC LIMIT ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return messages;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(limit));

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        StoredMessage msg;
        msg.id = sqlite3_column_int64(stmt, 0);
        msg.sender = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        msg.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        msg.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        msg.type = static_cast<Message::Type>(sqlite3_column_int(stmt, 4));
        messages.push_back(msg);
    }

    sqlite3_finalize(stmt);
    return messages;
}

std::vector<MessageStore::StoredMessage> MessageStore::getMessagesSince(
    const std::string& timestamp)
{
    std::vector<StoredMessage> messages;
    const char* sql = 
        "SELECT id, sender, content, timestamp, type "
        "FROM messages WHERE timestamp > ? ORDER BY timestamp ASC";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return messages;
    }

    sqlite3_bind_text(stmt, 1, timestamp.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        StoredMessage msg;
        msg.id = sqlite3_column_int64(stmt, 0);
        msg.sender = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        msg.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        msg.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        msg.type = static_cast<Message::Type>(sqlite3_column_int(stmt, 4));
        messages.push_back(msg);
    }

    sqlite3_finalize(stmt);
    return messages;
}

bool MessageStore::cleanupOldMessages(int daysToKeep)
{
    std::stringstream ss;
    ss << "DELETE FROM messages WHERE timestamp < datetime('now', '-"
       << daysToKeep << " days')";
    return executeQuery(ss.str());
}

bool MessageStore::executeQuery(const std::string& query)
{
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, query.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

std::string MessageStore::getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif
    
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return ss.str();
} 