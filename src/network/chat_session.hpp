#pragma once
#include <asio.hpp>
#include <memory>
#include <deque>
#include "message.hpp"

class ChatServer;

class ChatSession : public std::enable_shared_from_this<ChatSession> {
public:
    explicit ChatSession(asio::ip::tcp::socket socket, ChatServer& server);
    
    void start();
    void deliver(const Message& msg);
    const std::string& getUsername() const { return username_; }

private:
    void doRead();
    void doWrite();
    void handleMessage(const Message& msg);
    void startHeartbeatCheck();

    asio::ip::tcp::socket socket_;
    ChatServer& server_;
    std::vector<uint8_t> readBuffer_;
    std::deque<std::vector<uint8_t>> writeMessages_;
    std::string username_;
    bool isFirstMessage_;
    std::chrono::steady_clock::time_point lastHeartbeat_;
    asio::steady_timer heartbeatTimer_;
    static constexpr auto HEARTBEAT_TIMEOUT = std::chrono::seconds(15);
}; 