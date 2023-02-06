#ifndef TIMETABLEGENERATOR_H
#define TIMETABLEGENERATOR_H

#include <QString>
#include <QObject>
#include <QMap>
#include <QDebug>


#define STEP_INITIALIZE_CONDITIONS            0
#define STEP_LINK_CONDITIONS                  1

#define FORM_ERROR                            0
#define FORM_IGNORE_NOW_OR_ENTIRELY           1

#define EXCEPTION_REPETABLE_LESSON_HOURS      0
#define EXCEPTION_REPETABLE_LESSON_PREFERENCE 1
#define EXCEPTION_REPETABLE_LESSON_GROUPS     2
#define EXCEPTION_REPETABLE_TEACHER           3

class QInteger {
    int i;
public:
    QInteger(int i);

    int value();
};

typedef QMap<int, int> IntMap;
typedef QPair<int, int> IntPair;
typedef QVector<int> IntList;
typedef QSet<int> IntSet;
template <class T> class IdMap : public QMap<int, T> { };

class TimetableGenerator
{
public:
    ~TimetableGenerator();

    int conditions();

    void newCondition(char type, QString subdata="");
    void addRow(void * col0, void * col1, void * col2=nullptr, void * col3=nullptr);
    void finishCondition();

    void build();
    void receiveAnswer(QDataStream data);

    void markException(int exceptionId);
    void checkExceptions();

    static TimetableGenerator * startSession();
    static TimetableGenerator * currentSession();
    static bool isInSession();

    static void sendError(QString errorString);
    static void sendQuestion(int formId, QString question, QStringList data);

private:
    TimetableGenerator();

    IntSet markedExceptions;
    IntSet acceptedExceptions;

    class TimetableCondition {
    public:
        virtual void addRow(void * col0, void * col1, void * col2=nullptr, void * col3=nullptr);
        virtual char type();

        static TimetableCondition * getCondition(char type, QString subdata="");
    protected:
        int rowsNumber = 0;
        int rows();
    };

    class HoursCondition:TimetableCondition {
    public:
        HoursCondition(QString subdata);

        bool isEveryone();
        bool isSpecialClass();
        bool isParallelClasses();

        // Class id if isSpecialClass
        int getClass();
        // Classes id if parallel classes or a single value if special
        IntList getClasses();
        // LessonId -> Hours
        IntMap getHours();

        // Condition interface
        void addRow(void *col0, void *col1, void *col2, void *col3);
        char type();

    private:
        bool specialClass;
        int subdata;

        IntMap data;
    };

    class TeachersCondition:TimetableCondition {
    public:
        TeachersCondition();

        IntList getTeachers();
        IntList * getTeacherLessons(int teacherId);
        IntList * getTeacherVacations(int teacherId);

        bool isMonoProfileTeacher(int teacherId);
        bool isFullWeekTeacher(int teacherId);

        // Condition interface
        void addRow(void *col0, void *col1, void *col2, void *col3);
        char type();

    private:
        class Teacher {
        public:
            Teacher(int teacherId, IntList * lessons, IntList * vacations);

            int id();
            IntList * lessons();
            IntList * vacations();

        private:
            int teacherId;
            IntList * teacherLessons;
            IntList * teacherVacations;
        };

        IdMap<Teacher *> data;
    };

    class WindowsCondition:TimetableCondition {
    public:
        WindowsCondition(QString subdata);

        int getClass();

        IntList getTeachers();
        IntList * getTeacherDays(int teacherId);
        IntList * getTeacherLessons(int teacherId);
        char getTeacherShifts(int teacherId);

        char getShifts();

        // Condition interface
        void addRow(void *col0, void *col1, void *col2, void *col3);
        char type();

        class Preference {
        public:
            Preference(int teacherId, IntList * days, IntList * lessons, char shifts);

            int id();
            IntList * days();
            IntList * lessons();
            char shifts();

            void append(IntList * days, IntList * lessons, char shifts);

        private:
            int teacherId;
            IntList * teacherDays;
            IntList * teacherLessons;
            char teacherShifts;
        };

        Preference * getPreference(int teacherId);
    private:

        IdMap<Preference *> data;
        int classId;
        char shifts = 0;
    };

    class LessonsCondition:TimetableCondition {
    public:
        LessonsCondition();

        IntList lessons();
        IntList * days(int lessonId);
        IntList * hours(int lessonId);

        // Condition interface
        void addRow(void *col0, void *col1, void *col2, void *col3);
        char type();

    private:
        QMap<int, QPair<IntList *, IntList *>> data;

    };

    class GroupsCondition:TimetableCondition {
    public:
        GroupsCondition(QString subdata);

        QVector<int> lessons();
        int groups(int lessonId);
        bool isBySex(int lessonId);
        QVector<int> * teachers(int lessonId);

        bool isEveryone();
        bool isSpecialClass();
        bool isParallelClasses();

        // Class id if isSpecialClass
        int getClass();
        // Classes id if parallel classes or a single value if special
        QVector<int> getClasses();

        // Condition interface
        void addRow(void *col0, void *col1, void *col2, void *col3);
        char type();

        class Group {
        public:
            Group(int groups, bool isBySex, QVector<int> * teachers);

            int groups();
            bool isBySex();
            QVector<int> * teachers();

        private:
            int groupsNumber;
            bool bySex;
            QVector<int> * teachersList;
        };

        IdMap<Group *> getData();

    private:
        IdMap<Group *> data;

        int subdata;
        bool specialClass;
    };


    class TeachersConditions;
    class WindowsConditions;
    class LessonsConditions;
    class GroupsConditions;

    class HoursConditions {
    public:
        HoursConditions(QVector<TimetableGenerator::HoursCondition *> conditions);

        IntList classes();
        IntList lessons();

        IntMap * hoursPerLessonForClass(int classId);

        int hoursForClass(int classId);
        int hoursForLesson(int lessonId);

    private:
        IdMap<IntMap *> hoursPerLessonForClasses;
        IntMap hoursPerLesson;
        IntMap hoursPerClass;
    };

    class TeachersConditions {
    public:
        TeachersConditions(QVector<TimetableGenerator::TeachersCondition *> conditions);

        IntList teachers();
        IntList * lessonsOfTeacher(int teacherId);
        int daysOfTeacher(int teacherId);

        double hoursOfLesson(HoursConditions * hours, int lessonId);

    private:
        IntMap teachersDays;
        IdMap<IntList *> teacherLessons;
    };

    class WindowsConditions {
    public:
        WindowsConditions(QVector<TimetableGenerator::WindowsCondition *> conditions);

        IntList classes();
        IntList teachers(int classId);

        IntList * getTeacherDays(int classId, int teacherId);
        IntList * getTeacherLessons(int classId, int teacherId);
        char getTeacherShifts(int classId, int teacherId);

        char getShifts(int classId);

    private:
        IdMap<IdMap<WindowsCondition::Preference *> *> teachersPerClass;
    };

    class LessonsConditions {
    public:
        LessonsConditions(QVector<TimetableGenerator::LessonsCondition *> conditions);

        IntSet * lessons();
        IntList * days(int lessonId);
        IntList * hours(int lessonId);

    private:
        IntSet * lessonsList;
        IdMap<IntList *> daysList;
        IdMap<IntList *> hoursList;
    };

    class GroupsConditions {
    public:
        GroupsConditions(QVector<TimetableGenerator::GroupsCondition *> conditions);

        IntList classes();
        IntList lessons(int classId);

        int groupsNumber(int classId, int lessonId);
        bool isBySex(int classId, int lessonId);
        IntList * teachers(int classId, int lessonId);

    private:
        IdMap<IdMap<GroupsCondition::Group *>> data;
    };

    static TimetableGenerator * CURRENT_SESSION;

    TimetableCondition * currentCondition = nullptr;
    int currentStep = 0;

    QVector<TimetableGenerator::HoursCondition *> hoursConditions;
    QVector<TimetableGenerator::TeachersCondition *> teachersConditions;
    QVector<TimetableGenerator::WindowsCondition *> windowsConditions;
    QVector<TimetableGenerator::LessonsCondition *> lessonsConditions;
    QVector<TimetableGenerator::GroupsCondition *> groupsConditions;

    HoursConditions * hours;
    TeachersConditions * teachers;
    WindowsConditions * windows;
    LessonsConditions * lessons;
    GroupsConditions * groups;

    struct Lesson {
        const int lessonId;
        const int teacherId;
    };

    typedef  QVector<Lesson> VirtualLesson;

    template <class T> class Table {

        QVector<T> lessons = QVector<T>(48);
        char shift;

    public:
        Table();
        Table(QVector<T> lessons);
        Table(char shift);
        Table(QVector<T> lessons, char shift);
    };

    typedef Table<Lesson> Timetable;
    typedef Table<int> TeacherTable;

    void formTeachers();
    void formLessons();

    void includeRestOfClasses();

    IdMap<Timetable *> * createTimetable();

    void writeTimetable(IdMap<Timetable *> * timetable);

    struct Teacher {
        const int id;

        // Общая информация
        const IntList * lessons;
        const int days;

        // Конкретные указания для классов
        struct Preference {
            const IntList * days;
            const IntList * lessons;
            const char shifts;
        };

        IdMap<Preference *> preferences;
    };

    struct Class {
        const int id;

        // Общая информация по часам
        const IntMap * hours;
        // Уроки для которых требуется несколько учителей
        const IdMap<GroupsCondition::Group *> * groups;

        const IntMap teachers;

        const char shift;
    };

    IdMap<Teacher *> teachersList;
    IdMap<Class *> classesList;
};

#endif // TIMETABLEGENERATOR_H
