#include "user.h"
#include <dbaccessor.h>

User::User(int id)
{
    QSqlQuery * q = DBA.getUsersDbQuery();
    q->exec("SELECT name, surname, role, inroleid "
            "from sb_users WHERE id = " + QString::number(id));

    q->next();

    this->id = id;
    this->name = q->value(0).toString();
    this->surname = q->value(1).toString();
    this->role = q->value(2).toInt();
    this->inroleid = q->value(3).toInt();

    delete q;
}
