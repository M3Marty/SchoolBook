#include <QCoreApplication>

#include <server.h>
#include <dbaccessor.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Создаём и запускаем сервер
    Server server;

    return a.exec();
}
