#include "receiver.h"
#include <dbaccessor.h>
#include <server.h>

#define SERVER_RECEIVER
#define ANSWER_WRITE
#include <../SchoolBook/sb_master.h>

#include <timetablegenerator.h>

Receiver::Receiver() { }

COMDEF(register, 0) {
    // Регистрация

    INIT_STRING_READ(PASS_LENGTH)
            READ_STATIC_STRING(code, CODE_LENGTH);
            READ_STATIC_STRING(login, LOGIN_LENGTH);
            READ_STATIC_STRING(pass, PASS_LENGTH);

    QSqlQuery * q = DBA.getUsersDbQuery();

    q->exec("SELECT login from sb_users WHERE login=\"" + login + "\"");
    if (!q->next()) {
        q->exec("SELECT userId from sb_invites WHERE code = \"" + code + "\"");
        if (q->next()) {
            int userId = q->value(0).toInt();
            q->exec("SELECT role from sb_users WHERE id = " + QString::number(userId));
            q->next();
            char roleId = q->value(0).toInt();

            if (roleId < 3 && pass.length() < 8) {
                WRITE_CHAR(3);
            } else {
                q->exec("DELETE from sb_invites WHERE userId = " + QString::number(userId));

                q->exec("UPDATE sb_users"
                        " SET login = '" + login + "', password = " + QString::number(hash(pass)) +
                        " WHERE id = " + QString::number(userId));

                WRITE_CHAR(0);
            }
        } else {
            WRITE_CHAR(2);
        }
    } else  {
        WRITE_CHAR(1);
    }

    delete q;
}

COMDEF(login, 1) {
    // Вход
    INIT_STRING_READ(PASS_LENGTH)
            READ_STATIC_STRING(login, LOGIN_LENGTH);
            READ_STATIC_STRING(pass, PASS_LENGTH);

    qint64 password = hash(QString(pass));

    QSqlQuery * q = DBA.getUsersDbQuery();
    q->exec("SELECT password, id from sb_users WHERE login='" + login + "'");

    if (q->next()) {
        if (password == q->value(0).toLongLong()) {
            // Вносим пользователя в список пользователей
            User * user = Server::SERVER->registerUser(q->value(1).toInt());

            WRITE_CHAR(0);

            INIT_STATIC_STRING_WRITE(GROUP_LENGTH);
                    WRITE_STATIC_STRING(user->name, NAME_LENGTH);
                    WRITE_STATIC_STRING(user->surname, SURNAME_LENGTH);

            WRITE_CHAR(user->role);
            WRITE_INT(user->id);

            QString group;
            if (user->role == 2) {
                q->exec("SELECT g.name FROM sb_groups g JOIN sb_users_teachers t ON g.id = t.dGroupId WHERE t.id = "
                        + QString::number(user->inroleid));
                qDebug() << q->lastQuery();
                q->next();
                group = q->value(0).toString();
            } else if (user->role == 3) {
                q->exec("SELECT g.name FROM sb_groups g JOIN sb_users_students s ON g.id = s.cGroupId WHERE s.id = "
                        + QString::number(user->inroleid));
                q->next();
                group = q->value(0).toString();
            } else if (user->role == 4) {
                q->exec("SELECT u.name || ' ' || u.surname FROM sb_users u JOIN sb_users_parents p ON u.id = p.studentId WHERE p.id = "
                        + QString::number(user->inroleid));
                q->next();
                group = q->value(0).toString();
            }

            WRITE_STATIC_STRING(group, GROUP_LENGTH);
        } else {
            WRITE_CHAR(2);
        }
    } else {
        WRITE_CHAR(1);
    }

    delete q;
}

COMDEF(getLastId, 2) {
    READ_CHAR(idOf);
    READ_CHAR(transientByte);

    QSqlQuery * q = DBA.getUsersDbQuery();

    switch (idOf) {
    case 0:
        q->exec("SELECT MAX(id) FROM sb_posts");
        break;
    case 1: {
        READ_INT(id);
        if (!id)
            id = Server::SERVER->getCurrentUser()->id;

        q->exec("SELECT MAX(id) FROM sb_posts WHERE authorId = " + QString::number(id));
        break;
    }
    default:
        qDebug() << "Unexpected getLastId mode" << (int) idOf;
        return;
    }

    if (q->next())
        WRITE_INT(q->value(0).toInt());
    else
        WRITE_INT(-1);

    WRITE_CHAR(transientByte);

    delete q;
}

COMDEF(newPost, 3) {
    Q_UNUSED(answer)

    INIT_STRING_READ(TITLE_LENGTH)
            READ_STATIC_STRING(title, TITLE_LENGTH);

    READ_CHAR(group);

    QSqlQuery * q = DBA.getUsersDbQuery();

    if (group == 4) {
        INIT_STRING_READ(21)
                READ_STATIC_STRING(gName, GROUP_LENGTH);

        q->exec("SELECT id from sb_groups where name '" + gName + "'");
        q->next();
        group = q->value(0).toInt();
    }

    READ_DYNAMIC_STRING_NO_LIMIT(text, short);
    READ_DYNAMIC_STRING_NO_LIMIT(images, short);

    User * u = Server::SERVER->getCurrentUser();

    long long timeStamp = QDateTime::currentMSecsSinceEpoch();
    q->exec("INSERT INTO sb_posts VALUES"
            "(NULL, '" + title + "', " + QString::number(u->id) +
            ", '" + text + "', '" + images + "', " +
            QString::number(group) + ", " + QString::number(timeStamp) + ");");

    // TODO разослать его всем подходящим клиентам,
    // кроме автора (клиент сам сгенерирует пост)

    delete q;
}

COMDEF(loadNewFile, 4) {
    // TODO Неизвестно работает ли из-за ошибки на стороне клиента

    INIT_STRING_READ(256);
            READ_DYNAMIC_STRING(name, char);

    name = name.sliced(name.lastIndexOf('/'));

    QSqlQuery * q = DBA.getUsersDbQuery();
    q->exec("SELECT id from sb_files WHERE name = '" + name + "'");
    if (q->next()) {
        WRITE_CHAR(1);
        return;
    }

    QFile image("server/files/" + name);

    READ_INT(size);

    // Частный случай работы с входными данными, где читается большое количество сырых данных
    // Соответственно нет смысла в отдельном макросе
    char * d = (char *) malloc(size);
    data.readRawData(d, size);
    image.write(d, size);
    free(d);

    User * u = Server::SERVER->getCurrentUser();

    q->exec("INSERT INTO sb_files VALUES "
            "(NULL, " + QString::number(u->id) + ", '" + name + "')");
}

COMDEF(getPosts, 5) {
    // TODO сделать выборку, но для начала нужно наполнить базу данных (готово)
    // пока что будем просто брать все
    READ_INT(postId);
    READ_CHAR(number);

    User * u = Server::SERVER->getCurrentUser();
    QSqlQuery * q = DBA.getUsersDbQuery();

    if (u->role == 0) {
        // Все
        q->exec("SELECT p.*, u.name || ' ' || u.surname FROM sb_posts p"
                " JOIN sb_users u ON p.author = u.id WHERE p.id <= " + QString::number(postId) +
                " ORDER BY p.id DESC LIMIT " + QString::number(number));

    } else if (u->role == 1) {
        // То, что постили сами
        // То, что постили завучи или администраторы для всех
        // То, что постили их подчиненённые учителя
        q->exec("SELECT p.*, u.name || ' ' || u.surname FROM ("
                "SELECT * FROM sb_posts WHERE author = " + QString::number(u->id) +
                " UNION ALL SELECT p.* FROM sb_posts p JOIN sb_users u ON p.author = u.id"
                " WHERE u.role <= 1 AND p.`group` = 0 AND author != " + QString::number(u->id) +
                " UNION ALL SELECT p.* FROM sb_posts p JOIN sb_groups_d" + QString::number(u->inroleid) +
                " d ON p.author = d.memberId) p JOIN sb_users u"
                " ON p.author = u.id WHERE p.id <= " + QString::number(postId) +
                " ORDER BY p.id DESC LIMIT " + QString::number(number));
    } else if (u->role == 2) {
        // То, что постили сами
        // То, что постили завучи для всех
        // То, что постил администратор для учителей или всех
        // То, что постил ответственный завуч для подчиненных или учителей
        // То, что постили для их группы
        q->exec("SELECT p.*, u.name || ' ' || u.surname FROM ("
                "SELECT * FROM sb_posts WHERE author = " + QString::number(u->id) +
                " UNION ALL SELECT p.* FROM sb_posts p JOIN sb_users u ON p.author = u.id"
                " WHERE (u.role = 0 AND p.`group` IN (0, 1)) OR (u.role = 1 AND p.`group` = 0)"
                " UNION ALL SELECT p.* FROM sb_posts p JOIN sb_users_teachers t "
                " ON p.`group` = t.dGroupId WHERE t.id = " + QString::number(u->inroleid) +
                " UNION ALL SELECT p.* FROM sb_posts p WHERE p.`group` IN (1, 2) AND author IN ("
                " SELECT u.id FROM sb_groups g JOIN sb_users_teachers t ON g.id = t.dGroupId"
                " JOIN sb_users u ON g.leader = u.id WHERE t.id = " + QString::number(u->inroleid) +
                ")) p JOIN sb_users u ON p.author = u.id WHERE p.id <= " + QString::number(postId) +
                " ORDER BY p.id DESC LIMIT " + QString::number(number));
    } else if (u->role == 3){
        // То, что постил администратор для учеников или всех
        // То, что постил завуч для всех
        // То, что постил их учитель
        // То, что постил их завуч для подчиненных или учеников
        // То, что постилии для их группы
        q->exec("SELECT DISTINCT p.*, u.name || ' ' || u.surname FROM ("
                " SELECT p.* FROM sb_posts p JOIN sb_users u ON p.author = u.id WHERE (u.role = 0 AND p.`group` IN (0, 3)) OR (u.role = 1 AND p.`group` = 0)"
                " UNION ALL SELECT p.* FROM sb_posts p JOIN sb_groups g ON p.author = g.leader JOIN sb_users_students s ON g.id = s.cGroupId WHERE s.id = " + QString::number(u->inroleid) +
                " UNION ALL SELECT p.* FROM sb_users_students s JOIN sb_groups g1 ON s.cGroupId = g1.id JOIN sb_users u ON g1.leader = u.id JOIN sb_users_teachers t ON u.inroleid = t.id"
                " JOIN sb_groups g2 ON t.dGroupId = g2.id JOIN sb_posts p ON g2.leader = p.author WHERE p.`group` IN (2, 3) AND s.id = " + QString::number(u->inroleid) +
                " UNION ALL SELECT p.* FROM sb_posts p JOIN sb_users_students s ON p.`group` = s.cGroupId WHERE s.id = " + QString::number(u->inroleid) +
                " ) p JOIN sb_users u ON p.author = u.id WHERE p.id <= " + QString::number(postId) +
                " ORDER BY p.id DESC LIMIT " + QString::number(number));

    } else {
        // То, что видит их ученик
        q->exec("SELECT u.inroleid FROM sb_users_parents p JOIN sb_users ON p.studentId = u.id");
        q->next();
        int inroleid = q->value(0).toInt();
        q->exec("SELECT DISTINCT p.*, u.name || ' ' || u.surname FROM ("
                " SELECT p.* FROM sb_posts p JOIN sb_users u ON p.author = u.id WHERE (u.role = 0 AND p.`group` IN (0, 3)) OR (u.role = 1 AND p.`group` = 0)"
                " UNION ALL SELECT p.* FROM sb_posts p JOIN sb_groups g ON p.author = g.leader JOIN sb_users_students s ON g.id = s.cGroupId WHERE s.id = " + QString::number(inroleid) +
                " UNION ALL SELECT p.* FROM sb_users_students s JOIN sb_groups g1 ON s.cGroupId = g1.id JOIN sb_users u ON g1.leader = u.id JOIN sb_users_teachers t ON u.inroleid = t.id"
                " JOIN sb_groups g2 ON t.dGroupId = g2.id JOIN sb_posts p ON g2.leader = p.author WHERE p.`group` IN (2, 3) AND s.id = " + QString::number(inroleid) +
                " UNION ALL SELECT p.* FROM sb_posts p JOIN sb_users_students s ON p.`group` = s.cGroupId WHERE s.id = " + QString::number(inroleid) +
                " ) p JOIN sb_users u ON p.author = u.id WHERE p.id <= " + QString::number(postId) +
                " ORDER BY p.id DESC LIMIT " + QString::number(number));
    }

    QSqlQuery * authorName = DBA.getUsersDbQuery();

    char isAnotherPostAvailable = true;
    INIT_STATIC_STRING_WRITE(TITLE_LENGTH)
    while (q->next()) {

        int id = q->value(0).toInt();
        QString name = q->value(1).toString();
        int authorId = q->value(2).toInt();
        QString text = q->value(3).toString();
        long long timeStamp = q->value(6).toLongLong();

        authorName->exec("SELECT name || ' ' || surname FROM sb_users "
                         "WHERE id = " + QString::number(authorId));
        authorName->next();
        QString author = authorName->value(0).toString();

        WRITE_CHAR(isAnotherPostAvailable);

        WRITE_INT(id);
        WRITE_STATIC_STRING(name, TITLE_LENGTH);
        WRITE_INT(authorId);
        WRITE_STATIC_STRING(author, NAME_SURNAME_LENGTH);
        WRITE_DYNAMIC_STRING(text, short);
        WRITE_LONG(timeStamp);
    }

    WRITE_CHAR(0);

    delete q;
    delete authorName;

    return;
}

COMDEF(getFile, 6) {
    // TODO
}

COMDEF(editPost, 7) {
    Q_UNUSED(answer)

    READ_CHAR(mode);
    READ_INT(post);

    QSqlQuery * q = DBA.getUsersDbQuery();
    switch (mode) {
    case 0:
        // TODO Редактирование
        break;
    case 1:
        q->exec("DELETE from sb_posts WHERE id = " + QString::number(post));
        break;
    }
    delete q;
}

COMDEF(getMyGroups, 8) {
    Q_UNUSED(data)

    User * u = Server::SERVER->getCurrentUser();
    QSqlQuery * q = DBA.getUsersDbQuery();

    INIT_STATIC_STRING_WRITE(GROUP_LENGTH);
    switch (u->role) {
    case 0:
        q->exec("SELECT name FROM sb_groups WHERE active = 1 ORDER BY nameId");

        WRITE_INT(size(q));

        while (q->next()) {
            WRITE_STATIC_STRING(q->value(0).toString(), GROUP_LENGTH);
        }

       WRITE_INT(0);
        break;
    case 1: {
        q->exec("SELECT gName FROM (SELECT name AS gName FROM sb_groups g JOIN sb_groups_d" + QString::number(u->inroleid) +
                " d ON g.leader = d.memberId UNION ALL SELECT name FROM sb_groups WHERE leader = " + QString::number(u->id) +
                ") ORDER BY gName DESC");

        WRITE_INT(size(q));

        while (q->next()) {
            WRITE_STATIC_STRING(q->value(0).toString(), GROUP_LENGTH);
        }

        WRITE_INT(0);

        break;
    }
    case 2: {
        q->exec("SELECT name FROM sb_groups WHERE active = 1 and leader = " + QString::number(u->id) + " ORDER BY name");

        WRITE_INT(size(q));

        while (q->next()) {
            WRITE_STATIC_STRING(q->value(0).toString(), GROUP_LENGTH);
        }
        q->exec("SELECT g.name FROM sb_users u JOIN sb_users_teachers t ON t.id = u.inroleid "
                "JOIN sb_groups g ON t.dGroupId = g.id WHERE u.id = " + QString::number(u->id));
        q->next();

        WRITE_INT(1);
        WRITE_STATIC_STRING(q->value(0).toString(), GROUP_LENGTH);

        break;
    }
    case 3: {
        WRITE_INT(0);

        q->exec("SELECT g.name FROM sb_users u JOIN sb_users_students s ON s.id = u.inroleid "
                "JOIN sb_groups g ON s.cGroupId = g.id WHERE u.id = " + QString::number(u->id));
        q->next();

        WRITE_INT(1);
        WRITE_STATIC_STRING(q->value(0).toString(), GROUP_LENGTH);

        break;
    }
    case 4: {
        WRITE_INT(0);

        q->exec("SELECT g.name FROM sb_groups g"
                " JOIN sb_users_students s ON g.id = s.cGroupId"
                " JOIN sb_users u ON s.id = u.inroleid"
                " JOIN sb_users_parents p ON u.id = p.studentId"
                " WHERE p.id = " + QString::number(u->inroleid));
        q->next();

        WRITE_INT(1);
        WRITE_STATIC_STRING(q->value(0).toString(), GROUP_LENGTH);
        break;
    }
    }

    delete q;
}

COMDEF(getProfileData, 9) {
    READ_INT(id);
    User * user = Server::SERVER->getCurrentUser();
    if (id)
        user = new User(id);

    INIT_STATIC_STRING_WRITE(NAME_SURNAME_LENGTH);
    WRITE_INT(id);
            WRITE_STATIC_STRING(user->name, 16);
            WRITE_STATIC_STRING(user->surname, 16);
            WRITE_CHAR(user->role);

    QSqlQuery * q = DBA.getUsersDbQuery();

    QString groupName;
    switch (user->role) {
    case 0:
        groupName = "";
        break;
    case 1:
        groupName = "";
        break;
    case 2: {
        q->exec("SELECT g.name FROM sb_users u JOIN sb_users_teachers t ON t.id = u.inroleid "
                " JOIN sb_groups g ON t.dGroupId = g.id WHERE u.id = " + QString::number(user->id));
        q->next();

        groupName = q->value(0).toString();

        break;
    }
    case 3: {
        q->exec("SELECT g.name FROM sb_users u JOIN sb_users_students s ON s.id = u.inroleid "
                " JOIN sb_groups g ON s.cGroupId = g.id WHERE u.id = " + QString::number(user->id));
        q->next();

        groupName = q->value(0).toString();

        break;
    }
    case 4: {
        q->exec("SELECT u.name || ' ' || u.surname FROM sb_users u"
                " JOIN sb_users_parents p ON u.id = p.studentId WHERE p.id = " + QString::number(user->inroleid));
        q->next();

        groupName = q->value(0).toString();

        break;
    }
    }

    WRITE_STATIC_STRING(groupName, NAME_SURNAME_LENGTH);

    if (id)
        delete user;
    delete q;
}

COMDEF(getChats, 10) {
    Q_UNUSED(data)

    QSqlQuery * q = DBA.getUsersDbQuery();
    QString n = "sb_chats_u" + QString::number(Server::SERVER->getCurrentUser()->id);
    q->exec("SELECT COUNT(*) FROM " + n);
    q->next();
    WRITE_CHAR(q->value(0).toInt());

    q->exec("SELECT c.id, name FROM sb_chats c JOIN " + n + "n ON n.chatId = c.id");
    while (q->next()) {
        WRITE_INT(q->value(0).toInt());
        WRITE_DYNAMIC_STRING(q->value(1).toString(), char);
    }

    delete q;
}

COMDEF(getTasks, 11) {
    Q_UNUSED(data)

    QSqlQuery * q = DBA.getUsersDbQuery();
    QString n = "sb_tasks_u" + QString::number(Server::SERVER->getCurrentUser()->id);
    q->exec("SELECT COUNT(*) FROM " + n + " WHERE status < 2");
    q->next();
    WRITE_INT(q->value(0).toInt());

    INIT_STATIC_STRING_WRITE(NAME_SURNAME_LENGTH)
    q->exec("SELECT t.id, t.authorId, u.name || ' ' || u.surname, t.text, n.status FROM sb_tasks t JOIN " + n + " n ON n.taskid = t.id JOIN sb_users u ON u.id = t.authorId WHERE n.status < 2");
    while (q->next()) {
        int id = q->value(0).toInt();
        int authorId = q->value(1).toInt();
        QString aName = q->value(2).toString();
        QString text = q->value(3).toString();
        char status = q->value(4).toInt();

        WRITE_INT(id);
        WRITE_INT(authorId);

        WRITE_STATIC_STRING(aName, NAME_SURNAME_LENGTH);
        WRITE_DYNAMIC_STRING(text, short);
        WRITE_CHAR(status);
    }

    delete q;
}

COMDEF(getMessages, 12) {
    READ_INT(chatId);
    READ_INT(fromId);
    READ_INT(number);

    QSqlQuery * q = DBA.getUsersDbQuery();

    q->exec("SELECT * FROM sb_posts WHERE id <= " +
            QString::number(fromId) +
            " ORDER BY id DESC limit " +
            QString::number(number));

    // TODO
}

COMDEF(getCode, 13) {
    // Самая ответственная функция отвечающая за большую
    // часть функционала. Именно она создаёт пользователя
    // и позволяет ему не потеряться, сразу размещая во
    // все возможные структуры, создаёт ему таблицы и так
    // далее. Что-то возле по важности - это создание класса
    // или расписания.

    INIT_STRING_READ(NAME_SURNAME_LENGTH)
            READ_STATIC_STRING(name, NAME_LENGTH);
            READ_STATIC_STRING(surname, NAME_LENGTH);

            READ_CHAR(role);
            READ_DYNAMIC_STRING(subdata, char);

    QSqlQuery * q = DBA.getUsersDbQuery();

    q->exec("SELECT MAX(inroleid) FROM sb_users WHERE role = " + QString::number(role));
    q->next();
    int inroleid = q->value(0).toInt() + 1;
    int id;
    char code[17];
    code[16] = 0;
    QRandomGenerator g(hash(name + surname) + QDateTime::currentMSecsSinceEpoch());
    for (int i = 0; i < 16; i++) {
        int r = g.bounded(0, 61);
        if (r < 10)
            code[i] = '0' + r;
        else if (r < 36)
            code[i] = 'A' - 10 + r;
        else
            code[i] = 'a' - 36 + r;
    }

    // Ещё одно исключение с записью сырых данных
    answer.writeRawData(code, 16);

    q->exec("INSERT INTO sb_users VALUES "
            "(NULL, NULL, NULL, '" + name + "', '" + surname + "', " +
            QString::number(role) + ", " + QString::number(inroleid) + ")");

    id = q->lastInsertId().toInt();

    q->exec("INSERT INTO sb_invites VALUES "
            "(NULL, '" + QString(code) + "', " + QString::number(id) + ")");

    q->exec("CREATE TABLE sb_tasks_u" + QString::number(id) + " ("
            "id     INTEGER     PRIMARY KEY,"
            "taskId INTEGER     NOT NULL,"
            "status INTEGER (1))"
    );

    q->exec("CREATE TABLE sb_chats_u" + QString::number(id) + " ("
            "id     INTEGER     PRIMARY KEY,"
            "chatId INTEGER NOT NULL)"
    );

    QString groupName;

    switch (role) {
    case 0:
        groupName = "";
        break;
    case 1:
        groupName = "";

        q->exec("CREATE TABLE sb_groups_d" + QString::number(inroleid) + " ("
                "id     INTEGER     PRIMARY KEY,"
                "memberId INTEGER NOT NULL)"
        );
        q->exec("INSERT INTO sb_groups VALUES "
                "(NULL, 'd" + QString::number(inroleid) + "', " +
                QString::number(id) + ", 'Уч. " + surname + "', 1)");
        break;
    case 2: {
        int dGroupId;
        User * u = Server::SERVER->getCurrentUser();
        QString name;
        if (u->role) {
            groupName = "Уч." + u->surname;

            q->exec("SELECT id FROM sb_groups WHERE nameId = 'd" + QString::number(u->inroleid) + "'");
            q->next();
            name = "sb_groups_d" + QString::number(u->inroleid);
            dGroupId = q->value(0).toInt();
        } else {
            groupName = subdata;

            q->exec("SELECT nameId, id FROM sb_groups WHERE name = '" + subdata + "'");
            q->next();
            name = "sb_groups_" + q->value(0).toString();
            dGroupId = q->value(1).toInt();
        }
        q->exec("INSERT INTO " + name + " VALUES"
                "(NULL, " + QString::number(id) + ")");
        qDebug() << q->lastError().text();
        q->exec("INSERT INTO sb_users_teachers VALUES "
                "(" + QString::number(inroleid) + ", " + QString::number(dGroupId) + ")");
        break;
    }
    case 3: {
        groupName = subdata;

        q->exec("SELECT id, nameId FROM sb_groups WHERE name = '" + subdata + "'");
        q->next();
        int cGroupId = q->value(0).toInt();
        QString name = "sb_groups_" + q->value(1).toString();
        q->exec("INSERT INTO " + name + " (memberId) VALUES (" + QString::number(id) + ")");
        q->exec("INSERT INTO sb_users_students VALUES "
                "(" + QString::number(inroleid) + ", " + QString::number(cGroupId) + ")");
        qDebug() << q->lastQuery();

        break;
    }
    case 4:
        q->exec("SELECT surname FROM sb_users WHERE id = " + subdata);
        q->next();
        groupName = "Родитель " + q->value(0).toString();
        qDebug() << q->lastQuery();

        q->exec("INSERT INTO sb_users_parents VALUES "
                "(" + QString::number(inroleid) + ", " + subdata + ")");

        qDebug() << "Parent" << groupName;
        break;
    }

    WRITE_INT(id);

    INIT_STATIC_STRING_WRITE(NAME_SURNAME_LENGTH);
            WRITE_STATIC_STRING(name, NAME_LENGTH);
            WRITE_STATIC_STRING(surname, SURNAME_LENGTH);
            WRITE_CHAR(role);
            WRITE_STATIC_STRING(groupName, NAME_SURNAME_LENGTH);

    delete q;
}

COMDEF(createClass, 14) {
    Q_UNUSED(answer)

    READ_CHAR(classNumber);
    READ_CHAR(classLiteraId);
    READ_INT(classTeacherId);

    QString className = "c" + getCurrentYear() + "_" + QString::number(classNumber) + "_" + QString::number(classLiteraId);
    QString classLitera = QString("АБВГДЕЖЗИКЛМНО").at(classLiteraId);
    QString classPeopleName = "Класс " + QString::number(classNumber) + " \"" + classLitera + "\"";

    QSqlQuery * q = DBA.getUsersDbQuery();
    q->exec("INSERT INTO sb_groups VALUES "
            "(NULL, '" + className + "', " + QString::number(classTeacherId) + ", '" + classPeopleName + "', 1)");

    q->exec("CREATE TABLE sb_groups_" + className + " ("
            "id     INTEGER     PRIMARY KEY,"
            "memberId INTEGER NOT NULL)"
    );

    q->exec("CREATE TABLE sb_lessons_" + className + " ("
            "id        INTEGER PRIMARY KEY,"
            "lessonId   INTEGER NOT NULL,"
            "teacherId INTEGER)"
    );

    delete q;
}

COMDEF(occuipedLiters, 15) {
    Q_UNUSED(data)

    char numbers[11];
    QSqlQuery * q = DBA.getUsersDbQuery();
    for (int i = 0; i < 11; i++) {
        q->exec("SELECT COUNT(*) FROM sqlite_schema WHERE type ='table' AND "
                "name LIKE 'sb_groups_c" + getCurrentYear() + "_" + QString::number(i + 1) + "_%'");
        q->next();
        numbers[i] = q->value(0).toInt();
    }

    // И ещё одно исключение, кажется пора уже писать отдельно для сырых данных
    answer.writeRawData(numbers, 11);
}

COMDEF(getMyTeachers, 16) {
    Q_UNUSED(data)

    User * u = Server::SERVER->getCurrentUser();

    QSqlQuery * q = DBA.getUsersDbQuery();
    if (Server::SERVER->getCurrentUser()->role == 1)
        q->exec("SELECT u.id, u.name, u.surname, u.role, '" +
                u->surname +
                "' FROM sb_users u JOIN sb_groups_d" +
                QString::number(u->inroleid) +
                " d ON u.id = d.memberId");
    else if (Server::SERVER->getCurrentUser()->role == 0)
        q->exec("SELECT id, name, surname, role, gName FROM ("
                "SELECT u.id, u.name, u.surname, u.role, g.name AS gName FROM sb_users u JOIN sb_users_teachers t"
                " ON t.id = u.inroleid JOIN sb_groups g ON g.id = t.dGroupId WHERE u.role = 2"
                " UNION ALL SELECT id, name, surname, role, 'Завуч' FROM sb_users WHERE role = 1) ORDER BY id");

    WRITE_INT(size(q));

    INIT_STATIC_STRING_WRITE(NAME_LENGTH);
    while (q->next()) {
        WRITE_INT(q->value(0).toInt());
        WRITE_STATIC_STRING(q->value(1).toString(), NAME_LENGTH);
        WRITE_STATIC_STRING(q->value(2).toString(), SURNAME_LENGTH);
        WRITE_CHAR(q->value(3).toInt());
        WRITE_STATIC_STRING(q->value(4).toString(), NAME_SURNAME_LENGTH);
    }
}

COMDEF(getMyStudents, 17) {
    Q_UNUSED(data)

    QSqlQuery * q = DBA.getUsersDbQuery();
    User * u = Server::SERVER->getCurrentUser();
    if (u->role == 0)
        q->exec("SELECT u.id, u.name, u.surname, g.name FROM sb_users u JOIN sb_users_students s ON u.inroleid = s.id "
                "JOIN sb_groups g ON s.cGroupId = g.id WHERE u.role = 3 ORDER BY u.name, u.surname");
    else if (u->role == 1) {
        q->exec("SELECT g.nameId, g.name FROM sb_groups_d" + QString::number(u->inroleid) +
                " d JOIN sb_groups g ON d.memberId = g.leader JOIN sb_users u ON g.leader = u.id");

        QVector<QPair<QString, QString>> ni;
        while (q->next())
            ni.push_back(QPair<QString, QString>(q->value(0).toString(), q->value(1).toString()));

        QString query = "SELECT u.id, u.name, u.surname, gName FROM (";
        if (ni.length() > 0) {
            query += "SELECT memberId AS mId, '" + ni[0].second + "' AS gName FROM sb_groups_" + ni[0].first;
            for (int i = 1; i < ni.length(); i++) {
                query += " UNION ALL SELECT memberId, '" + ni[i].second + "' AS gName FROM sb_groups_" + ni[i].first;
            }
            query += ") JOIN sb_users u ON u.id = mId ORDER BY u.name, u.surname";
        }

        q->exec(query);
    } else if (u->role == 2)
        q->exec("SELECT u.id, u.name, u.surname, g.name FROM sb_users u JOIN sb_users_students s ON u.inroleid = s.id "
                "JOIN sb_groups g ON s.cGroupId = g.id WHERE u.role = 3 and g.leader = " + QString::number(u->id) +
                " ORDER BY u.name, u.surname");

    WRITE_INT(size(q));
    INIT_STATIC_STRING_WRITE(NAME_SURNAME_LENGTH)
    while (q->next()) {
        WRITE_INT(q->value(0).toInt());
        WRITE_STATIC_STRING(q->value(1).toString(), NAME_LENGTH);
        WRITE_STATIC_STRING(q->value(2).toString(), SURNAME_LENGTH);
        WRITE_CHAR(STUDENT_ROLE);
        WRITE_STATIC_STRING(q->value(3).toString(), NAME_SURNAME_LENGTH);
    }

    delete q;
}

COMDEF(transferStudent, 18) {
    Q_UNUSED(answer)

    INIT_STRING_READ(GROUP_LENGTH)
            READ_INT(id);
            READ_STATIC_STRING(to, GROUP_LENGTH);

    QSqlQuery * q = DBA.getUsersDbQuery();
    q->exec("SELECT id, nameId FROM sb_groups WHERE name = '" + to + "'");
    q->next();
    int idITo = q->value(0).toInt();
    QString idTo = q->value(1).toString();

    q->exec("SELECT g.nameId, u.inroleid FROM sb_users u JOIN sb_users_students s ON u.inroleid = s.id"
            " JOIN sb_groups g ON s.cGroupId = g.id WHERE u.id = " + QString::number(id));
    q->next();
    QString idFrom = q->value(0).toString();
    int inroleid = q->value(1).toInt();

    q->exec("DELETE from sb_groups_" + idFrom + " WHERE memberId = " + QString::number(id));
    q->exec("INSERT INTO sb_groups_" + idTo + " (memberId) VALUES (" + QString::number(id) + ")");
    q->exec("UPDATE sb_users_students SET cGroupId = " + QString::number(idITo) + " WHERE id = " + QString::number(inroleid));
}

COMDEF(setMasterTeacher, 19) {
    Q_UNUSED(answer)

    INIT_STRING_READ(GROUP_LENGTH)
            READ_STATIC_STRING(name, GROUP_LENGTH);
            READ_INT(tId);

    QSqlQuery * q = DBA.getUsersDbQuery();
    q->exec("UPDATE sb_groups SET leader = " + QString::number(tId) + " WHERE active = 1 AND name = '" + name + "'");
}

COMDEF(editLesson, 20) {
    READ_CHAR(mode);

    INIT_STRING_READ(NAME_LENGTH)
            READ_STATIC_STRING(name, NAME_LENGTH);

    QSqlQuery * q = DBA.getUsersDbQuery();

    int lId;

    if (mode) {
        q->exec("SELECT id FROM sb_lessons WHERE name = '" + name + "'");
        q->next();
        lId = q->value(0).toInt();
    }

    switch (mode) {
    case 0:
        // Добавить
        if (q->exec("INSERT INTO sb_lessons (name) VALUES ('" + name + "')"))
            WRITE_CHAR(0);
        else
            WRITE_CHAR(1);
        break;
    case 1: {
        // Сменить для класса учителя по уроку
        int tId;
        READ_STATIC_STRING(cName, NAME_LENGTH);
        data >> tId;
        q->exec("SELECT nameId FROM sb_groups WHERE name = '" + cName + "'");
        q->next();
        QString fName = "sb_lessons_" + q->value(0).toString();
        if (q->exec("UPDATE " + fName + " SET tId = " + QString::number(tId) + "WHERE lessonId = " + QString::number(lId)))
            WRITE_CHAR(0);
        else
            WRITE_CHAR(1);
        break;
    }
    case 2: {
        // Добавить классу урок
        READ_STATIC_STRING(cName, NAME_LENGTH);
        q->exec("SELECT nameId FROM sb_groups WHERE name = '" + cName + "'");
        q->next();
        QString fName = "sb_lessons_" + q->value(0).toString();
        if (q->exec("INSERT INTO " + fName + "(lessonId) VALUES (" + QString::number(lId) + ")"))
            WRITE_CHAR(0);
        else
            WRITE_CHAR(1);
        break;
    }
    case 3: {
        // Удалить урок у класса
        READ_STATIC_STRING(cName, NAME_LENGTH);
        q->exec("SELECT nameId FROM sb_groups WHERE name = '" + cName + "'");
        q->next();
        QString fName = "sb_lessons_" + q->value(0).toString();
        if (q->exec("DELETE FROM " + fName + " WHERE lessonId = " + QString::number(lId)))
            WRITE_CHAR(0);
        else
            WRITE_CHAR(1);
        break;
        break;
    }
    case 4: {
        // Поменять название
        READ_STATIC_STRING(newName, NAME_LENGTH);
        if (q->exec("UPDATE sb_lessons SET name = '" + newName + "' WHERE id = " + QString::number(lId)))
            WRITE_CHAR(0);
        else
            WRITE_CHAR(1);
        break;
    }
    case 5:
        // Удалить
        if (q->exec("UPDATE sb_lessons SET name = '' WHERE id = " + QString::number(lId)))
            WRITE_CHAR(0);
        else
            WRITE_CHAR(1);
        break;

    }
}

COMDEF(getUserPosts, 21) {
    READ_INT(uId);
}

COMDEF(updateTaskStatus, 22) {
    Q_UNUSED(answer)

    READ_INT(taskId);
    READ_CHAR(status);

    QString n = "sb_tasks_u" + QString::number(Server::SERVER->getCurrentUser()->id);
    QSqlQuery * q = DBA.getUsersDbQuery();

    q->exec("UPDATE " + n + " SET status = " + QString::number(status) + " WHERE taskId = " + QString::number(taskId));

    delete q;
}

COMDEF(getLessonsList, 23) {
    Q_UNUSED(data)

    QSqlQuery * q = DBA.getUsersDbQuery();
    q->exec("SELECT name FROM sb_lessons");
    WRITE_INT(size(q));
    INIT_STATIC_STRING_WRITE(NAME_LENGTH)
    while (q->next()) {
        WRITE_STATIC_STRING(q->value(0).toString(), NAME_LENGTH);
    }
}

COMDEF(teachersOfLessonsForClass, 24) {
    INIT_STRING_READ(GROUP_LENGTH)
            READ_STATIC_STRING(name, GROUP_LENGTH);

    QSqlQuery * q = DBA.getUsersDbQuery();
    q->exec("SELECT nameId FROM sb_groups WHERE active = 1 AND name = '" + name + "'");
    q->next();

    name = "sb_lessons_" + q->value(0).toString();
    q->exec("SELECT lessonId, teacherId FROM " + name);

    WRITE_INT(size(q));
    while(q->next()) {
        WRITE_INT(q->value(0).toInt());
        WRITE_INT(q->value(1).toInt());
    }
}

COMDEF(getParents, 25) {
    READ_INT(id);
    QSqlQuery * q = DBA.getUsersDbQuery();
    q->exec("SELECT u.id, u.name, u.surname FROM sb_users u JOIN sb_users_parents p "
            "ON u.inroleid = p.id WHERE u.role = 4 AND p.studentId = " + QString::number(id));

    WRITE_INT(size(q));
    INIT_STATIC_STRING_WRITE(NAME_LENGTH)
    while(q->next()) {
        WRITE_INT(q->value(0).toInt());
        WRITE_STATIC_STRING(q->value(1).toString(), NAME_LENGTH);
        WRITE_STATIC_STRING(q->value(2).toString(), SURNAME_LENGTH);
    }

    delete q;
}

COMDEF(getGroupMembers, 26) {

    INIT_STRING_READ(GROUP_LENGTH)
        READ_CHAR(mode);
        READ_STATIC_STRING(group, GROUP_LENGTH);

    QSqlQuery * q = DBA.getUsersDbQuery();
    q->exec("SELECT id, nameId FROM sb_groups WHERE name = '" + group + "'");
    q->next();
    QString groupId = q->value(0).toString();
    QString nameId = q->value(1).toString();

    INIT_STATIC_STRING_WRITE(GROUP_LENGTH)
        WRITE_CHAR(mode);
        WRITE_STATIC_STRING(group, GROUP_LENGTH);

    if (mode) {
        q->exec("SELECT u.id, u.name, u.surname, u.role FROM sb_groups g"
                " JOIN sb_users u ON g.leader = u.id WHERE g.id = " + groupId +
                " UNION ALL SELECT u.id, u.name, u.surname, u.role FROM sb_groups_"
                + nameId + " g JOIN sb_users u ON g.memberId = u.id");

        WRITE_INT(size(q));
        while (q->next()) {
            WRITE_INT(q->value(0).toInt());
            WRITE_STATIC_STRING(q->value(1).toString(), NAME_LENGTH);
            WRITE_STATIC_STRING(q->value(2).toString(), SURNAME_LENGTH);
            WRITE_CHAR(q->value(3).toInt());
        }
    } else {
        q->exec("SELECT u.id FROM sb_groups g"
                " JOIN sb_users u ON g.leader = u.id WHERE g.id = " + groupId +
                " UNION ALL SELECT u.id FROM sb_groups_"
                + nameId + " g JOIN sb_users u ON g.memberId = u.id");

        WRITE_INT(size(q));
        while (q->next()) {
            WRITE_INT(q->value(0).toInt());
        }
    }

    delete q;
}

COMDEF(getTimetables, 27) {
    READ_CHAR(number);
    QString n = QString::number(number + 1);

    QSqlQuery * q = DBA.getUsersDbQuery();
    q->exec("SELECT w.id, w.name, t.lessonId0, t.lessonId1, t.lessonId2, t.lessonId3, t.lessonId4, t.lessonId5,"
            " t.lessonId6, t.lessonId7 FROM sb_timetables_weeks w JOIN sb_timetables t ON t.id = w.timetableId0 WHERE w.number = " + n +
            " UNION ALL SELECT w.id, w.name, t.lessonId0, t.lessonId1, t.lessonId2, t.lessonId3, t.lessonId4, t.lessonId5,"
            " t.lessonId6, t.lessonId7 FROM sb_timetables_weeks w JOIN sb_timetables t ON t.id = w.timetableId1 WHERE w.number = " + n +
            " UNION ALL SELECT w.id, w.name, t.lessonId0, t.lessonId1, t.lessonId2, t.lessonId3, t.lessonId4, t.lessonId5,"
            " t.lessonId6, t.lessonId7 FROM sb_timetables_weeks w JOIN sb_timetables t ON t.id = w.timetableId2 WHERE w.number = " + n +
            " UNION ALL SELECT w.id, w.name, t.lessonId0, t.lessonId1, t.lessonId2, t.lessonId3, t.lessonId4, t.lessonId5,"
            " t.lessonId6, t.lessonId7 FROM sb_timetables_weeks w JOIN sb_timetables t ON t.id = w.timetableId3 WHERE w.number = " + n +
            " UNION ALL SELECT w.id, w.name, t.lessonId0, t.lessonId1, t.lessonId2, t.lessonId3, t.lessonId4, t.lessonId5,"
            " t.lessonId6, t.lessonId7 FROM sb_timetables_weeks w JOIN sb_timetables t ON t.id = w.timetableId4 WHERE w.number = " + n +
            " UNION ALL SELECT w.id, w.name, t.lessonId0, t.lessonId1, t.lessonId2, t.lessonId3, t.lessonId4, t.lessonId5,"
            " t.lessonId6, t.lessonId7 FROM sb_timetables_weeks w JOIN sb_timetables t ON t.id = w.timetableId5 WHERE w.number = " + n +
            " ORDER BY w.id");


    INIT_STATIC_STRING_WRITE(GROUP_LENGTH)
            WRITE_CHAR(number);
            WRITE_INT((size(q) / 6));
    int i = 0;
    while (q->next()) {
        if (i++ % 6 == 0) {
            WRITE_STATIC_STRING(q->value(1).toString(), GROUP_LENGTH);
        }

        for (int j = 2; j < 10; j++) {
            WRITE_INT(q->value(j).toInt());
        }
    }
}

COMDEF(editTimetable, 28) {
    Q_UNUSED(answer)

    INIT_STRING_READ(GROUP_LENGTH)
            READ_CHAR(number);
            READ_STATIC_STRING(name, GROUP_LENGTH);

    QSqlQuery * q = DBA.getUsersDbQuery();
    q->exec("SELECT id FROM sb_timetables_weeks WHERE name = '" + name + "'");
    int id;
    if (q->next()) {
        id = q->value(0).toInt();
    } else {
        q->exec("INSERT INTO sb_timetables_weeks (number, name) VALUES (" + QString::number(number) + ", '" + name +  "')");
        id = q->lastInsertId().toInt();
    }

    int dId[6];
    int l[48];
    for (int i = 0; i < 48; i++) {
        READ_INT(value);
        l[i] = value;
    }

    for (int i = 0; i < 6; i++) {
        q->exec("SELECT id FROM sb_timetables WHERE"
                " lessonId0 = " + QString::number(l[i * 8 + 0]) + " and"
                " lessonId1 = " + QString::number(l[i * 8 + 1]) + " and"
                " lessonId2 = " + QString::number(l[i * 8 + 2]) + " and"
                " lessonId3 = " + QString::number(l[i * 8 + 3]) + " and"
                " lessonId4 = " + QString::number(l[i * 8 + 4]) + " and"
                " lessonId5 = " + QString::number(l[i * 8 + 5]) + " and"
                " lessonId6 = " + QString::number(l[i * 8 + 6]) + " and"
                " lessonId7 = " + QString::number(l[i * 8 + 7]));
        qDebug() << q->lastError().text();

        if (q->next()) {
            dId[i] = q->value(0).toInt();
        } else {
            q->exec("INSERT INTO sb_timetables VALUES (NULL, "
                    + QString::number(l[i * 8 + 0]) + ", "
                    + QString::number(l[i * 8 + 1]) + ", "
                    + QString::number(l[i * 8 + 2]) + ", "
                    + QString::number(l[i * 8 + 3]) + ", "
                    + QString::number(l[i * 8 + 4]) + ", "
                    + QString::number(l[i * 8 + 5]) + ", "
                    + QString::number(l[i * 8 + 6]) + ", "
                    + QString::number(l[i * 8 + 7]) +
                    ")");
            qDebug() << q->lastError().text();

            dId[i] = q->lastInsertId().toInt();
        }
    }

    q->exec("UPDATE sb_timetables_weeks SET "
            "timetableId0 = " + QString::number(dId[0]) + ", "
            "timetableId1 = " + QString::number(dId[1]) + ", "
            "timetableId2 = " + QString::number(dId[2]) + ", "
            "timetableId3 = " + QString::number(dId[3]) + ", "
            "timetableId4 = " + QString::number(dId[4]) + ", "
            "timetableId5 = " + QString::number(dId[5]) +
            " WHERE id = " + QString::number(id));
    qDebug() << q->lastError().text();
}

COMDEF(getCalendar, 29) {
    Q_UNUSED(data)
    Q_UNUSED(answer)
}

COMDEF(setTeacherOfLessonForClass, 30) {
    Q_UNUSED(answer)

    INIT_STRING_READ(GROUP_LENGTH)
            READ_STATIC_STRING(group, GROUP_LENGTH);
            READ_INT(lId);
            READ_INT(tId);

    QSqlQuery * q = DBA.getUsersDbQuery();
    q->exec("SELECT nameId FROM sb_groups WHERE name = '" + group + "'");
    q->next();

    QString name = "sb_lessons_" + q->value(0).toString();
    q->exec("UPDATE " + name + " SET teacherId = " + QString::number(tId) + " WHERE lessonId = " + QString::number(lId));

    delete q;
}

COMDEF(generateTimetable, 31) {
    Q_UNUSED(answer)

    auto generator = TimetableGenerator::startSession();

    READ_INT(conditionNumber);

    for (int i = 0; i < conditionNumber; i++) {
        READ_CHAR(type);
        QString subdata;
        if (type % 2 == 0) {
            INIT_STRING_READ(GROUP_LENGTH);
            READ_STATIC_STRING(val, GROUP_LENGTH);
            subdata = val;
        }

        generator->newCondition(type, subdata);

        READ_INT(rows);
        for (int j = 0; j < rows; j++)
            switch (type) {
            case 0: {
                READ_INT(lessonId);
                READ_CHAR(hours);

                generator->addRow(new QInteger(lessonId), new QInteger(hours));
                break;
            }
            case 1: {
                READ_INT(teacherId);
                READ_INT(lessons);
                QVector<int> * lessonsList = new QVector<int>(lessons);
                for (int i = 0; i < lessons; i++) {
                    READ_INT(lessonId);
                    lessonsList->push_back(lessonId);
                }
                READ_CHAR(vacations);
                QVector<char> * vacationDays = new QVector<char>(vacations);
                for (int i = 0; i < vacations; i++) {
                    READ_CHAR(day);
                    vacationDays->push_back(day);
                }

                generator->addRow(new QInteger(teacherId), lessonsList, vacationDays);
                break;
            }
            case 2: {
                READ_INT(teacherId);
                READ_CHAR(preffered);
                QVector<char> * prefferedDays = new QVector<char>(preffered);
                for (int i = 0; i < preffered; i++) {
                    READ_CHAR(day);
                    prefferedDays->push_back(day);
                }
                READ_INT(lessons);
                QVector<int> * lessonsList = new QVector<int>(lessons);
                for (int i = 0; i < lessons; i++) {
                    READ_INT(lessonId);
                    lessonsList->push_back(lessonId);
                }
                READ_CHAR(shifts);

                generator->addRow(new QInteger(teacherId), prefferedDays, lessonsList, new QInteger(shifts));
                break;
            }
            case 3: {
                READ_INT(lessonId);
                READ_CHAR(exceptions);
                QVector<char> * exceptionDays = new QVector<char>(exceptions);
                for (int i = 0; i < exceptions; i++) {
                    READ_CHAR(day);
                    exceptionDays->push_back(day);
                }
                READ_CHAR(lessons);
                QVector<char> * exceptionLessons = new QVector<char>(lessons);
                for (int i = 0; i < lessons; i++) {
                    READ_CHAR(lesson);
                    exceptionLessons->push_back(lessons);
                }

                generator->addRow(new QInteger(lessonId), exceptionDays, exceptionLessons);
                break;
            }
            case 4: {
                READ_INT(lessonId);
                READ_CHAR(groups);
                READ_CHAR(sex);
                READ_INT(teachers);
                QVector<int> * teachersList = new QVector<int>(teachers);
                for (int i = 0; i < teachers; i++) {
                    READ_INT(teacherId);
                    teachersList->push_back(teacherId);
                }

                generator->addRow(new QInteger(lessonId), new QInteger(groups), new QInteger(sex), teachersList);
                break;
            }
            }

        generator->finishCondition();
    }

    generator->build();
}

COMDEF(getTimetable, 32) {
    INIT_STRING_READ(GROUP_LENGTH);
            READ_CHAR(type);
            READ_INT(period);
            READ_STATIC_STRING(group, GROUP_LENGTH);
            READ_INT(id);

    User * u = Server::SERVER->getCurrentUser();
    QSqlQuery * q = DBA.getUsersDbQuery();

    q->exec("SELECT id, nameId, leader FROM sb_groups WHERE name ='" + group + "' AND active = 1");
    q->next();
    int groupId = q->value(0).toInt();
    QString nameId = q->value(1).toString();
    int leaderId = q->value(2).toInt();

    if (u->role == 4) {
        q->exec("SELECT studentId FROM sb_users_parents p JOIN sb_users u ON p.id = u.inroleid WHERE u.id = " + QString::number(u->id));
        q->next();
        id = q->value(0).toInt();
    }

    if (id == 0)
        id = -1;

    QString user = (id + 1)? " AND userId = " + QString::number(id): "";

    q->exec("SELECT lessonId FROM sb_lessons_" + nameId);
    WRITE_INT(size(q));
    while (q->next())
        WRITE_INT(q->value(0).toInt());

    WRITE_CHAR(type);
    WRITE_INT(id);
    switch (type) {
    case 0: {
        // Годовые оценки. Если смотрит администратор, завуч или классный руководитель,
        // то видит всех учеников и их оценки по предметам в общей таблице
        // или оценки конкретного ученика.
        // Если учитель, то только по своему предмету в этом классе или ученика. Если ученик, то видит свои годовые оценки.

        period -= 2022;
        QString year = QString::number(period);

        if (u->id == leaderId || u->role == 0 || u->role == 1 || u->role == 2 || u->role == 3) {
            q->exec("SELECT userId, lessonId, mark, 0 FROM sb_lessons_mark_year WHERE year = " + year + user +
                    " UNION ALL SELECT userId, lessonId, mark, quarter FROM sb_lessons_mark_quarter WHERE"
                    " quarter >= " + QString::number(period * 4 + 1) + " AND quarter <= " + QString::number(period * 4 + 4) + user);
        } else {
            // Читаем к каким предметам относится преподаватель
            q->exec("SELECT lessonId FROM sb_lessons_" + group + " WHERE teacherId = " + QString::number(u->id));
            int count = size(q);
            q->next();
            QString inLesson;
            if (count == 1)
                inLesson = " = " + q->value(0).toString();
            else {
                inLesson = " IN (" + q->value(0).toString();
                q->next();
                inLesson += ", " + q->value(0).toString();
                while (q->next())
                    inLesson += ", " + q->value(0).toString();
                inLesson += ")";
            }
            q->exec("SELECT userId, lessonId, mark, 0 FROM sb_lessons_mark_year WHERE year = " + year + user +
                    " UNION ALL SELECT userId, lessonId, mark, quarter FROM sb_lessons_mark_quarter WHERE"
                    " quarter >= " + QString::number(period * 4 + 1) + " AND quarter <= " + QString::number(period * 4 + 4) + user +
                    " AND lessonId" + inLesson);
        }

        WRITE_INT(size(q));
        while (q->next()) {
            WRITE_INT(q->value(0).toInt());
            WRITE_INT(q->value(1).toInt());
            WRITE_CHAR(q->value(2).toInt());
            WRITE_CHAR(q->value(3).toInt());
        }
        break;
    }
    case 1: {
        // Оценки за четверть. Условия те же самые.
        QString inDay;
        switch (period) {
        case 1: inDay = "day >= 244 AND day <= 302"; break;
        case 2: inDay = "day >= 311 AND day <= 363"; break;
        case 3: inDay = "day >= 16 AND day <= 84"; break;
        case 4: inDay = "day >= 93 AND day <= 152"; break;
        }

        if (u->id == leaderId || u->role == 0 || u->role == 1 || u->role == 2 || u->role == 3) {
            q->exec("SELECT userId, lessonId, mark, day FROM sb_lessons_mark_day WHERE " + inDay + user +
                    " UNION ALL SELECT userId, lessonId, mark, 0 FROM sb_lessons_mark_quarter WHERE quarter = " + QString::number(period) + user);
        } else {
            // Читаем к каким предметам относится преподаватель
            q->exec("SELECT lessonId FROM sb_lessons_" + group + " WHERE teacherId = " + QString::number(u->id));
            int count = size(q);
            q->next();
            QString inLesson;
            if (count == 1)
                inLesson = " = " + q->value(0).toString();
            else {
                inLesson = " IN (" + q->value(0).toString();
                q->next();
                inLesson += ", " + q->value(0).toString();
                while (q->next())
                    inLesson += ", " + q->value(0).toString();
                inLesson += ")";
            }

            q->exec("SELECT userId, lessonId, mark, day FROM sb_lessons_mark_day WHERE " + inDay + user +
                    " UNION ALL SELECT userId, lessonId, mark, 0 FROM sb_lessons_mark_quarter WHERE quarter = " + QString::number(period) +
                    " AND lessonId" + inLesson + user);
        }

        WRITE_INT(size(q));
        while (q->next()) {
            WRITE_INT(q->value(0).toInt());
            WRITE_INT(q->value(1).toInt());
            WRITE_CHAR(q->value(2).toInt());
            WRITE_INT(q->value(3).toInt());
        }
        break;
    }
    case 2: {
        // Расписание и домашние задания или если конкретный ученик, то и оценки.
        q->exec("SELECT * FROM sb_groups_timetables t JOIN sb_timetables_weeks w ON w.id = t.weekTimetableId WHERE t.groupId = "
                + QString::number(groupId));
        q->next();
        int timetables[] = { q->value(7).toInt(), q->value(8).toInt(), q->value(9).toInt(),
                             q->value(10).toInt(), q->value(11).toInt(), q->value(12).toInt() };

        int weekStartDay = period * 7;
        qDebug() << weekStartDay << period;

        for (int i = 0; i < 6; i++) {
            q->exec("SELECT * FROM sb_timetables WHERE id = " + QString::number(timetables[i]));
            q->next();
            for (int j = 1; j < 9; j++)
                WRITE_INT(q->value(j).toInt());

            for (int j = 0; j < 8; j++) {
                q->exec("SELECT text FROM sb_tasks WHERE groupId = " + QString::number(groupId) +
                        " AND expireDay = " + QString::number(weekStartDay + i) + " AND lessonNumber = " + QString::number(j));

                if (q->next()) {
                    WRITE_DYNAMIC_STRING(q->value(0).toString(), short);
                }
                else
                    WRITE_SHORT(0);
            }
        }

        QString inDay = "day >= " + QString::number(weekStartDay) + " AND day <= " + QString::number(weekStartDay + 6);
        q->exec("SELECT userId, lessonId, mark, day FROM sb_lessons_mark_day WHERE " + inDay + user);

        WRITE_INT(size(q));
        while (q->next()) {
            WRITE_INT(q->value(0).toInt());
            WRITE_INT(q->value(1).toInt());
            WRITE_CHAR(q->value(2).toInt());
            WRITE_INT(q->value(3).toInt());
        }

        break;
    }}

    delete q;
}

COMDEF(setMark, 33) {
    Q_UNUSED(answer)

    READ_CHAR(type);
    READ_INT(period);
    READ_INT(id);
    READ_INT(lessonId);
    READ_CHAR(mark);

    if (id == -1)
        return;

    QSqlQuery * q = DBA.getUsersDbQuery();

    switch (type) {
    case 0: {
        READ_CHAR(column);
        period -= 2022;
        if (column == 5) {
            q->exec("UPDATE sb_lessons_mark_year SET mark = " + QString::number(mark) + " WHERE userId = " + QString::number(id) +
                    " AND lessonId = " + QString::number(lessonId) + " AND year = " + QString::number(period));
            if (!q->numRowsAffected())
                q->exec(QString("INSERT INTO sb_lessons_mark_year VALUES (NULL, " + QString::number(id) + ", " +
                            QString::number(lessonId) + ", " + QString::number(mark) + ", " + QString::number(period) + ")"));
        }
        else {
            q->exec("UPDATE sb_lessons_mark_quarter SET mark = " + QString::number(mark) + " WHERE userId = " + QString::number(id) +
                    " AND lessonId = " + QString::number(lessonId) + " AND quarter = " + QString::number(period * 4 + column));
            if (!q->numRowsAffected())
                q->exec(QString("INSERT INTO sb_lessons_mark_quarter VALUES (NULL, " + QString::number(id) +
                            ", " + QString::number(lessonId) + ", " + QString::number(mark) + ", " + QString::number(period * 4 + column) + ")"));
        }

        break;
    }
    case 1: {
        READ_INT(day);
        if (day != 0) {
            q->exec("UPDATE sb_lessons_mark_day SET mark = " + QString::number(mark) + " WHERE userId = " + QString::number(id) +
                    " AND lessonId = " + QString::number(lessonId) + " AND day = " + QString::number(day));
            qDebug() << q->lastError();
            if (!q->numRowsAffected())
                q->exec(QString("INSERT INTO sb_lessons_mark_day VALUES (NULL, " + QString::number(id) + ", " +
                            QString::number(lessonId) + ", " + QString::number(day) + ", " + QString::number(mark) + ", 1)"));
        }
        else {
            q->exec("UPDATE sb_lessons_mark_quarter SET mark = " + QString::number(mark) + " WHERE userId = " + QString::number(id) +
                    " AND lessonId = " + QString::number(lessonId) + " AND quarter = " + QString::number(period));
            if (!q->numRowsAffected())
                q->exec(QString("INSERT INTO sb_lessons_mark_quarter VALUES (NULL, " + QString::number(id) +
                            ", " + QString::number(lessonId) + ", " + QString::number(mark) + ", " + QString::number(period) + ")"));
        }

        break;
    }
    case 2: {
        READ_INT(day);
        q->exec("UPDATE sb_lessons_mark_day SET mark = " + QString::number(mark) + " WHERE userId = " + QString::number(id) +
                " AND lessonId = " + QString::number(lessonId) + " AND day = " + QString::number(day));
        if (!q->numRowsAffected())
            q->exec(QString("INSERT INTO sb_lessons_mark_day VALUES (NULL, " + QString::number(id) + ", " +
                        QString::number(lessonId) + ", " + QString::number(day) + ", " + QString::number(mark) + ", 1)"));
        break;
    }}
}

COMDEF(manageTask, 34) {
    Q_UNUSED(answer)

    INIT_STRING_READ(1024)
            READ_STATIC_STRING(group, GROUP_LENGTH);
            READ_DYNAMIC_STRING(text, short);
            READ_INT(expireDay);
            READ_INT(lessonNumber);

    QSqlQuery * q = DBA.getUsersDbQuery();
    q->exec("SELECT id, nameId FROM sb_groups WHERE active = 1 AND name = '" + group + "'");
    q->next();
    QString id = q->value(0).toString();
    QString nameId = q->value(1).toString();

    q->exec("UPDATE sb_tasks SET text = '" + text + "' WHERE groupId = " + id +
            " AND expireDay = " + QString::number(expireDay) + " AND lessonNumber = " + QString::number(lessonNumber));
    if (!q->numRowsAffected()) {
        q->exec("INSERT INTO sb_tasks VALUES (NULL, " + id + ", " + QString::number(Server::SERVER->getCurrentUser()->id) + ", '" + text
                + "', " + QString::number(expireDay) + ", " + QString::number(lessonNumber) + ")");
        QString taskId = q->lastInsertId().toString();
        q->exec("SELECT memberId FROM sb_groups_" + nameId);
        QSqlQuery * d = DBA.getUsersDbQuery();
        while (q->next())
            d->exec("INSERT INTO sb_tasks_u" + q->value(0).toString() + " VALUES (NULL, " + taskId + ", 0)");
    }
}

COMDEF(setTimetable, 35) {
    Q_UNUSED(answer)

    INIT_STRING_READ(GROUP_LENGTH)
            READ_STATIC_STRING(group, GROUP_LENGTH);
            READ_STATIC_STRING(timetable, GROUP_LENGTH);

    QMap<int, int> teachersForLessons;
    QSqlQuery * q = DBA.getUsersDbQuery();

    q->exec("SELECT nameId FROM sb_groups WHERE name = '" + group + "'");
    q->next();
    QString nameId = q->value(0).toString();
    qDebug() << q->lastQuery();
    qDebug() << q->lastError();

    q->exec("SELECT id FROM sb_timetables_weeks WHERE name = '" + timetable + "'");
    q->next();
    QString timetableId = q->value(0).toString();

    q->exec("SELECT lessonId, teacherId FROM sb_lessons_" + nameId);
    while (q->next())
        teachersForLessons.insert(q->value(0).toInt(), q->value(1).toInt());

    q->exec("DELETE FROM sb_lessons_" + nameId);

    QSet<int> lessons;
    QSqlQuery * d = DBA.getUsersDbQuery();
    q->exec("SELECT * FROM sb_timetables_weeks WHERE id = " + timetableId);
    q->next();
    for (int i = 3; i < 9; i++) {
        d->exec("SELECT * FROM sb_timetables WHERE id = " + q->value(i).toString());
        d->next();
        for (int j = 1; j < 9; j++)
            lessons.insert(d->value(j).toInt());
    }
    delete d;

    qDebug() << lessons;

    QString query = "INSERT INTO sb_lessons_" + nameId + " VALUES ";
    int left = lessons.size();
    for (int lId : lessons)
        if (lId == 0)
            left--;
        else if (--left)
            query += "(NULL, " + QString::number(lId) + ", " + (teachersForLessons.contains(lId)?
                                                                    QString::number(teachersForLessons[lId]): "NULL") + "), ";
        else
            query += "(NULL, " + QString::number(lId) + ", " + (teachersForLessons.contains(lId)?
                                                                    QString::number(teachersForLessons[lId]): "NULL") + ")";

    q->exec(query);
}

// Создаём массив обработчиков
COMMANDS_LIST_DEF

void Receiver::receive(QDataStream & data, QDataStream & answer)
{
    // Реализуем протокол:
    // Первый байт - число комманд, затем идут комманды плотной упаковкой
    //
    // Первый байт - номер комманды, затем данные.
    // Все строки имеют уже определенный размер (в связи с конфигурацией таблиц),
    // поэтому никаких дополнительных данных не нужно
    //
    // В процессе обработки мы будет собирать ответ по такому же шаблону
    //

    char commandsLeft;
    data >> commandsLeft;
    qDebug() << "Commands" << (int) commandsLeft;
    answer << commandsLeft;

    while (commandsLeft--) {

        char command;
        data >> command;
        qDebug() << "Parse command " << (int) command;

        if (command < COMMANDS_NUM) {
            answer << command;
            commands[(int) command](data, answer);
        }
        else {
            //  Неизвестная комманда
            // TODO
        }
    }
}
