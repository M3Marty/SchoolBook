#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <chatwidget.h>
#include <postwidget.h>
#include <taskwidget.h>
#include <groupwidget.h>

#include <post.h>
#include <user.h>
#include <timetable.h>

#include <QMainWindow>
#include <QListWidgetItem>
#include <QTableWidget>

#include <QVector>
#include <QTcpSocket>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void goProfile(int id);
    User * getUser(int id);
    User * getUser(QStringList nameSurname, int defaultReturn=-1);
    bool knowUser(int id);

    void deletePost(int id);
    void addPost(Post post, int where=-1);

    void addChat(int id, QString name);
    void updateChat(int id, QString, long long);

    void send(QByteArray &);

    void addGroupToLead(QString name);
    void addGroupToMember(QString name);

    void addGroupToDisplay(QString name, QVector<int> members);

    void setProfileViewData(QString name, QString surname, char role, QString groupName);

    void setGetCode(QString code);

    void setOccuipedLiters(char *);

    void addKnownUser(User u);
    void addKnownUser(int id, int role, QString name, QString surname);

    void addOwnedTeacher(int id, int rold, QString name, QString surname, QString groupName);
    void addOwnedStudent(int id, int rold, QString name, QString surname, QString groupName);

    void addLesson(QString lesson);

    void clearTasks();
    void moveTask(TaskWidget *, char, char);
    void addTask(int id, int authorId, QString aName, QString text, char status);

    void setTeacherOfLessonsForClass(QStringList list);

    void setParents(QVector<QPair<int, QString>> list);

    void clearYearTimetable(QVector<int> lessons);
    void clearQuarterTimetable(QVector<int> lessons);
    void clearWeekTimetable();

    void addYearTimetableValue(int userId, int lessonId, int mark, int column);
    void addQuarterTimetableValue(int userId, int lessonId, int mark, int column);
    void addWeekTimetableValue(QVector<int> lessons,  QStringList tasks, int dayId);
    void addWeekMark(int userId, int lessonId, int mark, int dayId);
    QVector<int> orderedLessons;

    static QString USER_NAME;
    static QString USER_SURNAME;
    static int USER_ROLE;
    static int USER_ID;
    static QString USER_GROUP;

    static MainWindow * MAIN;

    QVector<QString> lessons;
    QVector<QString> leadGroups;
    QVector<QString> memberGroups;
    QVector<User> ownedTeachers;

    QMap<QString, QVector<int>> groupMembers;
    QMap<QString, int> groupLeader;
    QMap<QString, GroupWidget *> groupWidgets;

    QVector<QMap<QString, Timetable *>> timetables;

    int lastPostId;

    void on_editTimetableNumber_activated(int index);
    void on_editTimetableNumber_currentIndexChanged(int index);

private slots:
    void on_homeTB_clicked();

    void on_goNewPost_clicked();

    void on_newsChooseImageB_clicked();

    void on_newsCreateB_clicked();

    void on_newsRemoveImageB_clicked();

    void on_calendarTB_clicked();

    void on_groupTB_clicked();

    void on_goProfile_clicked();

    void on_tabWidget_currentChanged(int index);

    void on_toolsTeacherButton_clicked();

    void on_toolsParentButton_clicked();

    void on_toolsGetCodeButton_clicked();

    void on_toolsLessonsButton_clicked();

    void on_toolstTimetableButton_clicked();

    void on_toolsClassButton_clicked();

    void on_getCodeRole_currentIndexChanged(int index);

    void on_getCodeButton_clicked();

    void on_toolsCreateClassButton_clicked();

    void on_createClassNumber_currentIndexChanged(int index);

    void on_createClassButton_clicked();

    void on_classesChoose_currentIndexChanged(int index);

    void on_classesToChoose_currentIndexChanged(int index);

    void on_classesFromChoose_currentIndexChanged(int index);

    void on_classesTo2FromButton_clicked();

    void on_classesFrom2ToButton_clicked();

    void on_classesToList_itemDoubleClicked(QListWidgetItem *item);

    void on_lessonsList_currentTextChanged(const QString &currentText);

    void on_lessonsAdd_clicked();

    void on_lessonsDelete_clicked();

    void on_lessonsRename_clicked();

    void on_classesToChoose_activated(int index);

    void on_parentsChoose_activated(int index);

    void on_parentsList_itemDoubleClicked(QListWidgetItem *item);

    void on_classesFromList_itemDoubleClicked(QListWidgetItem *item);

    void on_pushButton_clicked();

    void on_editTimetableAccept_clicked();

    void on_editTimetableChoose_currentIndexChanged(int index);

    void on_editTimetableChoose_editTextChanged(const QString &arg1);

    void on_editTimetableChoose_activated(int index);

    void on_calendarStack_currentChanged(int index);

    void on_calendarNext_clicked();

    void on_calendarBack_clicked();

    void on_calendarGroup_currentIndexChanged(int index);

    void on_calendarPerson_activated(int index);

    void on_calendarPerson_currentIndexChanged(int index);

    void on_editTimetableAddCondition_clicked();

    void on_editTimetableGenerate_clicked();

    void on_calendarGroup_activated(int index);

    void on_yearContent_cellChanged(int row, int column);

    void on_quarterContent_cellChanged(int row, int column);

    void on_calendarDay0_cellChanged(int row, int column);

    void on_calendarDay1_cellChanged(int row, int column);

    void on_calendarDay2_cellChanged(int row, int column);

    void on_calendarDay3_cellChanged(int row, int column);

    void on_calendarDay4_cellChanged(int row, int column);

    void on_calendarDay5_cellChanged(int row, int column);

private:
    Ui::MainWindow *ui;

    QTcpSocket * socket;
    void slotReadyRead();

    QVector<PostWidget *> postsOpened;
    QVector<ChatWidget *> chats;

    QVector<User> knownUsers;
    QVector<User> ownedStudents;

    QVector<QPair<int, QString>> parents;

    User currentProfile;

    char * occuipedLiters = nullptr;

    void onCalendarChanged(int row, int column, QTableWidget * day, int dayId);
};

#endif // MAINWINDOW_H
