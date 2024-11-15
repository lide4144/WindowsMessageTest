#pragma once
#include <asio.hpp>
#include <string>
#include <deque>
#include <functional>
#include <chrono>
#include <random>
#include "message.hpp"

class ChatClient {
public:
    using MessageHandler = std::function<void(const Message&)>;
    using ConnectHandler = std::function<void(bool)>;
    using DisconnectHandler = std::function<void()>;

    ChatClient(asio::io_context& io_context);
    
    void connect(const std::string& host, uint16_t port, ConnectHandler onConnect);
    void disconnect();
    void sendMessage(const Message& msg);
    void setMessageHandler(MessageHandler handler);
    void setDisconnectHandler(DisconnectHandler handler);
    bool isConnected() const { return connected_; }

    // 添加重连相关设置
    void setAutoReconnect(bool enable);
    void setReconnectInterval(std::chrono::seconds interval);
    void setMaxReconnectAttempts(int attempts);

    // 添加指数退避相关设置
    void setInitialBackoff(std::chrono::seconds interval) { initialBackoff_ = interval; }
    void setMaxBackoff(std::chrono::seconds interval) { maxBackoff_ = interval; }
    void setBackoffMultiplier(float multiplier) { backoffMultiplier_ = multiplier; }

    // 添加抖动相关设置
    void setJitterRange(float minPercent, float maxPercent) {
        jitterMin_ = minPercent;
        jitterMax_ = maxPercent;
    }

private:
    void doRead();
    void doWrite();
    void startHeartbeat();
    void checkHeartbeat();
    void handleHeartbeat();
    void startReconnectTimer();
    void tryReconnect();

    asio::io_context& io_context_;
    asio::ip::tcp::socket socket_;
    std::vector<uint8_t> readBuffer_;
    std::deque<std::vector<uint8_t>> writeMessages_;
    MessageHandler messageHandler_;
    DisconnectHandler disconnectHandler_;
    bool connected_;
    
    asio::steady_timer heartbeatTimer_;
    asio::steady_timer checkTimer_;
    std::chrono::steady_clock::time_point lastHeartbeat_;
    static constexpr auto HEARTBEAT_INTERVAL = std::chrono::seconds(5);
    static constexpr auto HEARTBEAT_TIMEOUT = std::chrono::seconds(15);

    std::string lastHost_;
    uint16_t lastPort_{0};
    bool autoReconnect_{true};
    int reconnectAttempts_{0};
    int maxReconnectAttempts_{5};
    std::chrono::seconds reconnectInterval_{std::chrono::seconds(3)};
    asio::steady_timer reconnectTimer_;

    // 计算下一次重连延迟
    std::chrono::seconds calculateBackoff() const;
    
    // 指数退避相关成员
    std::chrono::seconds initialBackoff_{std::chrono::seconds(1)};  // 初始等待时间
    std::chrono::seconds maxBackoff_{std::chrono::seconds(60)};     // 最大等待时间
    float backoffMultiplier_{2.0f};                                 // 退避倍数
    std::chrono::seconds currentBackoff_{std::chrono::seconds(1)};  // 当前等待时间

    // 计算带抖动的退避时间
    std::chrono::seconds calculateBackoffWithJitter() const;
    float generateJitterFactor() const;
    
    // 随机数生成相关
    mutable std::random_device rd_;
    mutable std::mt19937 gen_{rd_()};
    mutable std::uniform_real_distribution<float> jitterDist_{0.0f, 1.0f};
    
    // 抖动范围（百分比）
    float jitterMin_{0.8f};    // 默认延迟时间的80%
    float jitterMax_{1.2f};    // 默认延迟时间的120%
}; 