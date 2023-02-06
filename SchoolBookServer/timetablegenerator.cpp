#include "timetablegenerator.h"

#include <QIODevice>
#include <server.h>
#include <dbaccessor.h>

#include <stdlib.h>

#define ANSWER_WRITE
#include <../SchoolBook/sb_master.h>

QInteger::QInteger(int i)
{
    this->i = i;
}

int QInteger::value()
{
    return this->i;
}


TimetableGenerator * TimetableGenerator::CURRENT_SESSION = nullptr;

TimetableGenerator::TimetableGenerator() { }

void TimetableGenerator::formTeachers()
{
    for (auto t : teachers->teachers())
        teachersList.insert(t, new Teacher{ t, teachers->lessonsOfTeacher(t),
                teachers->daysOfTeacher(t), IdMap<TimetableGenerator::Teacher::Preference *>() });

    for (auto cId : windows->classes())
        for (auto tId : windows->teachers(cId))
            teachersList[tId]->preferences.insert(cId, new TimetableGenerator::Teacher::Preference{windows->getTeacherDays(cId, tId),
                    windows->getTeacherLessons(cId, tId), windows->getTeacherShifts(cId, tId)});
}

void TimetableGenerator::formLessons()
{
    for (auto cId : hours->classes())
        classesList.insert(cId, new Class{ cId, hours->hoursPerLessonForClass(cId),
                    new IdMap<GroupsCondition::Group *>()});
}

void TimetableGenerator::includeRestOfClasses()
{
    IdMap<QString> classes;
    QSqlQuery * q = DBA.getUsersDbQuery();
    q->exec("SELECT id, nameId FROM sb_groups WHERE nameId LIKE 'c%' AND active = 1");
    while (q->next()) {
        int cId = q->value(0).toInt();
        if (!classesList.contains(cId))
            classes.insert(cId, q->value(1).toString());
    }

    IdMap<IntMap> cash;

    IntSet toDelete;
    for (int cId : classes.keys()) {
        q->exec("SELECT t.shift, t.weekTimetableId FROM sb_groups_timetables t WHERE t.groupId = " + QString::number(cId) +
                " UNION ALL SELECT 0, 0"
                " UNION ALL SELECT l.lessonId, l.teacherId FROM sb_lessons_" + classes[cId] +
                " l UNION ALL SELECT 0, 0");

        QVector<IntPair> data;

        while (q->next())
            data.push_back(IntPair(q->value(0).toInt(), q->value(1).toInt()));

        if (!(data[0].first || data[0].second)) {
            toDelete.insert(cId);
            continue;
        }

        q->exec("SELECT lessonId, teacher1Id, teacher2Id, group1Id, group2Id FROM sb_groups_groups WHERE id " + QString::number(cId));

        while (q->next()) {
            data.push_back(IntPair(q->value(0).toInt(), 0));
            data.push_back(IntPair(q->value(1).toInt(), q->value(2).toInt()));
            data.push_back(IntPair(q->value(3).toInt(), q->value(4).toInt()));
        }


        IntMap * hours = new IntMap();
        IntMap teachers;

        int i = 2;
        if (cash.contains(data[0].second))
            for (;; i++)
                if (data[i].first == 0)
                    break;
                else {
                    hours->insert(data[i].first, cash[data[0].second].value(data[i].first));
                    teachers.insert(data[i].first, data[i].second);
                }
        else {
            cash.insert(data[i].second, IntMap());

            for (;; i++)
                if (data[i].first == 0)
                    break;
                else {
                    q->exec(QString("SELECT SUM(l0 = %1) + SUM(l1 = %1) + SUM(l2 = %1) + SUM(l3 = %1) + SUM(l4 = %1) + SUM(l5 = %1) + SUM(l6 = %1) + SUM(l7 = %1) AS Hours FROM ("
                            " SELECT lessonId0 AS l0, lessonId1 AS l1, lessonId2 AS l2, lessonId3 AS l3, lessonId4 AS l4, lessonId5 AS l5, lessonId6 AS l6, lessonId7 AS l7 FROM sb_timetables"
                            " WHERE id = (SELECT timetableId0 FROM sb_timetables_weeks WHERE id = %2) UNION ALL"
                            " SELECT lessonId0, lessonId1, lessonId2, lessonId3, lessonId4, lessonId5, lessonId6, lessonId7 FROM sb_timetables"
                            " WHERE id = (SELECT timetableId1 FROM sb_timetables_weeks WHERE id = %2) UNION ALL"
                            " SELECT lessonId0, lessonId1, lessonId2, lessonId3, lessonId4, lessonId5, lessonId6, lessonId7 FROM sb_timetables"
                            " WHERE id = (SELECT timetableId2 FROM sb_timetables_weeks WHERE id = %2) UNION ALL"
                            " SELECT lessonId0, lessonId1, lessonId2, lessonId3, lessonId4, lessonId5, lessonId6, lessonId7 FROM sb_timetables"
                            " WHERE id = (SELECT timetableId3 FROM sb_timetables_weeks WHERE id = %2) UNION ALL"
                            " SELECT lessonId0, lessonId1, lessonId2, lessonId3, lessonId4, lessonId5, lessonId6, lessonId7 FROM sb_timetables"
                            " WHERE id = (SELECT timetableId4 FROM sb_timetables_weeks WHERE id = %2) UNION ALL"
                            " SELECT lessonId0, lessonId1, lessonId2, lessonId3, lessonId4, lessonId5, lessonId6, lessonId7 FROM sb_timetables"
                            " WHERE id = (SELECT timetableId5 FROM sb_timetables_weeks WHERE id = %2)) l")
                            .arg(QString::number(data[i].first), QString::number(data[0].second)));
                }
        }

        auto groups = new IdMap<GroupsCondition::Group *>();

        for (; i < data.length(); i += 3) {
            IntList * teachers = new IntList(2);
            teachers->push_back(data[i + 1].first);
            teachers->push_back(data[i + 1].second);
            groups->insert(data[i].first, new GroupsCondition::Group(2, false, teachers));
        }


        classesList.insert(cId, new Class{ cId, hours, groups, teachers, (char) data[0].first });
    }

    delete q;
}

IdMap<TimetableGenerator::Timetable *> *TimetableGenerator::createTimetable()
{
    QSqlQuery * q = DBA.getUsersDbQuery();

    // Генерация расписания будет происходить в несколько этапов. Мы выберем учителей и раздадим им классы так,
    // чтобы число часов в неделю соответствовало числу смен - 36 (или 30) и 72 (или 60) соответственно. Возможно меньше, но не больше.
    // Затем в соответствии с числом полученных часов выбираем количество дней, когда они будут работать.
    //
    // С учителями начальных классов система немного иная, они будут преподавать все уроки, кроме тех для которых не сказано другого.
    // Тем не менее у начальных классов могут быть уроки преподаваемые другими учителями, поэтому их общие уроки будут вставлены позже.
    // Также их уроки от других учителей будут первыми в очереди, потому что у них обычно меньше уроков.
    // (Ну и разумеется от числа часов зависит число часов в день)
    //
    // Соответственно. Сначала мы выбираем для каждого учителя классы, потом выбираем им смену, потом распределяем часы наименее востребованных
    // Учителей и потом наиболее востребованных.
    //
    // Это нужно для того, чтобы востребованный учитель закрывал окна между невостребованными.
    // К тому же следствием этой системы является "неравномерная" нагрузка и примерно симметричное расписание в параллелях.
    // Неравномерная нагрузка означает, что несколько дней наполненны преимущественно простыми предметами (невостребованными), а другие
    // сложными (более глубокими и сложными).
    //
    // Дополнительным условием является недопущение более двух уроков в один день по возможности. А есть такая необходимость случается, то
    // Выбрасывать исключение. Тут спорный момент нужно ли размещать все подряд, у нас в школе учителя были бы рады 3 урокам подряд, но
    // кто-то точно скажет, что это недопустимо. Мы не можем спрашивать про это в исключении, потому что генерация идёт в один.
    // Учитывая детерменированность этого процесса в будущем это можно легко поменять, но сейчас мы просто будем стараться не размещать
    // три одинаковых предмета подряд.
    //
    // Особой проблемой является генерация расписания с небольшими изменениями, потому что необходимо сохранить смены и учителей других классов,
    // что можно спустить на "схожесть зерен" (начальных условий), но мы так делать не хотим. Поэтому вместе с тем, как мы раздаём классы
    // нужно учитывать существующие записанные в Class данные о смене и учителях. Это означает, что ещё до этого нужно определиться со сменами.
    // Они зависят от многих факторов. Мы будем ориентироваться на самый дорогой в отношении часы/учителя урок.
    // На этом же этапе мы можем проверить достаточно ли вообще у нас учителей.
    //
    // TODO (Только что понял, что мы не записывали дополнительно учителей из других классов. Пока мы это опустим, наша цель
    // сгенерировать всё расписание целиком, а потом уже будем заниматься автоматическим изменением расписания.)
    // Также возможно, что мы сделаем это сейчас подгружая данные в процессе, как никак у нас есть все учителя
    // в списках учителей классов по урокам.
    // В любом случае работы по рефакторингу в проекте тонна и маленькая тележка. Технический долг делает *брр*
    //
    // Небольшое пояснение: 7 и 8 урок 1 смены это 1 и 2 уроки второй смены. 7 и 8 уроки это внеучебная деятельность.
    // ИЛИ 1 это "нулевой урок", 8 урок внеучебка. 8 урок второй смены это 1 урок второй смены.
    // На сколько я помню по школе нулевой урок это только для второй смены понятие. Да и вряд ли оно нам нужно
    // в том плане, что мы генерируем расписание, а нулевой урок это просто легкий способ его изменить.

    // Записываем классных руководителей. Это определяет только учителей для начальных классов и смены классов в зависимости от смены учителя
    IntMap classroomTeacher;
    q->exec("SELECT leader, id FROM sb_groups WHERE nameId LIKE '%c' AND active = 1");
    while (q->next())
        classroomTeacher.insert(q->value(0).toInt(), q->value(1).toInt());

    // Считаем наиболее нагруженный урок
    // Для учителей преподающих несколько предметов числов часов равно 8 * lessonHours / Σ lessonHoursI * (6 - days.length)
    QMap<double, int> lessonTension;
    double sumTension = 0.0;
    for (int lId : this->hours->lessons()) {
        double tension = hours->hoursForLesson(lId) / teachers->hoursOfLesson(hours, lId);
        // Даже если здесь напряженность больше единицы мы не отсылаем ошибку, ибо учителя
        // у которых несколько предметов могут распределиться по другому (хотя я почти уверен, что можно математически доказать, что нужно)
        // Мне лень этим заниматься, но я доказал, что сумма непряженностей должна быть меньше либо равна единице
        // (Смены должны учитываться в функции TechersConditions::hoursOfLesson, что сейчас не происходит,
        // так что все учителя работают по одной смене. можно обойти это просто создав два аккаунта для разных смен,
        // но разумеется этот костыль будет нужен не всегда)

        sumTension += tension;
        lessonTension.insert(tension, lId);
    }

    // Пока что предполагаем, что все работают одну смену
    if (sumTension > this->hours->lessons().length())
        TimetableGenerator::sendError("Недостаточно учителей");

    QVector<double> tensions = lessonTension.keys();
    std::sort(tensions.begin(), tensions.end());

    q->exec("SELECT id, nameId FROM sb_groups WHERE nameId LIKE 'c%'");
    QVector<IntList> classParallel(12);
    while (q->next())
        classParallel[q->value(1).toString().split("_")[1].toInt()].push_back(q->value(0).toInt());

    auto table = new IdMap<Timetable *>();

    // TODO

    delete q;
    return table;
}

void TimetableGenerator::writeTimetable(IdMap<Timetable *> *timetable)
{

}


TimetableGenerator::~TimetableGenerator()
{
    if (this->hours)
        delete hours;

    if (this->teachers)
        delete teachers;

    if (this->windows)
        delete windows;

    if (this->lessons)
        delete lessons;

    if (this->groups)
        delete groups;

    CURRENT_SESSION = nullptr;
}

int TimetableGenerator::conditions()
{
    return hoursConditions.length() + teachersConditions.length() +
            windowsConditions.length() + lessonsConditions.length() + groupsConditions.length();
}

void TimetableGenerator::newCondition(char type, QString subdata)
{
    this->currentCondition = TimetableCondition::getCondition(type, subdata);

    switch (type) {
    case 0: this->hoursConditions.push_back((HoursCondition *) this->currentCondition); break;
    case 1: this->teachersConditions.push_back((TeachersCondition *) this->currentCondition); break;
    case 2: this->windowsConditions.push_back((WindowsCondition *) this->currentCondition); break;
    case 3: this->lessonsConditions.push_back((LessonsCondition *) this->currentCondition); break;
    case 4: this->groupsConditions.push_back((GroupsCondition *) this->currentCondition); break;
    }
}

void TimetableGenerator::addRow(void * col0, void * col1, void * col2, void * col3)
{
    if (this->currentCondition)
        this->currentCondition->addRow(col0, col1, col2, col3);
    else {
        qDebug() << "No condition to add row";
        return;
    }
}

void TimetableGenerator::finishCondition()
{
    this->currentCondition = nullptr;
    checkExceptions();
}

void TimetableGenerator::build()
{
    switch (this->currentStep) {
    case STEP_INITIALIZE_CONDITIONS:

        this->hours = new TimetableGenerator::HoursConditions(this->hoursConditions);
        this->teachers = new TimetableGenerator::TeachersConditions(this->teachersConditions);
        this->windows = new TimetableGenerator::WindowsConditions(this->windowsConditions);
        this->lessons = new TimetableGenerator::LessonsConditions(this->lessonsConditions);
        this->groups = new TimetableGenerator::GroupsConditions(this->groupsConditions);

        this->currentStep++;


    case STEP_LINK_CONDITIONS:



        break;
    }
}

void TimetableGenerator::receiveAnswer(QDataStream data)
{
    READ_CHAR(eId);
    READ_CHAR(answer);
    switch (eId) {
    case EXCEPTION_REPETABLE_LESSON_HOURS:
    case EXCEPTION_REPETABLE_LESSON_PREFERENCE:
    case EXCEPTION_REPETABLE_LESSON_GROUPS:
    case EXCEPTION_REPETABLE_TEACHER:
        switch (answer) {
        case 0:
            delete this;
            break;
        case 1:
            acceptedExceptions.insert(eId);
            break;
        }
        break;
    }
}

void TimetableGenerator::markException(int exceptionId)
{
    if (!acceptedExceptions.contains(exceptionId))
        markedExceptions.insert(exceptionId);
}

void TimetableGenerator::checkExceptions()
{
    QStringList data;

    for (auto e : markedExceptions) {
        switch (e) {
        case EXCEPTION_REPETABLE_LESSON_HOURS:
        case EXCEPTION_REPETABLE_LESSON_PREFERENCE:
            data.append(QString::number(conditions()));
            sendQuestion(FORM_IGNORE_NOW_OR_ENTIRELY, "Повторение урока", data);
            break;
        case EXCEPTION_REPETABLE_LESSON_GROUPS:
            data.append(QString::number(conditions()));
            sendQuestion(FORM_IGNORE_NOW_OR_ENTIRELY, "Повторение группы", data);
            break;
        case EXCEPTION_REPETABLE_TEACHER:
            data.append(QString::number(conditions()));
            sendQuestion(FORM_IGNORE_NOW_OR_ENTIRELY, "Повторение учителя", data);
            break;
        }
    }

    markedExceptions.clear();
}

TimetableGenerator * TimetableGenerator::startSession()
{
    CURRENT_SESSION = new TimetableGenerator();
    return TimetableGenerator::CURRENT_SESSION;
}

TimetableGenerator * TimetableGenerator::currentSession()
{
    return TimetableGenerator::CURRENT_SESSION;
}

bool TimetableGenerator::isInSession()
{
    return TimetableGenerator::CURRENT_SESSION;
}

void TimetableGenerator::sendError(QString errorString)
{
    sendQuestion(FORM_ERROR, errorString, QStringList(0));
}

void TimetableGenerator::sendQuestion(int formId, QString question, QStringList data)
{
    QByteArray answerData;
    QDataStream answer(&answerData, QIODevice::WriteOnly);
    answer.setVersion(QDataStream::Version::Qt_6_4);

    WRITE_INT(formId);
    WRITE_DYNAMIC_STRING(question, int);
    WRITE_INT(data.length());
    for (int i = 0; i < data.length(); i++) {
        WRITE_DYNAMIC_STRING(data[i], short);
    }

    Server::SERVER->send(answerData);
}


int TimetableGenerator::TimetableCondition::rows()
{
    return this->rowsNumber;
}


void TimetableGenerator::TimetableCondition::addRow(void *col0, void *col1, void *col2, void *col3) {
    Q_UNUSED(col0)
    Q_UNUSED(col1)
    Q_UNUSED(col2)
    Q_UNUSED(col3)
}

char TimetableGenerator::TimetableCondition::type()
{
    return -1;
}

TimetableGenerator::TimetableCondition *TimetableGenerator::TimetableCondition::getCondition(char type, QString subdata)
{
    switch (type) {
    case 0: return (TimetableCondition *) new HoursCondition(subdata);
    case 1: return (TimetableCondition *) new TeachersCondition();
    case 2: return (TimetableCondition *) new WindowsCondition(subdata);
    case 3: return (TimetableCondition *) new LessonsCondition();
    case 4: return (TimetableCondition *) new GroupsCondition(subdata);
    default: return nullptr;
    }
}


TimetableGenerator::HoursCondition::HoursCondition(QString subdata)
{
    if (subdata.toInt()) {
        this->specialClass = false;
        this->subdata = subdata.toInt();
    } if (subdata == "Все") {
        this->subdata = 0;
    } else {
        QSqlQuery * q = DBA.getUsersDbQuery();
        q->exec("SELECT id FROM sb_groups WHERE name = '" + subdata + "' AND active = 1");

        this->specialClass = true;
        this->subdata = q->value(0).toInt();

        delete q;
    }
}

bool TimetableGenerator::HoursCondition::isEveryone()
{
    return !subdata;
}

bool TimetableGenerator::HoursCondition::isSpecialClass()
{
    return this->specialClass && subdata;
}

bool TimetableGenerator::HoursCondition::isParallelClasses()
{
    return !this->specialClass && subdata;
}

int TimetableGenerator::HoursCondition::getClass()
{
    return subdata;
}

QVector<int> TimetableGenerator::HoursCondition::getClasses()
{
    if (this->isSpecialClass()) {
        QVector<int> r(1);
        r.push_back(this->subdata);
        return r;
    }

    QSqlQuery * q = DBA.getUsersDbQuery();

    if (this->isParallelClasses())
        q->exec("SELECT id FROM sb_groups WHERE nameId LIKE 'c%_" + QString::number(this->subdata) + "_%' AND active = 1");
    else
        q->exec("SELECT id FROM sb_groups WHERE nameId LIKE 'c%' AND active = 1");

    QVector<int> r;
    while (q->next())
        r.push_back(q->value(0).toInt());

    delete q;
    return r;
}

QMap<int, int> TimetableGenerator::HoursCondition::getHours()
{
    return this->data;
}

void TimetableGenerator::HoursCondition::addRow(void *col0, void *col1, void *col2, void *col3)
{
    Q_UNUSED(col2)
    Q_UNUSED(col3)

    int lessonId = ((QInteger *) col0)->value();
    int hours = ((QInteger *) col1)->value();

    if (this->data.contains(lessonId)) {
        TimetableGenerator::currentSession()->markException(EXCEPTION_REPETABLE_LESSON_HOURS);
        this->data[lessonId] += hours;
    } else this->data.insert(lessonId, hours);

    delete (QInteger *) col0;
    delete (QInteger *) col1;

    this->rowsNumber++;
}

char TimetableGenerator::HoursCondition::type()
{
    return 0;
}


TimetableGenerator::TeachersCondition::TeachersCondition() { }

QVector<int> TimetableGenerator::TeachersCondition::getTeachers()
{
    return data.keys();
}

QVector<int> * TimetableGenerator::TeachersCondition::getTeacherLessons(int teacherId)
{
    return data[teacherId]->lessons();
}

QVector<int> * TimetableGenerator::TeachersCondition::getTeacherVacations(int teacherId)
{
    return data[teacherId]->vacations();
}

bool TimetableGenerator::TeachersCondition::isMonoProfileTeacher(int teacherId)
{
    return data[teacherId]->lessons()->length() == 1;
}

bool TimetableGenerator::TeachersCondition::isFullWeekTeacher(int teacherId)
{
    return !data[teacherId]->vacations()->length();
}

void TimetableGenerator::TeachersCondition::addRow(void *col0, void *col1, void *col2, void *col3)
{
    Q_UNUSED(col3);

    int teacherId = ((QInteger *) col0)->value();
    QVector<int> * lessons = (QVector<int> *) col1;
    QVector<int> * vacations = (QVector<int> *) col2;

    if (this->data.contains(teacherId)) {
        TimetableGenerator::currentSession()->markException(EXCEPTION_REPETABLE_TEACHER);
        this->data.insert(0, nullptr);
    } else {
        this->data.insert(teacherId, new Teacher(teacherId, lessons, vacations));
    }

    delete (QInteger *) col0;

    this->rowsNumber++;
}

char TimetableGenerator::TeachersCondition::type()
{
    return 1;
}


TimetableGenerator::TeachersCondition::Teacher::Teacher(int teacherId, QVector<int> *lessons, QVector<int> *vacations)
{
    this->teacherId = teacherId;
    this->teacherLessons = lessons;
    this->teacherVacations = vacations;
}

int TimetableGenerator::TeachersCondition::Teacher::id()
{
    return this->teacherId;
}

QVector<int> *TimetableGenerator::TeachersCondition::Teacher::lessons()
{
    return this->teacherLessons;
}

QVector<int> *TimetableGenerator::TeachersCondition::Teacher::vacations()
{
    return this->teacherVacations;
}


TimetableGenerator::WindowsCondition::WindowsCondition(QString subdata)
{
    QSqlQuery * q = DBA.getUsersDbQuery();
    q->exec("SELECT id FROM sb_groups WHERE name = '" + subdata + "' AND active = 1");

    this->classId = q->value(0).toInt();

    delete q;
}

int TimetableGenerator::WindowsCondition::getClass()
{
    return classId;
}

QVector<int> TimetableGenerator::WindowsCondition::getTeachers()
{
    return data.keys();
}

QVector<int> * TimetableGenerator::WindowsCondition::getTeacherDays(int teacherId)
{
    return data[teacherId]->days();
}

QVector<int> * TimetableGenerator::WindowsCondition::getTeacherLessons(int teacherId)
{
    return data[teacherId]->lessons();
}

char TimetableGenerator::WindowsCondition::getTeacherShifts(int teacherId)
{
    return data[teacherId]->shifts();
}

char TimetableGenerator::WindowsCondition::getShifts()
{
    return this->shifts;
}

void TimetableGenerator::WindowsCondition::addRow(void *col0, void *col1, void *col2, void *col3)
{
    int teacherId = ((QInteger *) col0)->value();
    QVector<int> * days = (QVector<int> *) col1;
    QVector<int> * lessons = (QVector<int> *) col2;
    char shifts = ((QInteger *) col3)->value();

    if (data.contains(teacherId)) {
        TimetableGenerator::currentSession()->markException(EXCEPTION_REPETABLE_TEACHER);
        data.insert(0, nullptr);
    } else data.insert(teacherId, new Preference(teacherId, days, lessons, shifts));

    delete (QInteger *) col0;
    delete (QInteger *) col3;

    this->rowsNumber++;
}

char TimetableGenerator::WindowsCondition::type()
{
    return 2;
}

TimetableGenerator::WindowsCondition::Preference *TimetableGenerator::WindowsCondition::getPreference(int teacherId)
{
    return this->data[teacherId];
}


TimetableGenerator::WindowsCondition::Preference::Preference(int teacherId, QVector<int> *days, QVector<int> *lessons, char shifts)
{
    this->teacherId = teacherId;
    this->teacherDays = days;
    this->teacherLessons = lessons;
    this->teacherShifts = shifts;
}

int TimetableGenerator::WindowsCondition::Preference::id()
{
    return this->teacherId;
}

QVector<int> *TimetableGenerator::WindowsCondition::Preference::days()
{
    return this->teacherDays;
}

QVector<int> *TimetableGenerator::WindowsCondition::Preference::lessons()
{
    return this->teacherLessons;
}

char TimetableGenerator::WindowsCondition::Preference::shifts()
{
    return this->teacherShifts;
}


TimetableGenerator::LessonsCondition::LessonsCondition() { }

QVector<int> TimetableGenerator::LessonsCondition::lessons()
{
    return data.keys();
}

QVector<int> * TimetableGenerator::LessonsCondition::days(int lessonId)
{
    return data[lessonId].first;
}

QVector<int> * TimetableGenerator::LessonsCondition::hours(int lessonId)
{
    return data[lessonId].second;
}

void TimetableGenerator::LessonsCondition::addRow(void *col0, void *col1, void *col2, void *col3)
{
    Q_UNUSED(col3)

    int lessonId = ((QInteger *) col0)->value();
    QVector<int> * days = (QVector<int> *) col1;
    QVector<int> * hours = (QVector<int> *) col2;

    if (data.contains(lessonId)) {
        TimetableGenerator::currentSession()->markException(EXCEPTION_REPETABLE_LESSON_PREFERENCE);
        data[lessonId].first->append(*days);
        data[lessonId].second->append(*hours);
    }
    else data.insert(lessonId, QPair<QVector<int> *, QVector<int> *>(days, hours));

    delete (QInteger *) col0;

    this->rowsNumber++;
}

char TimetableGenerator::LessonsCondition::type()
{
    return 3;
}


TimetableGenerator::GroupsCondition::GroupsCondition(QString subdata)
{
    if (subdata == "Все")
        this->subdata = 0;
    else if (subdata.toInt())
        this->subdata = subdata.toInt();
    else {
        QSqlQuery * q = DBA.getUsersDbQuery();
        q->exec("FROM sb_groups SELECT id WHERE name = '" + subdata + "' AND active = 1");
        specialClass = true;
        this->subdata = q->value(0).toInt();
        delete q;
    }
}

QVector<int> TimetableGenerator::GroupsCondition::lessons()
{
    return data.keys();
}

int TimetableGenerator::GroupsCondition::groups(int lessonId)
{
    return data[lessonId]->groups();
}

bool TimetableGenerator::GroupsCondition::isBySex(int lessonId)
{
    return data[lessonId]->isBySex();
}

QVector<int> *TimetableGenerator::GroupsCondition::teachers(int lessonId)
{
    return data[lessonId]->teachers();
}

bool TimetableGenerator::GroupsCondition::isEveryone()
{
    return !subdata;
}

bool TimetableGenerator::GroupsCondition::isSpecialClass()
{
    return specialClass;
}

bool TimetableGenerator::GroupsCondition::isParallelClasses()
{
    return !specialClass && !isEveryone();
}

int TimetableGenerator::GroupsCondition::getClass()
{
    return subdata;
}

QVector<int> TimetableGenerator::GroupsCondition::getClasses()
{
    if (this->isSpecialClass()) {
        QVector<int> r(1);
        r.push_back(this->subdata);
        return r;
    }

    QSqlQuery * q = DBA.getUsersDbQuery();

    if (this->isParallelClasses())
        q->exec("SELECT id FROM sb_groups WHERE nameId LIKE 'c%_" + QString::number(this->subdata) + "_%' AND active = 1");
    else
        q->exec("SELECT id FROM sb_groups WHERE nameId LIKE 'c%' AND active = 1");

    QVector<int> r;
    while (q->next())
        r.push_back(q->value(0).toInt());

    delete q;
    return r;
}

void TimetableGenerator::GroupsCondition::addRow(void *col0, void *col1, void *col2, void *col3)
{
    int lessonId = ((QInteger *) col0)->value();
    int groups = ((QInteger *) col1)->value();
    bool isBySex = ((QInteger *) col2)->value();
    QVector<int> * teachers = (QVector<int> *) col3;

    if (data.contains(lessonId)) {
        TimetableGenerator::currentSession()->markException(EXCEPTION_REPETABLE_LESSON_GROUPS);
    } else data.insert(lessonId, new Group(groups, isBySex, teachers));

    delete (QInteger *) col0;
    delete (QInteger *) col1;
    delete (QInteger *) col2;

    this->rowsNumber++;
}

char TimetableGenerator::GroupsCondition::type()
{
    return 4;
}

IdMap<TimetableGenerator::GroupsCondition::Group *> TimetableGenerator::GroupsCondition::getData()
{
    return this->data;
}


TimetableGenerator::GroupsCondition::Group::Group(int groups, bool isBySex, QVector<int> * teachers) : groupsNumber(groups), bySex(isBySex) {
    this->teachersList = teachers;

    if (this->bySex) {
        TimetableGenerator::sendError("Groups by sex not implemented");
    }
}

int TimetableGenerator::GroupsCondition::Group::groups()
{
    return groupsNumber;
}

bool TimetableGenerator::GroupsCondition::Group::isBySex()
{
    return bySex;
}

QVector<int> *TimetableGenerator::GroupsCondition::Group::teachers()
{
    return teachersList;
}


TimetableGenerator::HoursConditions::HoursConditions(QVector<HoursCondition *> conditions)
{
    // Считаем для каких классов необходимо составить расписание и создаём им счётчики
    for (auto c : conditions)
        if (c->isSpecialClass()) {
            this->hoursPerLessonForClasses.insert(c->getClass(), new IntMap);
            this->hoursPerClass.insert(c->getClass(), 0);
        }
        else for (auto id : c->getClasses()) {
                this->hoursPerLessonForClasses.insert(id, new IntMap);
                this->hoursPerClass.insert(id, 0);
            }

    // Складываем часы
    for (auto c : conditions) {
        if (c->isEveryone())
            for (int id : this->hoursPerLessonForClasses.keys())
                for (int lesson : c->getHours().keys()) {
                    int hours = c->getHours()[lesson];

                    this->hoursPerLessonForClasses[id]->insert
                        (lesson, this->hoursPerLessonForClasses.value(id)->value(lesson) + hours);

                    this->hoursPerClass[id] += hours;
                    this->hoursPerLesson[lesson] += hours;

                }

        else if (c->isParallelClasses())
            for (int id : c->getClasses())
                for (int lesson : c->getHours().keys()) {
                    int hours = c->getHours()[lesson];

                    this->hoursPerLessonForClasses[id]->insert
                        (lesson, this->hoursPerLessonForClasses.value(id)->value(lesson) + hours);

                    this->hoursPerClass[id] += hours;
                    this->hoursPerLesson[lesson] += hours;
                }

        else for (int lesson : c->getHours().keys()) {
            int hours = c->getHours()[lesson];

            this->hoursPerLessonForClasses[c->getClass()]->insert
                (lesson, this->hoursPerLessonForClasses.value(c->getClass())->value(lesson) + hours);

            this->hoursPerClass[c->getClass()] += hours;
            this->hoursPerLesson[lesson] += hours;
        }
    }
}

IntList TimetableGenerator::HoursConditions::classes()
{
    return hoursPerClass.keys();
}

IntList TimetableGenerator::HoursConditions::lessons()
{
    return hoursPerLesson.keys();
}

IntMap * TimetableGenerator::HoursConditions::hoursPerLessonForClass(int classId)
{
    return hoursPerLessonForClasses[classId];
}

int TimetableGenerator::HoursConditions::hoursForClass(int classId)
{
    return hoursPerClass[classId];
}

int TimetableGenerator::HoursConditions::hoursForLesson(int lessonId)
{
    return hoursPerLesson[lessonId];
}


TimetableGenerator::TeachersConditions::TeachersConditions(QVector<TeachersCondition *> conditions)
{
    for (auto c : conditions)
        for (int id : c->getTeachers()) {
            if (this->teachersDays.contains(id))
                TimetableGenerator::CURRENT_SESSION->markException(EXCEPTION_REPETABLE_TEACHER);

            int daysValue;
            for (int d : *c->getTeacherVacations(id))
                daysValue |= 1 << d;

            this->teachersDays.insert(id, daysValue);
            this->teacherLessons.insert(id, c->getTeacherLessons(id));
        }
}

int TimetableGenerator::TeachersConditions::daysOfTeacher(int teacherId)
{
    return this->teachersDays[teacherId];
}

double TimetableGenerator::TeachersConditions::hoursOfLesson(HoursConditions * hours, int lessonId)
{
    double sum = 0.0;
    for (auto tId : this->teachers()) {
        int lessonsSum = 0;
        for (auto lId : *lessonsOfTeacher(tId))
            lessonsSum += hours->hoursForLesson(lId);

        int days = 0;
        int daysRaw = daysOfTeacher(tId);
        for (int i = 0; i < 6; i++) {
            days += daysRaw & 1;
            daysRaw >>= 1;
        }

        sum += 8.0 * hours->hoursForLesson(lessonId) / lessonsSum * (6 - days);
    }

    return sum;
}

IntList *TimetableGenerator::TeachersConditions::lessonsOfTeacher(int teacherId)
{
    return this->teacherLessons[teacherId];
}

IntList TimetableGenerator::TeachersConditions::teachers()
{
    return this->teachersDays.keys();
}


TimetableGenerator::WindowsConditions::WindowsConditions(QVector<WindowsCondition *> conditions)
{
    for (auto c : conditions)
        if (!this->teachersPerClass.contains(c->getClass()))
            this->teachersPerClass.insert(c->getClass(), new IdMap<WindowsCondition::Preference *>());
        else TimetableGenerator::CURRENT_SESSION->markException(EXCEPTION_REPETABLE_TEACHER);

    for (auto c : conditions) {
        for (auto id : c->getTeachers()) {
            this->teachersPerClass.value(c->getClass())->insert(id, c->getPreference(id));
        }
    }
}

IntList TimetableGenerator::WindowsConditions::classes()
{
    return this->teachersPerClass.keys();
}

IntList TimetableGenerator::WindowsConditions::teachers(int classId)
{
    return this->teachersPerClass[classId]->keys();
}

IntList *TimetableGenerator::WindowsConditions::getTeacherDays(int classId, int teacherId)
{
    return this->teachersPerClass[classId]->value(teacherId)->days();
}

IntList *TimetableGenerator::WindowsConditions::getTeacherLessons(int classId, int teacherId)
{
    return this->teachersPerClass[classId]->value(teacherId)->lessons();
}

char TimetableGenerator::WindowsConditions::getTeacherShifts(int classId, int teacherId)
{
    return this->teachersPerClass[classId]->value(teacherId)->shifts();
}

char TimetableGenerator::WindowsConditions::getShifts(int classId)
{
    char shifts = 0;
    for (auto t : this->teachersPerClass[classId]->values()) {
        shifts |= t->shifts();
        if (shifts == 3)
            return shifts;
    }

    return shifts;
}


TimetableGenerator::LessonsConditions::LessonsConditions(QVector<LessonsCondition *> conditions)
{
    for (auto c : conditions)
        for (auto lId : c->lessons())
            if (!this->lessonsList->contains(lId)) {
                this->lessonsList->insert(lId);
                if (c->days(lId))
                    this->daysList.insert(lId, c->days(lId));
                if (c->hours(lId))
                    this->hoursList.insert(lId, c->hours(lId));
            }
            else {
                TimetableGenerator::CURRENT_SESSION->markException(EXCEPTION_REPETABLE_LESSON_PREFERENCE);
                if (c->days(lId))
                    this->daysList.value(lId)->append(*c->days(lId));
                if (c->hours(lId))
                    this->hoursList.value(lId)->append(*c->hours(lId));
            }
}

IntSet * TimetableGenerator::LessonsConditions::lessons()
{
    return this->lessonsList;
}

IntList * TimetableGenerator::LessonsConditions::days(int lessonId)
{
    return this->daysList[lessonId];
}

IntList * TimetableGenerator::LessonsConditions::hours(int lessonId)
{
    return this->hoursList[lessonId];
}


TimetableGenerator::GroupsConditions::GroupsConditions(QVector<GroupsCondition *> conditions)
{
    for (auto c : conditions)
        if (c->isSpecialClass())
            this->data.insert(c->getClass(), c->getData());
        else for (auto id : c->getClasses())
                this->data.insert(id, c->getData());
}

IntList TimetableGenerator::GroupsConditions::classes()
{
    return data.keys();
}

IntList TimetableGenerator::GroupsConditions::lessons(int classId)
{
    return data[classId].keys();
}

int TimetableGenerator::GroupsConditions::groupsNumber(int classId, int lessonId)
{
    return this->data[classId].value(lessonId)->groups();
}

bool TimetableGenerator::GroupsConditions::isBySex(int classId, int lessonId)
{
    return this->data[classId].value(lessonId)->isBySex();
}

IntList *TimetableGenerator::GroupsConditions::teachers(int classId, int lessonId)
{
    return this->data[classId].value(lessonId)->teachers();
}


template<class T>
TimetableGenerator::Table<T>::Table()
{
    this->shift = 0;
}

template<class T>
TimetableGenerator::Table<T>::Table(QVector<T> lessons)
{
    this->lessons = lessons;
}

template<class T>
TimetableGenerator::Table<T>::Table(char shift)
{
    this->shift = shift;
}

template<class T>
TimetableGenerator::Table<T>::Table(QVector<T> lessons, char shift)
{
    this->lessons = lessons;
    this->shift = shift;
}
