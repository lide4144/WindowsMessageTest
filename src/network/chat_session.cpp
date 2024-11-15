#include "chat_session.hpp"
#include "chat_server.hpp"
#include <iostream>

ChatSession::ChatSession(asio::ip::tcp::socket socket, ChatServer& server)
    : socket_(std::move(socket))
    , server_(server)
    , isFirstMessage_(true)
    , heartbeatTimer_(socket_.get_executor())
{
    readBuffer_.resize(1024);
    lastHeartbeat_ = std::chrono::steady_clock::now();
    startHeartbeatCheck();
}

void ChatSession::start()
{
    doRead();
}

void ChatSession::deliver(const Message& msg)
{
    auto encoded = msg.encode();
    bool writeInProgress = !writeMessages_.empty();
    writeMessages_.push_back(encoded);
    
    if (!writeInProgress) {
        doWrite();
    }
}

void ChatSession::doRead()
{
    auto self(shared_from_this());
    socket_.async_read_some(
        asio::buffer(readBuffer_),
        [this, self](const asio::error_code& ec, std::size_t length)
        {
            if (!ec) {
                if (auto msg = Message::decode({readBuffer_.begin(), 
                                              readBuffer_.begin() + length})) {
                    handleMessage(*msg);
                }
                doRead();
            } else {
                server_.removeSession(shared_from_this());
            }
        });
}

void ChatSession::doWrite()
{
    auto self(shared_from_this());
    asio::async_write(
        socket_,
        asio::buffer(writeMessages_.front()),
        [this, self](const asio::error_code& ec, std::size_t)
        {
            if (!ec) {
                writeMessages_.pop_front();
                if (!writeMessages_.empty()) {
                    doWrite();
                }
            } else {
                server_.removeSession(shared_from_this());
            }
        });
}

void ChatSession::startHeartbeatCheck()
{
    heartbeatTimer_.expires_after(HEARTBEAT_TIMEOUT);
    heartbeatTimer_.async_wait([this, self = shared_from_this()](const asio::error_code& ec) {
        if (!ec) {
            auto now = std::chrono::steady_clock::now();
            if (now - lastHeartbeat_ > HEARTBEAT_TIMEOUT) {
                // 心跳超时，断开连接
                socket_.close();
                server_.removeSession(shared_from_this());
            } else {
                startHeartbeatCheck();
            }
        }
    });
}

void ChatSession::handleMessage(const Message& msg)
{
    lastHeartbeat_ = std::chrono::steady_clock::now();
    
    if (msg.getType() == Message::Type::HEARTBEAT) {
        // 收到心跳消息，回复心跳
        Message heartbeat(Message::Type::HEARTBEAT);
        deliver(heartbeat);
        return;
    }
    
    if (isFirstMessage_) {
        username_ = msg.getSender();
        isFirstMessage_ = false;
        server_.addSession(shared_from_this());
    } else {
        server_.broadcastMessage(msg, shared_from_this());
    }
} 