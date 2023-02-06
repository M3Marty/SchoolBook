#include "server.h"
#include <receiver.h>

Server * Server::SERVER = 0;

Server::Server()
{
    // Слушаем порт, если возможно
    if (!this->listen(QHostAddress::Any, 10300)) {
        qDebug() << "Server could not start";
    }
    else {
        qDebug() << "Server running";
    }

    Server::SERVER = this;
}

User * Server::registerUser(int id) {
    User * user = new User(id);
    this->users.insert(socket, user);
    return user;
}

void Server::incomingConnection(qintptr d) {
    // Попытка подключения. Настраиваем интерфейс сокета и добавляем в пул.
    socket = new QTcpSocket();
    socket->setSocketDescriptor(d);

    connect(socket, &QTcpSocket::readyRead, this, &Server::slotReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &Server::deleteConnection);

    this->sockets.push_back(socket);
    qDebug() << "Client connected: " << d;
}

void Server::deleteConnection() {
    sockets.erase(std::remove(sockets.begin(), sockets.end(), socket), sockets.end());

    auto f = users.constFind(socket);
    if (f != users.constEnd())
        users.erase(f);

    socket->deleteLater();
}

void Server::slotReadyRead() {
    // Читаем входные данные и отправляем на ресивер. За время обработки он может отправить данные в ответ
    socket = (QTcpSocket*) sender();
    QDataStream in(socket);
    in.setVersion(QDataStream::Version::Qt_6_4);

    qDebug() << "Request";

    if (in.status() == QDataStream::Status::Ok) {
        QByteArray answerData;
        QDataStream answer(&answerData, QIODevice::WriteOnly);
        answer.setVersion(QDataStream::Version::Qt_6_4);
        Receiver::receive(in, answer);
        socket->readAll();

        this->send(answerData);
    } else {
        qDebug() << "DataStream error";
    }
}

void Server::send(QByteArray & data) {
    // Отправляем данные
    qDebug() << "Send " << data.size() << " bytes";
    socket->write(data);
}

User *Server::getCurrentUser()
{
    return Server::SERVER->users.value(Server::SERVER->socket);
}
