#include "loginwindow.h"
#include "mainwindow.h"

#include <QApplication>

#include <QTcpSocket>

static QApplication * app;
static LoginWindow * loginView;
static QMainWindow * mainView;

int execApp() {
    app->quit();
    return app->exec();
}

QTcpSocket * getLoginSocket() {
    return loginView->socket;
}

void openMainWindow() {
    MainWindow * mainWindow = new MainWindow();
    mainWindow->showMaximized();
    mainView = mainWindow;
}

int main(int argc, char *argv[])
{
    app = new QApplication(argc, argv);
    LoginWindow * loginWindow = new LoginWindow();
    loginWindow->show();
    loginView = loginWindow;
    return execApp();
}
