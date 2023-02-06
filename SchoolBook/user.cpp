#include "user.h"

User::User(int id, int role, QString name, QString surname, QString className)
{
    this->id = id;
    this->role = role;
    this->name = name;
    this->surname = surname;
    this->className = className;
}

bool User::operator==(const User u) {
    return id == u.id;
}

QString User::toString() {
    return this->name + " " + this->surname;
}
