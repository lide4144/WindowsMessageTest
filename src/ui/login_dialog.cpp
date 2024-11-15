#include "login_dialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
    connectSignals();
}

void LoginDialog::setupUi()
{
    auto mainLayout = new QVBoxLayout(this);
    
    // 用户名输入
    auto userLayout = new QHBoxLayout;
    userLayout->addWidget(new QLabel(tr("用户名:")));
    usernameEdit = new QLineEdit(this);
    userLayout->addWidget(usernameEdit);
    mainLayout->addLayout(userLayout);
    
    // 密码输入
    auto passwordLayout = new QHBoxLayout;
    passwordLayout->addWidget(new QLabel(tr("密码:")));
    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordLayout->addWidget(passwordEdit);
    mainLayout->addLayout(passwordLayout);
    
    // 服务器地址输入
    auto serverLayout = new QHBoxLayout;
    serverLayout->addWidget(new QLabel(tr("服务器:")));
    serverAddressEdit = new QLineEdit(this);
    serverAddressEdit->setText("127.0.0.1");
    serverLayout->addWidget(serverAddressEdit);
    mainLayout->addLayout(serverLayout);
    
    // 按钮布局
    auto buttonLayout = new QHBoxLayout;
    loginButton = new QPushButton(tr("登录"), this);
    cancelButton = new QPushButton(tr("取消"), this);
    buttonLayout->addWidget(loginButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);
    
    setWindowTitle(tr("聊天软件 - 登录"));
    setFixedSize(300, 200);
}

void LoginDialog::connectSignals()
{
    connect(loginButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

QString LoginDialog::getUsername() const
{
    return usernameEdit->text();
}

QString LoginDialog::getPassword() const
{
    return passwordEdit->text();
}

QString LoginDialog::getServerAddress() const
{
    return serverAddressEdit->text();
} 