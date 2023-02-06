#ifndef DBACCESSOR_H
#define DBACCESSOR_H

#include <QSqlDatabase>
#include <QSqlQuery>

class DBAccessor
{
public:
    DBAccessor();

    QSqlQuery * getUsersDbQuery();

private:
    QSqlDatabase usersDb;
};

extern DBAccessor DBA;

#endif // DBACCESSOR_H
