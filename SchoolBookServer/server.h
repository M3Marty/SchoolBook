#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>

#include <QVector>
#include <QMap>

#include <user.h>

class Server: public QTcpServer
{
    Q_OBJECT

public:
    Server();
    QTcpSocket * socket;

    void send(QByteArray &);

    User * getCurrentUser();

    User * registerUser(int id);

    static Server * SERVER;

private:
    QVector<QTcpSocket *> sockets;
    QMap<QTcpSocket *, User *> users;

public slots:
    void incomingConnection(qintptr socketDesc);
    void deleteConnection();
    void slotReadyRead();
};

#endif // SERVER_H
