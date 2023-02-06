#ifndef USER_H
#define USER_H

#include <QSqlQuery>

class User
{
public:
    User(int id);

    int id;

    QString name;
    QString surname;

    int role;
    int inroleid;
};

#endif // USER_H
