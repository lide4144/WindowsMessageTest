#include "chat_server.hpp"
#include "chat_session.hpp"
#include <iostream>

ChatServer::ChatServer(asio::io_context& io_context, uint16_t port)
    : io_context_(io_context)
    , acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
{
    (void)io_context_;
}

void ChatServer::start()
{
    std::cout << "服务器启动在端口: " << acceptor_.local_endpoint().port() << std::endl;
    doAccept();
}

void ChatServer::doAccept()
{
    acceptor_.async_accept(
        [this](const asio::error_code& error, asio::ip::tcp::socket socket)
        {
            if (!error) {
                std::cout << "新客户端连接: " 
                          << socket.remote_endpoint().address().to_string() 
                          << std::endl;
                
                auto session = std::make_shared<ChatSession>(std::move(socket), *this);
                session->start();
            }
            
            doAccept();
        });
}

void ChatServer::broadcastMessage(const Message& msg, std::shared_ptr<ChatSession> sender)
{
    for (const auto& [username, session] : sessions_) {
        if (!sender || session != sender) {
            session->deliver(msg);
        }
    }
}

void ChatServer::removeSession(std::shared_ptr<ChatSession> session)
{
    const std::string& username = session->getUsername();
    if (!username.empty()) {
        sessions_.erase(username);
        
        // 广播用户离开消息
        Message leaveMsg(Message::Type::LEAVE);
        leaveMsg.setSender(username);
        leaveMsg.setContent(username + " 离开了聊天室");
        broadcastMessage(leaveMsg);
        
        updateUserList();
    }
}

void ChatServer::addSession(std::shared_ptr<ChatSession> session)
{
    const std::string& username = session->getUsername();
    if (!username.empty()) {
        sessions_[username] = session;
        
        // 广播用户加入消息
        Message joinMsg(Message::Type::JOIN);
        joinMsg.setSender(username);
        joinMsg.setContent(username + " 加入了聊天室");
        broadcastMessage(joinMsg);
        
        updateUserList();
    }
}

void ChatServer::updateUserList()
{
    // 构建用户列表消息
    Message userListMsg(Message::Type::USER_LIST);
    std::string userList;
    for (const auto& [username, _] : sessions_) {
        if (!userList.empty()) userList += ",";
        userList += username;
    }
    userListMsg.setContent(userList);
    
    // 广播用户列表
    broadcastMessage(userListMsg);
} 