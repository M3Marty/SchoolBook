#include "dbaccessor.h"

#include <QSqlError>

QSqlDatabase connect2Database(QSqlDatabase & db, QString name) {
    // Подключаем базу данных
    db = QSqlDatabase::addDatabase("QSQLITE", name);
    db.setDatabaseName("F:\\projects\\SchoolBook.sqlite");

    // Если соединение не установлено, то выходим из программы
    if (!db.open()) {
        qDebug() << "Could not connect to " << name << " database";
    }

    // Проверяем, что существует рут пользователь, если нет,
    // то создаём его и инвайт для него
    QSqlQuery q(db);

    q.exec("select * from sb_users where name=\"root\"");
    if (q.next())
        return db;

    q.exec("select * from sb_invites where code=\"starting-root-in\"");
    if (q.next())
        return db;

    const bool creates = q.exec(
           "insert into sb_users values (0, NULL, NULL, \"root\", \"user\", 0, 0);"
    ) && q.exec(
            "insert into sb_invites values (0, \"starting-root-in\", 0);"
        );

    if (creates)
        qDebug() << "Creadted ROOT user";
    else {
        qDebug() << "Would and could not create ROOT user";
        qDebug() << q.lastError().text();
    }

    return db;
}

DBAccessor::DBAccessor() {
    // Подключаем базы данных
    connect2Database(this->usersDb, "SchoolBookDb");
}

DBAccessor DBA;

QSqlQuery * DBAccessor::getUsersDbQuery() {
    return new QSqlQuery(this->usersDb);
}
