#pragma once
#include <QMainWindow>
#include <memory>
#include <asio.hpp>
#include "../network/chat_client.hpp"
#include "../database/message_store.hpp"

QT_BEGIN_NAMESPACE
class QTextEdit;
class QLineEdit;
class QPushButton;
class QListWidget;
class QLabel;
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString &username, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void sendMessage();
    void handleReceivedMessage(const QString &message);
    void connectToServer(const QString& address, uint16_t port);

private:
    void setupUi();
    void createMenus();
    void connectSignals();
    void updateConnectionStatus(const QString& status);
    void setupStatusBar();
    void loadChatHistory();
    void storeMessage(const Message& msg);

    QString username;
    QTextEdit *chatDisplay;    // 聊天显示区域
    QLineEdit *messageInput;   // 消息输入框
    QPushButton *sendButton;   // 发送按钮
    QListWidget *userList;     // 用户列表

    std::unique_ptr<asio::io_context> io_context_;
    std::unique_ptr<ChatClient> client_;
    std::unique_ptr<std::thread> network_thread_;

    QLabel* connectionStatusLabel_;
    int reconnectAttempts_{0};
    std::unique_ptr<MessageStore> messageStore_;
}; 