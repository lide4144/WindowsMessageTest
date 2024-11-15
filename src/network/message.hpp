#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

class Message {
public:
    enum class Type : uint8_t {
        TEXT,       // 文本消息
        JOIN,       // 加入聊天
        LEAVE,      // 离开聊天
        USER_LIST,  // 用户列表
        HEARTBEAT   // 心跳消息
    };

    Message();
    explicit Message(Type type);

    // 编码解码方法
    std::vector<uint8_t> encode() const;
    static std::shared_ptr<Message> decode(const std::vector<uint8_t>& data);

    // 设置获取消息内容
    void setContent(const std::string& content);
    void setSender(const std::string& sender);
    
    Type getType() const;
    const std::string& getContent() const;
    const std::string& getSender() const;

private:
    Type type_;
    std::string content_;
    std::string sender_;
}; 