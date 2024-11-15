#include <QApplication>
#include "ui/login_dialog.hpp"
#include "ui/main_window.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    LoginDialog loginDialog;
    if (loginDialog.exec() != QDialog::Accepted) {
        return 0;
    }
    
    MainWindow mainWindow(loginDialog.getUsername());
    mainWindow.show();
    
    return app.exec();
} 