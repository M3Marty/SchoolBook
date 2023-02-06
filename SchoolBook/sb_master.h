#ifndef SB_MASTER_H
#define SB_MASTER_H

#define NAME_LENGTH 16
#define SURNAME_LENGTH 16
#define CODE_LENGTH 16
#define LOGIN_LENGTH 16
#define GROUP_LENGTH 20
#define PASS_LENGTH 32
#define NAME_SURNAME_LENGTH 33
#define TITLE_LENGTH 64

#define COMMANDS_LIST {\
c_register, c_login,\
c_getLastId, c_newPost, c_loadNewFile, c_getPosts, c_getFile, c_editPost,\
c_getMyGroups, c_getProfileData, c_getChats, c_getTasks, c_getMessages,\
c_getCode, c_createClass, c_occuipedLiters, c_getMyTeachers, c_getMyStudents,\
c_transferStudent, c_setMasterTeacher, c_editLesson, c_getUserPosts, c_updateTaskStatus,\
c_getLessonsList, c_teachersOfLessonsForClass, c_getParents, c_getGroupMembers,\
c_getTimetables, c_editTimetable, c_getCalendar, c_setTeacherOfLessonForClass,\
c_generateTimetable, c_getTimetable, c_setMark, c_manageTask, c_setTimetable\
};

#define ADMIN_ROLE 0
#define MASTER_ROLE 1
#define TEACHER_ROLE 2
#define STUDENT_ROLE 3
#define PARENT_ROLE 4

#ifdef CID_EXTERN
#define COMDEF(name, id) \
    extern const char cid_##name
#else
#ifdef CID_DEFINE
#define COMDEF(name, id) \
    extern const char cid_##name; \
    const char cid_##name = id
#else
#define COMDEF(name, id)
#endif
#endif

COMDEF(register, 0);
COMDEF(login, 1);
COMDEF(getLastId, 2);
COMDEF(newPost, 3);
COMDEF(loadNewFile, 4);
COMDEF(getPosts, 5);
COMDEF(getFile, 6);
COMDEF(editPost, 7);
COMDEF(getMyGroups, 8);
COMDEF(getProfileData, 9);
COMDEF(getChats, 10);
COMDEF(getTasks, 11);
COMDEF(getMessages, 12);
COMDEF(getCode, 13);
COMDEF(createClass, 14);
COMDEF(occuipedLiters, 15);
COMDEF(getMyTeachers, 16);
COMDEF(getMyStudents, 17);
COMDEF(transferStudent, 18);
COMDEF(setMasterTeacher, 19);
COMDEF(editLesson, 20);
COMDEF(getUserPosts, 21);
COMDEF(updateTaskStatus, 22);
COMDEF(getLessonsList, 23);
COMDEF(teachersOfLessonsForClass, 24);
COMDEF(getParents, 25);
COMDEF(getGroupMembers, 26);
COMDEF(getTimetables, 27);
COMDEF(editTimetable, 28);
COMDEF(getCalendar, 29);
COMDEF(setTeacherOfLessonForClass, 30);
COMDEF(generateTimetable, 31);
COMDEF(getTimetable, 32);
COMDEF(setMark, 33);
COMDEF(manageTask, 34);
COMDEF(setTimetable, 35);

#undef COMDEF

#define COMMANDS_NUM 36

#define READ_VAR(var_name) data >> var_name

#define READ_LONG(var_name)\
    long long var_name;\
    READ_VAR(var_name)

#define READ_INT(var_name) \
    int var_name;\
    READ_VAR(var_name)

#define READ_SHORT(var_name) \
    short var_name;\
    READ_VAR(var_name)

#define READ_CHAR(var_name) \
    char var_name;\
    READ_VAR(var_name)

#define INIT_STRING_READ(maxLength) \
    char __buffer[maxLength + 1];

#define READ_STATIC_STRING(var_name, __length) \
    data.readRawData(__buffer, __length); \
    __buffer[__length] = 0; \
    QString var_name = QString::fromLocal8Bit(__buffer)

#define READ_DYNAMIC_STRING(var_name, lengthType) \
    lengthType var_name##Length; \
    READ_VAR(var_name##Length); \
    data.readRawData(__buffer, var_name##Length); \
    __buffer[var_name##Length] = 0;\
    QString var_name = QString::fromLocal8Bit(__buffer)

#define READ_DYNAMIC_STRING_NO_LIMIT(var_name, lengthType) \
    lengthType var_name##Length; \
    READ_VAR(var_name##Length); \
    char * var_name##Raw = (char *) malloc(var_name##Length + 1);\
    data.readRawData(var_name##Raw, var_name##Length); \
    var_name##Raw[(int) var_name##Length] = 0;\
    QString var_name = QString::fromLocal8Bit(var_name##Raw); \
    free(var_name##Raw)

#ifdef REQUEST_WRITE
#define WRITE_DEST request
#else
#ifdef ANSWER_WRITE
#define WRITE_DEST answer
#endif
#endif

#ifdef WRITE_DEST
#define WRITE_LONG(var_name)\
    WRITE_DEST << (long long) var_name

#define WRITE_INT(var_name)\
    WRITE_DEST << (int) var_name

#define WRITE_SHORT(var_name)\
    WRITE_DEST << (short) var_name

#define WRITE_CHAR(var_name)\
    WRITE_DEST << (char) var_name

#define INIT_STATIC_STRING_WRITE(maxLength)\
    char __filler[maxLength + 1] = { 0 };

#define WRITE_STATIC_STRING(var_name, __length) \
    WRITE_DEST.writeRawData(var_name.toLocal8Bit().data(), var_name.length());\
    WRITE_DEST.writeRawData(__filler, __length - var_name.length())

#define WRITE_DYNAMIC_STRING(var_name, lengthType)\
    WRITE_DEST << (lengthType) var_name.length();\
    WRITE_DEST.writeRawData(var_name.toLocal8Bit().data(), var_name.length())
#endif

#ifdef GENERAL_INCLUDE
#define REQUEST \
QByteArray requestData; \
QDataStream request(&requestData, QIODevice::WriteOnly); \
request.setVersion(QDataStream::Version::Qt_6_4);
#endif

#ifdef SERVER_RECEIVER

#include <QCryptographicHash>
#include <QDateTime>
#include <QRandomGenerator>
#include <QFile>

#include <QSqlError>

#include <cstdlib>

int size(QSqlQuery * query)
{
    int initialPos = query->at();

    int pos = 0;
    if (query->last())
        pos = query->at() + 1;
    else
        pos = 0;

    query->seek(initialPos);
    return pos;
}

qint64 hash(const QString & str)
{
    QByteArray hash = QCryptographicHash::hash(
        QByteArray::fromRawData((const char*)str.utf16(), str.length()*2),
        QCryptographicHash::Md5
    );
    Q_ASSERT(hash.size() == 16);
    QDataStream stream(&hash, QIODevice::ReadOnly);
    qint64 a, b;
    stream >> a >> b;
    return a ^ b;
}

#define COMDEF(name, id) \
    void c_##name(QDataStream & data, QDataStream & answer)

#define COMDEF_UNUSED(name, id)\
    void c_##name(QDataStream & data, QDataStream & answer) { Q_UNUSED(data) QUNUSED(answer) }

QString getCurrentYear() {
    auto date = QDateTime::currentDateTime().date();
    int year = date.year();
    int month = date.month();
    if (month < 7)
        year--;

    return QString::number(year);
}

#define COMMANDS_LIST_DEF void(*commands[])(QDataStream &, QDataStream &) = COMMANDS_LIST

#endif

#ifdef CLIENT_RECEIVER

#include <QDataStream>

#define COMDEF(name, id) \
    void c_##name(QDataStream & data)

#define COMDEF_UNUSED(name, id)\
    void c_##name(QDataStream & data) { Q_UNUSED(data) }

#define COMMANDS_LIST_DEF void(*commands[])(QDataStream &) = COMMANDS_LIST

#define c_register c_stub
#define c_login c_stub

#endif

#endif // SB_MASTER_H
