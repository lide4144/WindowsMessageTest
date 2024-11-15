#pragma once
#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QLabel;
QT_END_NAMESPACE

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);
    QString getUsername() const;
    QString getPassword() const;
    QString getServerAddress() const;

private:
    void setupUi();
    void connectSignals();

    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QLineEdit *serverAddressEdit;
    QPushButton *loginButton;
    QPushButton *cancelButton;
}; 