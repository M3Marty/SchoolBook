#include "loginwindow.h"
#include "./ui_loginwindow.h"

#include <stdlib.h>

#include <mainwindow.h>

#define GENERAL_INCLUDE
#define REQUEST_WRITE
#define CID_EXTERN
#include <sb_master.h>

// From main.cpp
QTcpSocket * getLoginSocket();
void execApp();
void openMainWindow();

LoginWindow::LoginWindow(QWidget *parent): QDialog(parent),
    ui(new Ui::LoginWindow)
{
    this->ui->setupUi(this);
    this->ui->stack->setCurrentIndex(0);

    this->socket = new QTcpSocket();
    this->socket->connectToHost("127.0.0.1", 10300);

    connect(this->socket, &QTcpSocket::readyRead, this, &LoginWindow::slotReadyRead);
    connect(this->socket, &QTcpSocket::disconnected, this->socket, &QTcpSocket::deleteLater);
}

void LoginWindow::slotReadyRead() {
    QDataStream data(socket);
    data.setVersion(QDataStream::Version::Qt_6_4);

    if (data.status() == QDataStream::Status::Ok) {
        READ_CHAR(pages);
        READ_CHAR(command);

        if (command == 0) {
            // Ответ на регистрацию
            READ_CHAR(answer);

            if (answer == 0) {
                // Регистрация успешна
                this->on_goLogin_clicked();
            } else if (answer == 1) {
                this->ui->loginF_2->setPlaceholderText("Логин уже существует");
                this->ui->loginF_2->setText("");
            } else if (answer == 2) {
                this->ui->codeF->setPlaceholderText("Код не существует");
                this->ui->codeF->setText("");
            } else if (answer == 3) {
                this->ui->passF_2->setPlaceholderText("Ваш уровень в системе не позволяет игнорировать пароль");
                this->ui->passF_2->setText("");
            }
        } else if (command == 1) {
            // Ответ на вход
            READ_CHAR(answer);

            if (answer == 0) {
                // Вход успешен

                INIT_STRING_READ(GROUP_LENGTH)
                        READ_STATIC_STRING(name, NAME_LENGTH);
                        READ_STATIC_STRING(surname, SURNAME_LENGTH);
                        READ_CHAR(role);
                        READ_INT(id);
                        READ_STATIC_STRING(groupName, GROUP_LENGTH);

                MainWindow::USER_NAME = name;
                MainWindow::USER_SURNAME = surname;
                MainWindow::USER_ROLE = role;
                MainWindow::USER_ID = id;
                MainWindow::USER_GROUP = groupName;
                qDebug() << "User(1) role =" << (int) role << name << surname << id << groupName;

                this->close();
                openMainWindow();
            } else if (answer == 1) {
                this->ui->loginF->setPlaceholderText("Неверный логин");
                this->ui->loginF->setText("");
            } else if (answer == 2) {
                this->ui->passF->setPlaceholderText("Неверный пароль");
                this->ui->passF->setText("");
            }
        }
    } else {
        qDebug() << "DataStream error";
    }
}

void LoginWindow::send(QByteArray & data) {
    qDebug() << "Send request";
    this->socket->write(data);
}

LoginWindow::~LoginWindow()
{
    delete ui;
}


void LoginWindow::on_loginB_clicked()
{
    QString login = this->ui->loginF->text();
    QString password = this->ui->passF->text();

    if (login.length() < 4) {
        this->ui->loginF->setPlaceholderText("Минимальная длина логина - 4 символа");
        this->ui->loginF->setText("");
    } else {
        REQUEST

        WRITE_CHAR(1);
        WRITE_CHAR(cid_login);

        INIT_STATIC_STRING_WRITE(PASS_LENGTH)
                WRITE_STATIC_STRING(login, LOGIN_LENGTH);
                WRITE_STATIC_STRING(password, PASS_LENGTH);

        send(requestData);
    }
}

void LoginWindow::on_regB_clicked()
{
    QString code = this->ui->codeF->text();
    QString login = this->ui->loginF_2->text();
    QString password = this->ui->passF_2->text();
    QString confirm = this->ui->passConfirmF->text();

    if (password != confirm) {
        this->ui->passConfirmF->setPlaceholderText("Пароли не совпадают");
        this->ui->passConfirmF->setText("");
    } else if (code.length() != 16) {
        this->ui->codeF->setPlaceholderText("Код не существует");
        this->ui->codeF->setText("");
    } else if (login.length() < 4) {
        this->ui->loginF_2->setPlaceholderText("Минимальная длина логина - 4 символа");
        this->ui->loginF_2->setText("");
    } else {
        REQUEST

        WRITE_CHAR(1);
        WRITE_CHAR(cid_register);

        INIT_STATIC_STRING_WRITE(PASS_LENGTH)
                WRITE_STATIC_STRING(code, CODE_LENGTH);
                WRITE_STATIC_STRING(login, LOGIN_LENGTH);
                WRITE_STATIC_STRING(password, PASS_LENGTH);

        send(requestData);
    }
}

void LoginWindow::on_exitB_clicked()
{
    execApp();
}

void LoginWindow::on_exitB_2_clicked()
{
    execApp();
}

void LoginWindow::on_goReg_clicked()
{
    this->ui->stack->setCurrentIndex(1);
}

void LoginWindow::on_goLogin_clicked()
{
    this->ui->stack->setCurrentIndex(0);
}

