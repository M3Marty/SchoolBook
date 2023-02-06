#include "timetable.h"

Timetable::Timetable(QStringList lessons, int number)
{
    this->lessons = lessons;
    this->number = number;
}

QStringList Timetable::getDay(int day) {
    return lessons.sliced(8 * day, 8);
}
