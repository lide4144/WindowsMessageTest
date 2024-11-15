#include "main_window.hpp"
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QMenuBar>
#include <QMenu>
#include <QDateTime>
#include <QLabel>
#include <QStatusBar>
#include <memory>
#include <asio.hpp>
#include <asio/executor_work_guard.hpp>
#include "../network/chat_client.hpp"
#include "../database/message_store.hpp"

MainWindow::MainWindow(const QString &username, QWidget *parent)
    : QMainWindow(parent)
    , username(username)
{
    setupUi();
    createMenus();
    connectSignals();
    setWindowTitle(tr("聊天软件 - %1").arg(username));

    // 初始化网络
    io_context_ = std::make_unique<asio::io_context>();
    client_ = std::make_unique<ChatClient>(*io_context_);
    
    // 配置重连参数
    client_->setAutoReconnect(true);
    client_->setMaxReconnectAttempts(5);
    
    // 配置指数退避参数
    client_->setInitialBackoff(std::chrono::seconds(1));    // 初始等待1秒
    client_->setMaxBackoff(std::chrono::seconds(60));       // 最大等待60秒
    client_->setBackoffMultiplier(2.0f);                    // 每次失败后等待时间翻倍
    
    // 配置抖动范围（延迟时间的80%到120%之间随机）
    client_->setJitterRange(0.8f, 1.2f);
    
    // 初始化消息存储
    messageStore_ = std::make_unique<MessageStore>("chat_history.db");
    
    // 加载历史消息
    loadChatHistory();
    
    // 设置消息处理器
    client_->setMessageHandler([this](const Message& msg) {
        if (msg.getType() == Message::Type::TEXT) {
            QString text = QString::fromStdString(msg.getSender() + ": " + msg.getContent());
            QMetaObject::invokeMethod(this, "handleReceivedMessage", 
                                    Qt::QueuedConnection,
                                    Q_ARG(QString, text));
            
            // 存储收到的消息
            storeMessage(msg);
        }
    });

    // 设置断开连接处理器
    client_->setDisconnectHandler([this]() {
        QMetaObject::invokeMethod(this, "updateConnectionStatus",
            Qt::QueuedConnection,
            Q_ARG(QString, tr("连接断开 - 尝试重连...")));
    });

    // 启动网络线程
    network_thread_ = std::make_unique<std::thread>(
        [this]() {
            asio::executor_work_guard<asio::io_context::executor_type> work(
                io_context_->get_executor());
            io_context_->run();
        });
}

MainWindow::~MainWindow()
{
    if (client_) {
        client_->disconnect();
    }
    if (io_context_) {
        io_context_->stop();
    }
    if (network_thread_ && network_thread_->joinable()) {
        network_thread_->join();
    }
}

void MainWindow::setupUi()
{
    auto centralWidget = new QWidget(this);
    auto mainLayout = new QHBoxLayout(centralWidget);
    
    // 左侧用户列表
    userList = new QListWidget(this);
    userList->setMaximumWidth(200);
    userList->addItem(tr("在线用户"));
    mainLayout->addWidget(userList);
    
    // 右侧聊天区域
    auto chatLayout = new QVBoxLayout;
    
    // 聊天显示区域
    chatDisplay = new QTextEdit(this);
    chatDisplay->setReadOnly(true);
    chatLayout->addWidget(chatDisplay);
    
    // 消息输入区域
    auto inputLayout = new QHBoxLayout;
    messageInput = new QLineEdit(this);
    sendButton = new QPushButton(tr("发送"), this);
    inputLayout->addWidget(messageInput);
    inputLayout->addWidget(sendButton);
    chatLayout->addLayout(inputLayout);
    
    mainLayout->addLayout(chatLayout);
    setCentralWidget(centralWidget);
    resize(800, 600);
    
    // 添加状态栏
    setupStatusBar();
}

void MainWindow::createMenus()
{
    auto fileMenu = menuBar()->addMenu(tr("文件"));
    fileMenu->addAction(tr("设置"), this, []{});
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出"), this, &QMainWindow::close);
    
    auto helpMenu = menuBar()->addMenu(tr("帮助"));
    helpMenu->addAction(tr("关于"), this, []{});
}

void MainWindow::connectSignals()
{
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendMessage);
    connect(messageInput, &QLineEdit::returnPressed, this, &MainWindow::sendMessage);
}

void MainWindow::sendMessage()
{
    QString text = messageInput->text().trimmed();
    if (text.isEmpty()) return;
    
    // 创建消息
    Message msg(Message::Type::TEXT);
    msg.setSender(username.toStdString());
    msg.setContent(text.toStdString());
    
    // 发送消息
    if (client_) {
        client_->sendMessage(msg);
    }
    
    // 显示并存储发送的消息
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString message = QString("[%1] %2: %3")
                         .arg(timestamp)
                         .arg(username)
                         .arg(text);
    
    chatDisplay->append(message);
    messageInput->clear();
    
    // 存储消息
    storeMessage(msg);
}

void MainWindow::connectToServer(const QString& address, uint16_t port)
{
    if (client_) {
        updateConnectionStatus(tr("正在连接..."));
        client_->connect(address.toStdString(), port,
            [this](bool success) {
                QString status = success ? tr("已连接") : tr("连接失败");
                QMetaObject::invokeMethod(this, "updateConnectionStatus",
                    Qt::QueuedConnection,
                    Q_ARG(QString, status));
                
                if (success) {
                    QMetaObject::invokeMethod(this, "handleReceivedMessage",
                        Qt::QueuedConnection,
                        Q_ARG(QString, tr("已连接到服务器")));
                } else {
                    QMetaObject::invokeMethod(this, "handleReceivedMessage",
                        Qt::QueuedConnection,
                        Q_ARG(QString, tr("连接服务器失败 - 尝试重连中...")));
                }
            });
    }
}

void MainWindow::handleReceivedMessage(const QString &message)
{
    chatDisplay->append(message);
}

void MainWindow::setupStatusBar()
{
    connectionStatusLabel_ = new QLabel(tr("未连接"), this);
    statusBar()->addPermanentWidget(connectionStatusLabel_);
}

void MainWindow::updateConnectionStatus(const QString& status)
{
    connectionStatusLabel_->setText(status);
}

void MainWindow::loadChatHistory()
{
    auto messages = messageStore_->getMessages(50);  // 加载最近50条消息
    for (const auto& msg : messages) {
        QString text = QString("[%1] %2: %3")
                        .arg(QString::fromStdString(msg.timestamp))
                        .arg(QString::fromStdString(msg.sender))
                        .arg(QString::fromStdString(msg.content));
        chatDisplay->append(text);
    }
    chatDisplay->append("--------以上是历史消息--------");
}

void MainWindow::storeMessage(const Message& msg)
{
    if (msg.getType() == Message::Type::TEXT) {
        messageStore_->storeMessage(msg);
    }
} 