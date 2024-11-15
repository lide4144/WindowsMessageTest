#include "message.hpp"
#include <cstring>

Message::Message() : type_(Type::TEXT) {}

Message::Message(Type type) : type_(type) {}

std::vector<uint8_t> Message::encode() const {
    // 消息格式: [类型(1字节)][发送者长度(2字节)][发送者][内容长度(4字节)][内容]
    std::vector<uint8_t> data;
    
    // 添加消息类型
    data.push_back(static_cast<uint8_t>(type_));
    
    // 添加发送者
    uint16_t senderLen = static_cast<uint16_t>(sender_.length());
    data.push_back(senderLen & 0xFF);
    data.push_back((senderLen >> 8) & 0xFF);
    data.insert(data.end(), sender_.begin(), sender_.end());
    
    // 添加内容
    uint32_t contentLen = static_cast<uint32_t>(content_.length());
    data.push_back(contentLen & 0xFF);
    data.push_back((contentLen >> 8) & 0xFF);
    data.push_back((contentLen >> 16) & 0xFF);
    data.push_back((contentLen >> 24) & 0xFF);
    data.insert(data.end(), content_.begin(), content_.end());
    
    return data;
}

std::shared_ptr<Message> Message::decode(const std::vector<uint8_t>& data) {
    if (data.size() < 7) return nullptr; // 最小消息长度
    
    auto msg = std::make_shared<Message>();
    size_t pos = 0;
    
    // 读取类型
    msg->type_ = static_cast<Type>(data[pos++]);
    
    // 读取发送者
    uint16_t senderLen = data[pos] | (data[pos + 1] << 8);
    pos += 2;
    msg->sender_.assign(data.begin() + pos, data.begin() + pos + senderLen);
    pos += senderLen;
    
    // 读取内容
    uint32_t contentLen = data[pos] | (data[pos + 1] << 8) | 
                         (data[pos + 2] << 16) | (data[pos + 3] << 24);
    pos += 4;
    msg->content_.assign(data.begin() + pos, data.begin() + pos + contentLen);
    
    return msg;
}

void Message::setContent(const std::string& content) {
    content_ = content;
}

void Message::setSender(const std::string& sender) {
    sender_ = sender;
}

Message::Type Message::getType() const {
    return type_;
}

const std::string& Message::getContent() const {
    return content_;
}

const std::string& Message::getSender() const {
    return sender_;
} 