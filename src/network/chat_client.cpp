#include "chat_client.hpp"
#include <iostream>

ChatClient::ChatClient(asio::io_context& io_context)
    : io_context_(io_context)
    , socket_(io_context)
    , connected_(false)
    , heartbeatTimer_(io_context)
    , checkTimer_(io_context)
    , reconnectTimer_(io_context)
{
    readBuffer_.resize(1024);
}

void ChatClient::connect(const std::string& host, uint16_t port, ConnectHandler onConnect)
{
    lastHost_ = host;
    lastPort_ = port;
    reconnectAttempts_ = 0;
    currentBackoff_ = initialBackoff_;  // 重置退避时间
    
    asio::ip::tcp::resolver resolver(io_context_);
    auto endpoints = resolver.resolve(host, std::to_string(port));

    asio::async_connect(socket_, endpoints,
        [this, onConnect](const asio::error_code& ec, const asio::ip::tcp::endpoint&)
        {
            connected_ = !ec;
            if (connected_) {
                reconnectAttempts_ = 0;
                currentBackoff_ = initialBackoff_;  // 连接成功后重置退避时间
                doRead();
                startHeartbeat();
            } else if (autoReconnect_) {
                startReconnectTimer();
            }
            if (onConnect) {
                onConnect(connected_);
            }
        });
}

void ChatClient::startHeartbeat()
{
    // 发送心跳
    heartbeatTimer_.expires_after(HEARTBEAT_INTERVAL);
    heartbeatTimer_.async_wait([this](const asio::error_code& ec) {
        if (!ec && connected_) {
            Message heartbeat(Message::Type::HEARTBEAT);
            sendMessage(heartbeat);
            startHeartbeat();
        }
    });

    // 检查心跳超时
    checkTimer_.expires_after(HEARTBEAT_TIMEOUT);
    checkTimer_.async_wait([this](const asio::error_code& ec) {
        if (!ec && connected_) {
            checkHeartbeat();
        }
    });
}

void ChatClient::checkHeartbeat()
{
    auto now = std::chrono::steady_clock::now();
    if (now - lastHeartbeat_ > HEARTBEAT_TIMEOUT) {
        // 心跳超时，断开连接
        disconnect();
        if (disconnectHandler_) {
            disconnectHandler_();
        }
    } else {
        checkTimer_.expires_after(HEARTBEAT_TIMEOUT);
        checkTimer_.async_wait([this](const asio::error_code& ec) {
            if (!ec && connected_) {
                checkHeartbeat();
            }
        });
    }
}

void ChatClient::handleHeartbeat()
{
    lastHeartbeat_ = std::chrono::steady_clock::now();
}

void ChatClient::disconnect()
{
    if (connected_) {
        asio::error_code ec;
        socket_.close(ec);
        connected_ = false;
        heartbeatTimer_.cancel();
        checkTimer_.cancel();
        reconnectTimer_.cancel();
    }
}

void ChatClient::setDisconnectHandler(DisconnectHandler handler)
{
    disconnectHandler_ = std::move(handler);
}

void ChatClient::sendMessage(const Message& msg)
{
    auto encoded = msg.encode();
    bool writeInProgress = !writeMessages_.empty();
    writeMessages_.push_back(encoded);
    
    if (!writeInProgress) {
        doWrite();
    }
}

void ChatClient::setMessageHandler(MessageHandler handler)
{
    messageHandler_ = handler;
}

void ChatClient::doRead()
{
    socket_.async_read_some(asio::buffer(readBuffer_),
        [this](const asio::error_code& ec, std::size_t length)
        {
            if (!ec) {
                if (auto msg = Message::decode({readBuffer_.begin(), 
                                              readBuffer_.begin() + length})) {
                    if (msg->getType() == Message::Type::HEARTBEAT) {
                        handleHeartbeat();
                    } else if (messageHandler_) {
                        messageHandler_(*msg);
                    }
                }
                doRead();
            } else {
                disconnect();
                if (autoReconnect_) {
                    startReconnectTimer();
                } else if (disconnectHandler_) {
                    disconnectHandler_();
                }
            }
        });
}

void ChatClient::doWrite()
{
    asio::async_write(socket_,
        asio::buffer(writeMessages_.front()),
        [this](const asio::error_code& ec, std::size_t)
        {
            if (!ec) {
                writeMessages_.pop_front();
                if (!writeMessages_.empty()) {
                    doWrite();
                }
            } else {
                socket_.close();
                connected_ = false;
            }
        });
}

void ChatClient::startReconnectTimer()
{
    if (reconnectAttempts_ >= maxReconnectAttempts_) {
        if (disconnectHandler_) {
            disconnectHandler_();
        }
        return;
    }

    // 使用带抖动的退避时间
    auto backoffWithJitter = calculateBackoffWithJitter();
    reconnectTimer_.expires_after(backoffWithJitter);
    reconnectTimer_.async_wait([this](const asio::error_code& ec) {
        if (!ec && !connected_) {
            tryReconnect();
        }
    });
    
    // 计算下一次的退避时间（不带抖动，用于显示）
    currentBackoff_ = calculateBackoff();
    
    // 如果有消息处理器，发送重连状态消息
    if (messageHandler_) {
        Message msg(Message::Type::TEXT);
        msg.setContent("重连失败，将在 " + 
                      std::to_string(backoffWithJitter.count()) + 
                      " 秒后重试... (基准时间: " +
                      std::to_string(currentBackoff_.count()) +
                      "秒)");
        messageHandler_(msg);
    }
}

void ChatClient::tryReconnect()
{
    if (connected_ || lastHost_.empty() || lastPort_ == 0) return;
    
    ++reconnectAttempts_;
    
    // 创建新的socket（因为旧的可能已经失效）
    socket_ = asio::ip::tcp::socket(io_context_);
    
    asio::ip::tcp::resolver resolver(io_context_);
    auto endpoints = resolver.resolve(lastHost_, std::to_string(lastPort_));

    asio::async_connect(socket_, endpoints,
        [this](const asio::error_code& ec, const asio::ip::tcp::endpoint&)
        {
            if (!ec) {
                connected_ = true;
                reconnectAttempts_ = 0;
                doRead();
                startHeartbeat();
                
                // 发送重连成功消息
                if (messageHandler_) {
                    Message msg(Message::Type::TEXT);
                    msg.setContent("重新连接成功");
                    messageHandler_(msg);
                }
            } else {
                startReconnectTimer();
            }
        });
}

void ChatClient::setAutoReconnect(bool enable)
{
    autoReconnect_ = enable;
}

void ChatClient::setReconnectInterval(std::chrono::seconds interval)
{
    reconnectInterval_ = interval;
}

void ChatClient::setMaxReconnectAttempts(int attempts)
{
    maxReconnectAttempts_ = attempts;
}

std::chrono::seconds ChatClient::calculateBackoff() const
{
    // 计算下一次重试的等待时间
    auto nextBackoff = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::duration<float>(currentBackoff_.count() * backoffMultiplier_)
    );
    
    // 确保不超过最大等待时间
    if (nextBackoff > maxBackoff_) {
        return maxBackoff_;
    }
    
    return nextBackoff;
}

float ChatClient::generateJitterFactor() const
{
    // 生成一个在jitterMin_和jitterMax_之间的随机数
    return jitterMin_ + (jitterMax_ - jitterMin_) * jitterDist_(gen_);
}

std::chrono::seconds ChatClient::calculateBackoffWithJitter() const
{
    // 首先计算基本的退避时间
    auto baseBackoff = calculateBackoff();
    
    // 应用抖动因子
    float jitterFactor = generateJitterFactor();
    auto jitteredDuration = std::chrono::duration<float>(baseBackoff.count() * jitterFactor);
    
    // 转换为秒
    return std::chrono::duration_cast<std::chrono::seconds>(jitteredDuration);
}
 