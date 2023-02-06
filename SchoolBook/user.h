#ifndef USER_H
#define USER_H

#include <QString>

class User
{
public:
    User(int id, int role, QString name, QString surname, QString className="");

    int id;
    int role;
    QString name;
    QString surname;
    QString className;

    bool operator==(const User u);

    QString toString();
};

#endif // USER_H
