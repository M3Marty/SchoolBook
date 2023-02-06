#ifndef TIMETABLE_H
#define TIMETABLE_H

#include <QStringList>

class Timetable
{
public:
    Timetable(QStringList lessons, int number);

    QStringList getDay(int day);

    int number;
    QStringList lessons;
};

#endif // TIMETABLE_H
