#include "receiver.h"

#include <mainwindow.h>

#define CLIENT_RECEIVER
#define GENERAL_INCLUDE
#define REQUEST_WRITE
#define CID_DEFINE
#include <sb_master.h>

Receiver::Receiver() { }

COMDEF_UNUSED(stub, -1)

COMDEF(getLastId, 2) {
    READ_INT(lastId);
    READ_CHAR(submode);

    if (submode == 1) {
        MainWindow::MAIN->lastPostId = lastId;
        // Мы получили id поста и нужно его реализовать
        REQUEST

        WRITE_CHAR(1);
        WRITE_CHAR(cid_getPosts);
        WRITE_INT(lastId);
        WRITE_CHAR(5);

        MainWindow::MAIN->send(requestData);
    } else if (submode == 2) {
        // Реализовываем id поста профиля
        REQUEST

        WRITE_CHAR(1);
        WRITE_CHAR(cid_getUserPosts);
        WRITE_INT(lastId);
        WRITE_CHAR(5);

        MainWindow::MAIN->send(requestData);
    }
}

COMDEF_UNUSED(newPost, 3)

COMDEF_UNUSED(loadNewFile, 4)

COMDEF(getPosts, 5) {
    READ_CHAR(isAnotherPostAvailable);

    INIT_STRING_READ(TITLE_LENGTH)
    while (isAnotherPostAvailable) {
        READ_INT(id);
        READ_STATIC_STRING(name, TITLE_LENGTH);
        READ_INT(authorId);
        READ_STATIC_STRING(authorName, NAME_SURNAME_LENGTH);
        READ_DYNAMIC_STRING_NO_LIMIT(text, short);
        READ_LONG(timeStamp);

        READ_VAR(isAnotherPostAvailable);
        if (!isAnotherPostAvailable) {
            MainWindow::MAIN->lastPostId = id - 1;
        }

        MainWindow::MAIN->addPost(Post(name, text, timeStamp, authorName, authorId, id));
    }
}

COMDEF(getFile, 6) {
    // TOOD
}

COMDEF(editPost, 7) {
    // TODO
}

COMDEF(getMyGroups, 8) {

    READ_INT(leadGroups);
    INIT_STRING_READ(GROUP_LENGTH)
    for (int i = 0; i < leadGroups; i++) {
        READ_STATIC_STRING(name, GROUP_LENGTH);
        MainWindow::MAIN->addGroupToLead(name);
    }

    READ_INT(memberGroups);
    for (int i = 0; i < memberGroups; i++) {
        READ_STATIC_STRING(name, GROUP_LENGTH);
        MainWindow::MAIN->addGroupToMember(name);
    }


    REQUEST

    WRITE_CHAR((leadGroups + memberGroups + 1));
    INIT_STATIC_STRING_WRITE(GROUP_LENGTH);
    while (leadGroups--) {
        WRITE_CHAR(cid_getGroupMembers);
        WRITE_CHAR(0); // Только id
        WRITE_STATIC_STRING(MainWindow::MAIN->leadGroups[leadGroups], GROUP_LENGTH);
    }

    while (memberGroups--) {
        WRITE_CHAR(cid_getGroupMembers);
        WRITE_CHAR(1); // Полная информация
        WRITE_STATIC_STRING(MainWindow::MAIN->memberGroups[memberGroups], GROUP_LENGTH);
    }

    WRITE_CHAR(cid_getLastId);
    WRITE_CHAR(0);
    WRITE_CHAR(1);

    MainWindow::MAIN->send(requestData);
}

COMDEF(getProfileData, 9) {
    READ_INT(id);

    INIT_STRING_READ(NAME_SURNAME_LENGTH);
            READ_STATIC_STRING(name, NAME_LENGTH);
            READ_STATIC_STRING(surname, SURNAME_LENGTH);
            READ_CHAR(role);
            READ_STATIC_STRING(groupName, NAME_SURNAME_LENGTH);

    MainWindow::MAIN->addKnownUser(User(id, role, name, surname, groupName));
    MainWindow::MAIN->setProfileViewData(name, surname, role, groupName);
}

COMDEF(getChats, 10) {
    READ_CHAR(chats);

    REQUEST // TODO

    char command = cid_getLastId; // Получить сообщения

    for (int i = 0; i < chats; i++) {
        READ_INT(id);
        READ_DYNAMIC_STRING_NO_LIMIT(name, char);
        MainWindow::MAIN->addChat(id, name);
    }
}

COMDEF(getTasks, 11) {
    READ_INT(number);

    INIT_STRING_READ(NAME_SURNAME_LENGTH)
    for (int i = 0; i < number; i++) {

        READ_INT(id);
        READ_INT(authorId);
        READ_STATIC_STRING(aName, NAME_SURNAME_LENGTH);
        READ_DYNAMIC_STRING_NO_LIMIT(text, short);
        READ_CHAR(status);

        MainWindow::MAIN->addTask(id, authorId, aName, text, status);
    }
}

COMDEF(getMessages, 12) {
    // TODO
}

COMDEF(getCode, 13) {
    // Не буду трогать

    char code[17];
    code[16] = 0;
    data.readRawData(code, 16);
    qDebug() << code;

    READ_INT(id);

    INIT_STRING_READ(NAME_SURNAME_LENGTH);
            READ_STATIC_STRING(name, NAME_LENGTH);
            READ_STATIC_STRING(surname, SURNAME_LENGTH);
            READ_CHAR(role);
            READ_STATIC_STRING(groupName, NAME_SURNAME_LENGTH);

    MainWindow::MAIN->setGetCode(QString(code));

    switch (role) {
    case 0:
        MainWindow::MAIN->addKnownUser(id, role, name, surname);
        break;
    case 1:
    case 2:
        MainWindow::MAIN->addOwnedTeacher(id, role, name, surname, groupName);
        break;
    case 3:
        MainWindow::MAIN->addOwnedStudent(id, role, name, surname, groupName);
    }
}

COMDEF_UNUSED(createClass, 14)

COMDEF(occuipedLiters, 15) {
    char * numbers = new char[11];
    data.readRawData(numbers, 11);

    MainWindow::MAIN->setOccuipedLiters(numbers);
}

COMDEF(getMyTeachers, 16) {
    READ_INT(number);

    INIT_STRING_READ(NAME_SURNAME_LENGTH)
    for (int i = 0; i < number; i++) {

        READ_INT(id);
        READ_STATIC_STRING(name, NAME_LENGTH);
        READ_STATIC_STRING(surname, SURNAME_LENGTH);
        READ_CHAR(role);
        READ_STATIC_STRING(groupName, NAME_SURNAME_LENGTH);

        MainWindow::MAIN->addOwnedTeacher(id, role, name, surname, groupName);
    }
}

COMDEF(getMyStudents, 17) {
    READ_INT(number);

    INIT_STRING_READ(NAME_SURNAME_LENGTH)
    for (int i = 0; i < number; i++) {

        READ_INT(id);
        READ_STATIC_STRING(name, NAME_LENGTH);
        READ_STATIC_STRING(surname, SURNAME_LENGTH);
        READ_CHAR(role);
        READ_STATIC_STRING(groupName, NAME_SURNAME_LENGTH);

        MainWindow::MAIN->addOwnedStudent(id, role, name, surname, groupName);
    }
}

COMDEF_UNUSED(transferStudent, 18)

COMDEF_UNUSED(setMasterTeacher, 19)

COMDEF(editLesson, 20) {
    READ_CHAR(answer);
    if (answer)
        qDebug() << "Edit lesson error";
}

COMDEF(getUserPosts, 21) {

}

COMDEF_UNUSED(updateTaskStatus, 22)

COMDEF(getLessonsList, 23) {
    READ_INT(number);
    INIT_STRING_READ(NAME_LENGTH)
    for (int i = 0; i < number; i++) {
        READ_STATIC_STRING(name, NAME_LENGTH);
        MainWindow::MAIN->addLesson(name);
    }
}

COMDEF(teachersOfLessonsForClass, 24) {
    QStringList l;
    READ_INT(number);
    for (int i = 0; i < number; i++) {
        READ_INT(lId);
        READ_INT(tId);
        User * u = MainWindow::MAIN->getUser(tId);
        QString lesson = MainWindow::MAIN->lessons[lId];

        l.append(lesson + " ("+ u->toString() + ")");
    }

    MainWindow::MAIN->setTeacherOfLessonsForClass(l);
}

COMDEF(getParents, 25) {
    READ_INT(number);

    QVector<QPair<int, QString>> l;
    INIT_STRING_READ(NAME_LENGTH)
    while (number--) {
        READ_INT(id);
        READ_STATIC_STRING(name, NAME_LENGTH);
        READ_STATIC_STRING(surname, SURNAME_LENGTH);

        l.append(QPair<int, QString>(id, name + " " + surname));
    }

    MainWindow::MAIN->setParents(l);
}

COMDEF(getGroupMembers, 26) {
    INIT_STRING_READ(GROUP_LENGTH)
        READ_CHAR(mode);
        READ_STATIC_STRING(group, GROUP_LENGTH);
        READ_INT(number);

    QVector<int> members;

    if (mode) {
        while (number--) {
            READ_INT(id);
            READ_STATIC_STRING(name, NAME_LENGTH);
            READ_STATIC_STRING(surname, SURNAME_LENGTH);
            READ_CHAR(role);
            MainWindow::MAIN->addKnownUser(User(id, role, name, surname, group));

            members.push_back(id);
        }
    } else {
        while (number--) {
            READ_INT(id);
            members.push_back(id);
        }
    }

    MainWindow::MAIN->addGroupToDisplay(group, members);
}

COMDEF(getTimetables, 27) {
    READ_CHAR(classNumber);
    READ_INT(number);

    INIT_STRING_READ(GROUP_LENGTH)
    while (number--) {
        QStringList lessons;

        READ_STATIC_STRING(name, GROUP_LENGTH);
        for (int i = 0; i < 48; i++) {
            READ_INT(lessonId);
            lessons.append(MainWindow::MAIN->lessons[lessonId]);
        }

        MainWindow::MAIN->timetables[(int) classNumber].insert(name, new Timetable(lessons, classNumber));
    }

    MainWindow::MAIN->on_editTimetableNumber_currentIndexChanged(classNumber);
}

COMDEF_UNUSED(editTimetable, 28)

COMDEF(getCalendar, 29) {

}

COMDEF_UNUSED(setTeacherOfLessonForClass, 30)

COMDEF_UNUSED(generateTimetable, 31)

COMDEF(getTimetable, 32) {
    READ_INT(lessonsCount);
    QVector<int> lessons;
    for (int i = 0; i < lessonsCount; i++) {
        READ_INT(value);
        lessons.push_back(value);
    }

    READ_CHAR(type);
    READ_INT(id);

    switch (type) {
    case 0: {
        MainWindow::MAIN->clearYearTimetable(lessons);
        if (id == -1)
            return;

        READ_INT(size);
        for (int i = 0; i < size; i++) {
            READ_INT(userId);
            READ_INT(lessonId);
            READ_CHAR(mark);
            READ_CHAR(quarter);

            if (userId == id)
                MainWindow::MAIN->addYearTimetableValue(userId, lessonId, mark, quarter? quarter: 5);
        }
        break;
    }
    case 1: {
        MainWindow::MAIN->clearQuarterTimetable(lessons);
        if (id == -1)
            return;

        READ_INT(size);
        for (int i = 0; i < size; i++) {
            READ_INT(userId);
            READ_INT(lessonId);
            READ_CHAR(mark);
            READ_INT(dayOrQuarter);

            if (userId == id)
                MainWindow::MAIN->addQuarterTimetableValue(userId, lessonId, mark, dayOrQuarter);
        }
        break;
    }
    case 2: {
        MainWindow::MAIN->clearWeekTimetable();

        QVector<int> lessons;
        QVector<QString> tasks;
        INIT_STRING_READ(1024);
        for (int i = 0; i < 6; i++) {
            lessons.clear();
            tasks.clear();
            for (int j = 0; j < 8; j++) {
                READ_INT(lessonId);
                lessons.push_back(lessonId);
            }
            for (int j = 0; j < 8; j++) {
                READ_DYNAMIC_STRING(text, short);
                tasks.push_back(text);
            }

            MainWindow::MAIN->addWeekTimetableValue(lessons, tasks, i);
        }
        READ_INT(size);
        for (int i = 0; i < size; i++) {
            READ_INT(userId);
            READ_INT(lessonId);
            READ_CHAR(mark);
            READ_INT(day);

            if (userId == id)
                MainWindow::MAIN->addWeekMark(userId, lessonId, mark, day % 7);
        }
        break;
    }
    }
}

COMDEF_UNUSED(setMark, 33)

COMDEF_UNUSED(manageTask, 34)

COMDEF_UNUSED(setTimetable, 35)

// Создаём массив обработчиков
COMMANDS_LIST_DEF

void Receiver::receive(QDataStream & data)
{
    // Реализуем протокол:
    // Первый байт - число комманд, затем идут комманды плотной упаковкой
    //
    // Первый байт - номер комманды, затем данные
    // Все строки имеют уже определенный размер (в связи с конфигурацией таблиц),
    // поэтому никаких дополнительных данных не нужно
    //

    char commandsLeft;
    data >> commandsLeft;
    qDebug() << (int) commandsLeft << data.atEnd();

    while (commandsLeft--) {

        char command;
        data >> command;
        qDebug() << "Parse command " << (int) command;

        if (command < COMMANDS_NUM)
            commands[(int) command](data);
        else {
            //  Неизвестная комманда
            // TODO
        }
    }
}

