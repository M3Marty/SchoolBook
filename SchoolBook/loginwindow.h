#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QDialog>

#include <QTcpSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class LoginWindow; }
QT_END_NAMESPACE

class LoginWindow : public QDialog
{
    Q_OBJECT

public:
    LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

    QTcpSocket * socket;

private slots:
    void on_loginB_clicked();

    void on_exitB_clicked();

    void on_regB_clicked();

    void on_exitB_2_clicked();

    void on_goReg_clicked();

    void on_goLogin_clicked();

private:
    Ui::LoginWindow *ui;

    void slotReadyRead();

    void send(QByteArray &);
};
#endif // LOGINWINDOW_H
