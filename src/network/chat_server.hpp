#pragma once
#include <asio.hpp>
#include <unordered_map>
#include <memory>
#include <string>
#include "message.hpp"

class ChatSession;

class ChatServer {
public:
    explicit ChatServer(asio::io_context& io_context, uint16_t port);

    void start();
    void broadcastMessage(const Message& msg, std::shared_ptr<ChatSession> sender = nullptr);
    void removeSession(std::shared_ptr<ChatSession> session);
    void addSession(std::shared_ptr<ChatSession> session);

private:
    void doAccept();
    void updateUserList();

    asio::io_context& io_context_;
    asio::ip::tcp::acceptor acceptor_;
    std::unordered_map<std::string, std::shared_ptr<ChatSession>> sessions_;
}; 