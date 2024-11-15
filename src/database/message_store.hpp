#pragma once
#include <string>
#include <vector>
#include <memory>
#include <sqlite3.h>
#include "../network/message.hpp"

class MessageStore {
public:
    struct StoredMessage {
        int64_t id;
        std::string sender;
        std::string content;
        std::string timestamp;
        Message::Type type;
    };

    explicit MessageStore(const std::string& dbPath);
    ~MessageStore();

    // 初始化数据库
    bool initialize();
    
    // 存储消息
    bool storeMessage(const Message& msg);
    
    // 获取历史消息
    std::vector<StoredMessage> getMessages(size_t limit = 50);
    
    // 获取特定时间之后的消息
    std::vector<StoredMessage> getMessagesSince(const std::string& timestamp);
    
    // 清理旧消息
    bool cleanupOldMessages(int daysToKeep = 30);

private:
    bool executeQuery(const std::string& query);
    static std::string getCurrentTimestamp();

    sqlite3* db_{nullptr};
    std::string dbPath_;
}; 