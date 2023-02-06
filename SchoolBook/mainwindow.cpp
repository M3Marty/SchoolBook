#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <receiver.h>

#include <QT>

#include <QTcpSocket>

#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QButtonGroup>

#include <timetableconditionwidget.h>

#define GENERAL_INCLUDE
#define REQUEST_WRITE
#define CID_EXTERN
#include <sb_master.h>

// From main.cpp
QTcpSocket * getLoginSocket();
void execApp();

QString MainWindow::USER_NAME;
QString MainWindow::USER_SURNAME;
int MainWindow::USER_ROLE;
int MainWindow::USER_ID;
QString MainWindow::USER_GROUP;

MainWindow * MainWindow::MAIN = nullptr;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), currentProfile(USER_ID, USER_ROLE, USER_NAME, USER_SURNAME, USER_GROUP)
{
    ui->setupUi(this);
    ui->toolsTeacherButton->setVisible(false);

    MainWindow::MAIN = this;

    this->socket = getLoginSocket();

    disconnect(this->socket, nullptr, nullptr, nullptr);
    connect(this->socket, &QTcpSocket::readyRead, this, &MainWindow::slotReadyRead);
    connect(this->socket, &QTcpSocket::disconnected, this, &QTcpSocket::deleteLater);

    this->lastPostId = -1;
    this->timetables = QVector<QMap<QString, Timetable *>>(11);

    this->addKnownUser(User(USER_ID, USER_ROLE, USER_NAME, USER_SURNAME, USER_GROUP));
    if (USER_ID)
        this->addKnownUser(User(0, 0, "root", "user", ""));

    on_homeTB_clicked();

    ui->topperUserName->setText(MainWindow::USER_NAME + " " + MainWindow::USER_SURNAME);

    QButtonGroup * newsFor = new QButtonGroup();
    newsFor->addButton(ui->newsForAll, 0);
    newsFor->addButton(ui->newsForTeachers, 1);
    newsFor->addButton(ui->newsForLabors, 2);
    newsFor->addButton(ui->newsForStudents, 3);
    newsFor->addButton(ui->newsForChoosed, 4);

    ui->calendarStack->setCurrentIndex(0);

    if (USER_ROLE > 2) {
        ui->calendarDay0->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->calendarDay1->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->calendarDay2->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->calendarDay3->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->calendarDay4->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->calendarDay5->setEditTriggers(QAbstractItemView::NoEditTriggers);

        ui->yearContent->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->quarterContent->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }

    ui->toolsStack->setCurrentIndex(1);
    ui->getCodeRole->setCurrentIndex(4);
    if (USER_ROLE > 0) {
        ui->toolsLessonsButton->setVisible(false);

        ui->getCodeRole->setItemData(0, 0, 0x0100 - 1);
        ui->getCodeRole->setItemData(1, 0, 0x0100 - 1);
    }
    if (USER_ROLE > 1) {
        ui->toolsTeacherButton->setVisible(false);
        ui->toolstTimetableButton->setVisible(false);
        ui->toolsClassButton->setVisible(false);

        ui->createClassTeacher->setEnabled(false);

        ui->getCodeRole->setItemData(2, 0, 0x0100 - 1);
    }
    if (USER_ROLE > 2) {
        ui->toolsCreateClassButton->setVisible(false);

        ui->getCodeRole->setItemData(3, 0, 0x0100 - 1);
    }
    if (USER_ROLE > 3) {
        ui->toolsGetCodeButton->setVisible(false);

        ui->getCodeButton->setEnabled(false);
    }

    REQUEST

    WRITE_CHAR((2 + (USER_ROLE < 2? 1: 0) + (USER_ROLE < 3? 1:0)));

    WRITE_CHAR(cid_getMyGroups);
    WRITE_CHAR(cid_getLessonsList);

    if (USER_ROLE < 2)
        WRITE_CHAR(cid_getMyTeachers);

    if (USER_ROLE < 3)
        WRITE_CHAR(cid_getMyStudents);

    send(requestData);
}

MainWindow::~MainWindow()
{
    delete ui;
}


bool MainWindow::knowUser(int id) {
    return getUser(id) != nullptr;
}

User * MainWindow::getUser(int id)
{
    for (int i = 0; i < knownUsers.length(); i++)
        if (knownUsers.at(i).id == id)
            return &knownUsers[i];

    return nullptr;
}

User * MainWindow::getUser(QStringList nameSurname, int defaultReturn) {
    for (int i = 0; i < knownUsers.length(); i++)
        if (knownUsers[i].name == nameSurname[0] && knownUsers[i].surname == nameSurname[1])
            return &knownUsers[i];

    if (defaultReturn == -1)
        return nullptr;
    else
        return &knownUsers[defaultReturn];
}

void MainWindow::goProfile(int id) {
    if (id == 0) {
        currentProfile = User(USER_ID, USER_ROLE, USER_NAME, USER_SURNAME, USER_GROUP);
    }
    else if (knowUser(id)) {
        for (int i = 0; i < knownUsers.length(); i++)
            if (knownUsers[i].id == id) {
                currentProfile = knownUsers[i];
                break;
            }
    } else {
        REQUEST
        request << (char) 1 << (char) 9 << id;
        send(requestData);
        ui->mainStack->setCurrentIndex(3);
        return;
    }

    auto c = ui->profilePosts->layout()->children();
    for (auto i:c)
        ui->profilePosts->layout()->removeWidget(static_cast<QWidget *>(i));

    setProfileViewData(currentProfile.name, currentProfile.surname, currentProfile.role, currentProfile.className);
    ui->mainStack->setCurrentIndex(3);
}


void MainWindow::deletePost(int id) {
    Q_UNUSED(id);
}

void MainWindow::addPost(Post post, int where) {
    PostWidget * widget = new PostWidget(&post);
    int index = where == -1? ui->newsContent->children().length() - 2: where;
    ((QVBoxLayout *) ui->newsContent->layout())->insertWidget(index, widget);
    widget->show();
    this->postsOpened.push_back(widget);
}

void MainWindow::addChat(int id, QString name) {
    ChatWidget * w = new ChatWidget(id, name, "Нет сообщений", 0);
    static_cast<QBoxLayout *>(ui->messageUserContent->layout())->insertWidget(0, w);
    w->show();
    chats.push_back(w);
}

void MainWindow::updateChat(int id, QString lastMessage, long long lastTimestamp) {
    for (int i = 0; i < chats.length(); i++)
        if (chats.at(i)->id == id) {
            chats.at(i)->update(lastMessage, lastTimestamp);
            break;
        }
}

void MainWindow::addGroupToLead(QString name) {
    ui->newsCreateChooseClass->addItem(name);
    leadGroups.push_back(name);
    if (name.startsWith("Класс"))
        ui->calendarGroup->addItem(name);
}

void MainWindow::addGroupToMember(QString name) {
    memberGroups.push_back(name);
    if (name.startsWith("Класс"))
        ui->calendarGroup->addItem(name);
}

void MainWindow::addGroupToDisplay(QString name, QVector<int> members) {
    GroupWidget * w = new GroupWidget(name, members);

    ((QVBoxLayout *) ui->groups->layout())->insertWidget(
                ui->groups->children().length() - 1, w);

    groupMembers.insert(name, members.sliced(1));
    groupLeader.insert(name, members[0]);
    groupWidgets.insert(name, w);
}

void MainWindow::setProfileViewData(QString name, QString surname, char role, QString groupName) {
    switch (role) {
    case 0: groupName = "Администратор"; break;
    case 1: groupName = "Завуч"; break;
    case 2: groupName = "Учитель " + groupName; break;
    case 3: groupName = "Ученик " + groupName; break;
    case 4: groupName = "Родитель " + groupName; break;
    }

    ui->profileTitle->setText(name + " " + surname + " (" + groupName + ")");
}

void MainWindow::setGetCode(QString code) {
    ui->getCodeResult->setText(code);
}

void MainWindow::setOccuipedLiters(char * numbers) {
    if (occuipedLiters)
        delete occuipedLiters;
    occuipedLiters = numbers;
    ui->createClassNumber->setCurrentIndex(1);
    ui->createClassNumber->setCurrentIndex(0);
}

void MainWindow::addKnownUser(User user) {
    if (!knowUser(user.id))
        knownUsers.push_back(user);
}

void MainWindow::addKnownUser(int id, int role, QString name, QString surname) {
    addKnownUser(User(id, role, name, surname));
}

void MainWindow::addOwnedTeacher(int id, int role, QString name, QString surname, QString groupName) {
    User u(id, role, name, surname, groupName);
    addKnownUser(u);
    ownedTeachers.push_back(u);
}

void MainWindow::addOwnedStudent(int id, int role, QString name, QString surname, QString groupName) {
    User u(id, role, name, surname, groupName);
    addKnownUser(u);
    ownedStudents.push_back(u);

    ui->parentsChoose->addItem(u.toString());
}

void MainWindow::addLesson(QString lesson) {
    if (lesson.length())
        ui->lessonsList->addItem(lesson);
    lessons.append(lesson);
}

void MainWindow::clearTasks() {
    for (auto i:ui->newTaskList->children())
        ui->newTaskList->removeWidget(static_cast<QWidget *>(i));
    for (auto i:ui->workTaskList->children())
        ui->workTaskList->removeWidget(static_cast<QWidget *>(i));
    for (auto i:ui->completeTaskList->children())
        ui->completeTaskList->removeWidget(static_cast<QWidget *>(i));
}

void MainWindow::addTask(int id, int authorId, QString name, QString text, char status) {
    auto parent = ui->newTaskList;
    switch (status) {
    case 1:
        parent = ui->workTaskList;
        break;
    case 2:
        parent = ui->completeTaskList;
        break;
    }

    parent->insertWidget(0, new TaskWidget(id, authorId, name, text, status));
}

void MainWindow::moveTask(TaskWidget * w, char from, char to) {
    auto parent = ui->newTaskList;
    switch (to) {
    case 1:
        parent = ui->workTaskList;
        break;
    case 2:
        parent = ui->completeTaskList;
        break;
    }

    auto fromP = ui->newTaskList;
    switch (from) {
    case 1:
        fromP = ui->workTaskList;
        break;
    case 2:
        fromP = ui->completeTaskList;
        break;
    }

    fromP->removeWidget(w);
    parent->insertWidget(0, w);
}

void MainWindow::setTeacherOfLessonsForClass(QStringList list) {
    ui->classesToList->clear();
    ui->classesToList->addItems(list);
}

void MainWindow::setParents(QVector<QPair<int, QString>> parents) {
    this->parents = parents;

    ui->parentsList->clear();
    for (int i = 0; i < parents.length(); i++)
        ui->parentsList->addItem(parents[i].second);
}

void MainWindow::clearYearTimetable(QVector<int> l)
{
    orderedLessons = l;

    while (ui->yearContent->rowCount())
        ui->yearContent->removeRow(0);

    for (auto lId : l) {
        ui->yearContent->insertRow(ui->yearContent->rowCount());
        ui->yearContent->setItem(ui->yearContent->rowCount() - 1, 0, new QTableWidgetItem(lessons[lId]));
        ui->quarterContent->item(ui->quarterContent->rowCount() - 1, 0)->setFlags(Qt::ItemIsEnabled);
    }
}

void MainWindow::clearQuarterTimetable(QVector<int> l)
{
    orderedLessons = l;

    while (ui->quarterContent->rowCount() != 1)
        ui->quarterContent->removeRow(1);

    while (ui->quarterContent->columnCount() != 2)
        ui->quarterContent->removeColumn(1);

    for (auto lId : l) {
        ui->quarterContent->insertRow(ui->quarterContent->rowCount());
        ui->quarterContent->setItem(ui->quarterContent->rowCount() - 1, 0, new QTableWidgetItem(lessons[lId]));
        ui->quarterContent->item(ui->quarterContent->rowCount() - 1, 0)->setFlags(Qt::ItemIsEnabled);
    }
}

void MainWindow::clearWeekTimetable()
{
    ui->calendarDay0->clearContents();
    ui->calendarDay1->clearContents();
    ui->calendarDay2->clearContents();
    ui->calendarDay3->clearContents();
    ui->calendarDay4->clearContents();
    ui->calendarDay5->clearContents();
}

bool editTimetable = false;

void MainWindow::addYearTimetableValue(int userId, int lessonId, int mark, int column)
{
    editTimetable = true;
    Q_UNUSED(userId)
    this->ui->yearContent->setItem(this->orderedLessons.indexOf(lessonId), column, new QTableWidgetItem(QString::number(mark)));
    editTimetable = false;
}

void MainWindow::addQuarterTimetableValue(int userId, int lessonId, int mark, int dayOrQuarter)
{
    editTimetable = true;
    Q_UNUSED(userId)
    if (dayOrQuarter == 0)
        this->ui->quarterContent->setItem(orderedLessons.indexOf(lessonId) + 1, ui->quarterContent->columnCount() - 1, new QTableWidgetItem(QString::number(mark)));
    else if (this->ui->quarterContent->columnCount() == 2 || this->ui->quarterContent->item(0, 1)->text().toInt() > dayOrQuarter) {
        ui->quarterContent->insertColumn(1);
        ui->quarterContent->setItem(0, 1, new QTableWidgetItem(QString::number(dayOrQuarter)));
        ui->quarterContent->item(0, 1)->setFlags(Qt::ItemIsEnabled);
        ui->quarterContent->setItem(orderedLessons.indexOf(lessonId) + 1, 1, new QTableWidgetItem(QString::number(mark)));
    }
    else if (this->ui->quarterContent->item(0, ui->quarterContent->columnCount() - 2)->text().toInt() < dayOrQuarter) {
        ui->quarterContent->insertColumn(ui->quarterContent->columnCount() - 1);
        ui->quarterContent->setItem(0, ui->quarterContent->columnCount() - 2, new QTableWidgetItem(QString::number(dayOrQuarter)));
        ui->quarterContent->item(0, ui->quarterContent->columnCount() - 2)->setFlags(Qt::ItemIsEnabled);
        ui->quarterContent->setItem(orderedLessons.indexOf(lessonId) + 1, ui->quarterContent->columnCount() - 2, new QTableWidgetItem(QString::number(mark)));
    }
    else
        for (int i = 1; i < this->ui->quarterContent->columnCount() - 1; i++) {
            int columnDay = ui->quarterContent->item(0, i)->text().toInt();
            if (columnDay == dayOrQuarter) {
                ui->quarterContent->setItem(orderedLessons.indexOf(lessonId) + 1, i, new QTableWidgetItem(QString::number(mark)));
                break;
            }
            else if (columnDay > dayOrQuarter) {
                ui->quarterContent->insertColumn(i - 1);
                ui->quarterContent->setItem(0, i - 1, new QTableWidgetItem(QString::number(dayOrQuarter)));
                ui->quarterContent->item(0, i - 1)->setFlags(Qt::ItemIsEnabled);
                ui->quarterContent->setItem(orderedLessons.indexOf(lessonId) + 1, i - 1, new QTableWidgetItem(QString::number(mark)));
                break;
            }
        }
    editTimetable = false;
}

void MainWindow::addWeekTimetableValue(QVector<int> lessonsList, QStringList tasks, int dayId)
{
    editTimetable = true;

    QTableWidget * day;
    switch (dayId) {
    case 0: day = ui->calendarDay0; break;
    case 1: day = ui->calendarDay1; break;
    case 2: day = ui->calendarDay2; break;
    case 3: day = ui->calendarDay3; break;
    case 4: day = ui->calendarDay4; break;
    case 5: day = ui->calendarDay5; break;
    }

    for (int i = 0; i < lessonsList.length(); i++) {
        day->setItem(i, 0, new QTableWidgetItem(this->lessons[lessonsList[i]]));
        if (day->item(i, 0))
            day->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        day->setItem(i, 2, new QTableWidgetItem(tasks[i]));
        if (ui->calendarPerson->currentIndex() == 0 && day->item(i, 2))
            day->item(i, 2)->setFlags(Qt::ItemIsEnabled);
    }

    editTimetable = false;
}

void MainWindow::addWeekMark(int userId, int lessonId, int mark, int dayId)
{
    Q_UNUSED(userId)
    editTimetable = true;

    QTableWidget * day;
    switch (dayId) {
    case 0: day = ui->calendarDay0; break;
    case 1: day = ui->calendarDay1; break;
    case 2: day = ui->calendarDay2; break;
    case 3: day = ui->calendarDay3; break;
    case 4: day = ui->calendarDay4; break;
    case 5: day = ui->calendarDay5; break;
    default:
        return;
    }

    for (int i = 0; i < 8; i++) {
        auto item = day->item(i, 0);
        qDebug() << item;
        if (!item)
            break;

        qDebug() << lessonId << mark << dayId << i;

        if (lessons[lessonId] == item->text()) {
            qDebug() << "in";
            auto markItem = day->item(i, 1);
            if (!markItem || !markItem->text().length()) {
                day->setItem(i, 1, new QTableWidgetItem(QString::number(mark)));
                break;
            }

        }
    }


    editTimetable = false;
}


void MainWindow::send(QByteArray & data) {
    qDebug() << "Send " << data.size() << "bytes";
    this->socket->write(data);
}

void MainWindow::slotReadyRead() {
    QDataStream in(socket);
    in.setVersion(QDataStream::Version::Qt_6_4);
    qDebug() << "Answer";

    if (in.status() == QDataStream::Status::Ok) {
        Receiver::receive(in);
        socket->readAll();
    } else {
        qDebug() << "DataStream error";
    }
}


void MainWindow::on_homeTB_clicked()
{
    ui->mainStack->setCurrentIndex(0);
    ui->newsStack->setCurrentIndex(0);
    ui->goNewPost->setVisible(true);

    if (MainWindow::USER_ROLE > 2)
        ui->goNewPost->setVisible(false);
}


void MainWindow::on_goNewPost_clicked()
{
    ui->newsStack->setCurrentIndex(1);
    ui->goNewPost->setVisible(false);

    ui->newsForTeachers->setEnabled(true);
    ui->newsForLabors->setEnabled(true);

    if (MainWindow::USER_ROLE != 1)
        ui->newsForLabors->setEnabled(false);
    if (MainWindow::USER_ROLE > 1)
        ui->newsForTeachers->setEnabled(false);
}

void MainWindow::on_newsChooseImageB_clicked()
{
    ui->newsImages->addItems(QFileDialog::getOpenFileNames(
                this, tr("Выберите изображение"), "",
                tr("Image Files (*.png *.jpg *.bmp)")));
}

void MainWindow::on_newsRemoveImageB_clicked()
{
    qDeleteAll(ui->newsImages->selectedItems());
}

void MainWindow::on_newsCreateB_clicked()
{
    QString title = ui->newsCreateTitle->text();
    QString text = ui->newsCreateText->toPlainText();
    // TODO: доделать передачу изображений
    QStringList images;
    for(int i = 0; i < ui->newsImages->count(); i++)
        images << ui->newsImages->item(i)->text();

    char group = ui->newsForAll->group()->checkedId();
    QString groupSpecific = ui->newsCreateChooseClass->currentText();


    REQUEST

    WRITE_CHAR((1 + images.length()));

    for (int i = 0; i < images.length(); i++) {
        // Протокол передачи файлов: 4 байта размера и данные
        QString imageName = images.at(i);
        QFile image(images.at(i));
        QByteArray imageRaw = image.read(image.size());

        WRITE_CHAR(cid_loadNewFile);
        WRITE_DYNAMIC_STRING(imageName, char);
        WRITE_INT(image.size());

        // Очередное исключение
        request.writeRawData(imageRaw.data(), image.size());
    }

    WRITE_CHAR(cid_newPost);

    INIT_STATIC_STRING_WRITE(TITLE_LENGTH)
            WRITE_STATIC_STRING(title, TITLE_LENGTH);

    // Передаём данные о группе
    request << group;
    if (group == 4) {
        // Если это отдельная выбранная группа, то передаём её название
        WRITE_STATIC_STRING(groupSpecific, GROUP_LENGTH);
    }

    // Передаём строки перенной длины
    WRITE_DYNAMIC_STRING(text, short);

    QString imagesNames("");
    for (int i = 0; i < images.length(); i++)
        imagesNames += images.at(i) + ";";

    WRITE_DYNAMIC_STRING(imagesNames, short);

    send(requestData);

    // Очищаем поля ввода
    ui->newsCreateTitle->setText("");
    ui->newsCreateText->setPlainText("");
    ui->newsImages->clear();

    // Возвращаемся на стену с постами
    addPost(Post(title, text, QDateTime::currentMSecsSinceEpoch(),
             USER_NAME + " " + USER_SURNAME, USER_ID, 0), 0);
    on_homeTB_clicked();
}


void MainWindow::on_calendarTB_clicked()
{
    ui->mainStack->setCurrentIndex(1);
    ui->calendarGroup->activated(0);
}

void MainWindow::on_groupTB_clicked()
{
    ui->mainStack->setCurrentIndex(2);
}

void MainWindow::on_goProfile_clicked()
{
    goProfile(0);

    ui->mainStack->setCurrentIndex(3);

    REQUEST

    WRITE_CHAR(1);
    WRITE_CHAR(cid_getTasks);

    send(requestData);
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    Q_UNUSED(index)
}


void MainWindow::on_toolsTeacherButton_clicked()
{
    ui->toolsStack->setCurrentIndex(0);
}

void MainWindow::on_toolsParentButton_clicked()
{
    ui->toolsStack->setCurrentIndex(1);
}

void MainWindow::on_toolsGetCodeButton_clicked()
{
    ui->toolsStack->setCurrentIndex(2);
}

void MainWindow::on_toolsLessonsButton_clicked()
{
    ui->toolsStack->setCurrentIndex(3);
}

void MainWindow::on_toolstTimetableButton_clicked()
{
    ui->toolsStack->setCurrentIndex(4);
    ui->timetableStack->setCurrentIndex(0);
    on_editTimetableNumber_currentIndexChanged(0);
}

void MainWindow::on_toolsClassButton_clicked()
{
    ui->toolsStack->setCurrentIndex(5);
    ui->classesChoose->setCurrentIndex(0);
    on_classesChoose_currentIndexChanged(0);
}

void MainWindow::on_toolsCreateClassButton_clicked()
{
    ui->toolsStack->setCurrentIndex(6);

    ui->createClassTeacher->clear();
    for (int i = 0; i < ownedTeachers.length(); i++)
        ui->createClassTeacher->addItem(ownedTeachers[i].toString());
    if (USER_ROLE == 1)
        ui->createClassTeacher->addItem(USER_NAME + " " + USER_SURNAME);

    REQUEST

    WRITE_CHAR(1);
    WRITE_CHAR(cid_occuipedLiters);

    send(requestData);
}


void MainWindow::on_getCodeRole_currentIndexChanged(int index)
{
    switch (index) {
    case 0:
    case 1:
        ui->getCodeSubdata->setEnabled(false);
        ui->getCodeSubdata->clear();
        break;
    case 2:
        ui->getCodeSubdata->clear();
        if (USER_ROLE == 0) {
            ui->getCodeSubdata->setEnabled(true);
            for (int i = 0; i < leadGroups.length(); i++)
                if (leadGroups[i].startsWith("Уч. "))
                    ui->getCodeSubdata->addItem(leadGroups[i]);
        } else {
            ui->getCodeSubdata->setEnabled(false);
        }
        break;
    case 3:
        ui->getCodeSubdata->setEnabled(true);
        ui->getCodeSubdata->clear();
        for (int i = 0; i < leadGroups.length(); i++)
            if (leadGroups[i].startsWith("Класс"))
                ui->getCodeSubdata->addItem(leadGroups[i]);
        break;
    case 4:
        ui->getCodeSubdata->clear();
        if (USER_ROLE == 3) {
            ui->getCodeSubdata->setEnabled(false);
        } else {
            ui->getCodeSubdata->setEnabled(true);
            for (int i = 0; i < ownedStudents.length(); i++)
                ui->getCodeSubdata->addItem(ownedStudents[i].toString());
            break;
        }

        break;
    }
}

void MainWindow::on_getCodeButton_clicked()
{
    REQUEST

    WRITE_CHAR(1);

    WRITE_CHAR(cid_getCode);
    INIT_STATIC_STRING_WRITE(NAME_SURNAME_LENGTH)
            WRITE_STATIC_STRING(ui->getCodeName->text(), NAME_LENGTH);
            WRITE_STATIC_STRING(ui->getCodeSurname->text(), SURNAME_LENGTH);
            WRITE_CHAR(ui->getCodeRole->currentIndex());

    QString subdata = ui->getCodeSubdata->currentText();
    if (ui->getCodeRole->currentIndex() == 4) {
        if (USER_ROLE == 3) {
            subdata = QString::number(USER_ID);
        } else {
            for (int i = 0; i < ownedStudents.length(); i++)
                if (ownedStudents[i].toString() == subdata) {
                    subdata = QString::number(ownedStudents[i].id);
                    break;
                }
        }
    }

    WRITE_DYNAMIC_STRING(subdata, char);

    send(requestData);

    ui->getCodeName->setText("");
    ui->getCodeSurname->setText("");
}


void MainWindow::on_createClassNumber_currentIndexChanged(int index)
{
    if (occuipedLiters)
        ui->createClassLitera->setText(QString("АБВГДЕЖЗИКЛМНО").at(occuipedLiters[index]));
}

void MainWindow::on_createClassButton_clicked()
{
    char classNumber = ui->createClassNumber->currentIndex() + 1;
    leadGroups.append("Класс " + QString::number(classNumber) + " \"" + ui->createClassLitera->toPlainText() + "\"");
    char classLiteraId = QString("АБВГДЕЖЗИКЛМНО").indexOf(ui->createClassLitera->toPlainText());
    int teacherId;

    occuipedLiters[classNumber - 1]++;
    ui->createClassNumber->setCurrentIndex(0);
    ui->createClassNumber->setCurrentIndex(1);
    ui->createClassNumber->setCurrentIndex(classNumber - 1);

    if (USER_ROLE == 2)
        teacherId = USER_ID;
    else {
        if (ui->createClassTeacher->currentIndex() == ownedTeachers.length())
            teacherId = USER_ID;
        else
            teacherId = ownedTeachers[ui->createClassTeacher->currentIndex()].id;
    }


    REQUEST

    WRITE_CHAR(1);
    WRITE_CHAR(cid_createClass);
    WRITE_CHAR(classNumber);
    WRITE_CHAR(classLiteraId);
    WRITE_INT(teacherId);

    send(requestData);
}


void MainWindow::on_classesChoose_currentIndexChanged(int index)
{
    if (leadGroups.length() < 2) {
        ui->classesToChoose->setEnabled(false);
        ui->classesFromChoose->setEnabled(false);
        ui->classesFrom2ToButton->setEnabled(false);
        ui->classesTo2FromButton->setEnabled(false);
    } else {
        ui->classesToChoose->setEnabled(true);
        ui->classesFromChoose->setEnabled(true);
        ui->classesFrom2ToButton->setEnabled(true);
        ui->classesTo2FromButton->setEnabled(true);
    }

    ui->classesFromChoose->clear();
    ui->classesToChoose->clear();
    ui->classesFromList->clear();
    ui->classesToList->clear();
    QStringList classes = leadGroups.filter("Класс");
    switch (index) {
    case 0:
        ui->classesFromChoose->setEnabled(true);
        ui->classesFromChoose->addItems(classes);
        ui->classesToChoose->addItems(classes);
        ui->classesTo2FromButton->setVisible(true);
        ui->classesFrom2ToButton->setVisible(false);
        break;
    case 1:
    case 2:
        ui->classesFromChoose->setEnabled(false);
        ui->classesTo2FromButton->setVisible(false);
        ui->classesFrom2ToButton->setVisible(true);
        for (auto i : ownedTeachers)
            ui->classesFromList->addItem(i.toString());
        ui->classesToChoose->addItems(classes);
        break;
    case 3:
        for (int i = 0; i < 11; i++)
            ui->classesFromChoose->addItems(timetables[i].keys());
        ui->classesTo2FromButton->setVisible(false);
        ui->classesToChoose->addItems(classes);
    }
    ui->classesFromChoose->setCurrentIndex(0);
    ui->classesToChoose->setCurrentIndex(0);
    on_classesFromChoose_currentIndexChanged(0);
    on_classesToChoose_currentIndexChanged(0);
}

void MainWindow::on_classesToChoose_currentIndexChanged(int index)
{
    if (ui->classesChoose->currentIndex() == 0) {
        ui->classesToList->clear();
        ui->classesFrom2ToButton->setVisible(true);
        for (int i = 0; i < ownedStudents.length(); i++) {
            if (ownedStudents[i].className == ui->classesToChoose->currentText())
                ui->classesToList->addItem(ownedStudents[i].toString());
        }
    } else if (ui->classesChoose->currentIndex() == 2) {
        REQUEST

        INIT_STATIC_STRING_WRITE(GROUP_LENGTH)
                WRITE_CHAR(1);
                WRITE_CHAR(cid_teachersOfLessonsForClass);
                WRITE_STATIC_STRING(ui->classesFromChoose->currentText(), GROUP_LENGTH);

        send(requestData);
    }
}

void MainWindow::on_classesFromChoose_currentIndexChanged(int index)
{
     if (ui->classesChoose->currentIndex() == 0) {
         ui->classesFromList->clear();
         for (int i = 0; i < ownedStudents.length(); i++) {
             if (ownedStudents[i].className == ui->classesFromChoose->currentText())
                 ui->classesFromList->addItem(ownedStudents[i].toString());
         }
     }
}

void MainWindow::on_classesTo2FromButton_clicked()
{
    if (ui->classesToChoose->currentIndex() != ui->classesFromChoose->currentIndex() &&
            ui->classesToList->selectedItems().length()) {

        auto selected = ui->classesToList->selectedItems()[0];
        User * transfer = this->getUser(selected->text().split(" "));
        transfer->className = ui->classesFromChoose->currentText();
        ui->classesFromList->addItem(selected->text());
        delete selected;

        REQUEST

        INIT_STATIC_STRING_WRITE(GROUP_LENGTH)
            WRITE_CHAR(1);
            WRITE_CHAR(cid_transferStudent);
            WRITE_INT(transfer->id);
            WRITE_STATIC_STRING(ui->classesFromChoose->currentText(), GROUP_LENGTH);

        send(requestData);
    }
}

void MainWindow::on_classesFrom2ToButton_clicked()
{
    if (!ui->classesFromList->selectedItems().length() && ui->classesChoose->currentIndex() != 3)
        return;

    REQUEST
    WRITE_CHAR(1);

    INIT_STATIC_STRING_WRITE(GROUP_LENGTH)
    switch (ui->classesChoose->currentIndex()) {
    case 1:
        if (!ui->classesToList->selectedItems().length())
            return;

        WRITE_CHAR(cid_setTeacherOfLessonForClass);
        WRITE_STATIC_STRING(ui->classesToChoose->currentText(), GROUP_LENGTH);
        WRITE_INT(lessons.indexOf(ui->classesToList->selectedItems()[0]->text().split(" ")[0]));
        WRITE_INT(getUser(ui->classesFromList->selectedItems()[0]->text().split(" "))->id);

        ui->classesToList->selectedItems()[0]->setText(
                    ui->classesToList->selectedItems()[0]->text().split(" ")[0] + " (" +
                    ui->classesFromList->selectedItems()[0]->text() + ")");
        break;
    case 2:
        WRITE_CHAR(cid_setMasterTeacher);
        WRITE_STATIC_STRING(ui->classesToChoose->currentText(), GROUP_LENGTH);
        WRITE_INT(getUser(ui->classesFromList->selectedItems()[0]->text().split(" "))->id);

        break;
    case 3:
        WRITE_CHAR(cid_setTimetable);
        WRITE_STATIC_STRING(ui->classesToChoose->currentText(), GROUP_LENGTH);
        WRITE_STATIC_STRING(ui->classesFromChoose->currentText(), GROUP_LENGTH);
        break;
    }

    send(requestData);
}

void MainWindow::on_classesToList_itemDoubleClicked(QListWidgetItem *item)
{
    if (ui->classesChoose->currentIndex() == 0) {
        goProfile(getUser(item->text().split(" "))->id);
    }
}

void MainWindow::on_classesFromList_itemDoubleClicked(QListWidgetItem *item)
{
    goProfile(getUser(item->text().split(" "))->id);
}

void MainWindow::on_classesToChoose_activated(int index)
{
    if (ui->classesChoose->currentIndex() == 1) {
        REQUEST
        WRITE_CHAR(1);
        WRITE_CHAR(cid_teachersOfLessonsForClass);
        INIT_STATIC_STRING_WRITE(GROUP_LENGTH)
        WRITE_STATIC_STRING(ui->classesToChoose->itemText(index), GROUP_LENGTH);
        send(requestData);
    }
}


void MainWindow::on_lessonsList_currentTextChanged(const QString &currentText)
{
    Q_UNUSED(currentText)
}

void MainWindow::on_lessonsAdd_clicked()
{
    if (ui->lessonsAddField->text().length()) {
        ui->lessonsList->addItem(ui->lessonsAddField->text());
        REQUEST
        WRITE_CHAR(1);
        WRITE_CHAR(cid_editLesson);
        WRITE_CHAR(0);
        INIT_STATIC_STRING_WRITE(NAME_LENGTH)
                WRITE_STATIC_STRING(ui->lessonsAddField->text(), NAME_LENGTH);

        send(requestData);

        lessons.append(ui->lessonsAddField->text());
        ui->lessonsAddField->setText("");
    }
}

void MainWindow::on_lessonsDelete_clicked()
{
    if (ui->lessonsList->selectedItems().length()) {
        REQUEST
        WRITE_CHAR(ui->lessonsList->selectedItems().length());
        INIT_STATIC_STRING_WRITE(NAME_LENGTH)
        for (auto i : ui->lessonsList->selectedItems()) {
            WRITE_CHAR(cid_editLesson);
            WRITE_CHAR(5);
            WRITE_STATIC_STRING(i->text(), NAME_LENGTH);
        }

        send(requestData);

        ui->lessonsList->selectedItems()[0]->setText("");
    }
}

void MainWindow::on_lessonsRename_clicked()
{
    if (ui->lessonsAddField->text().length() && ui->lessonsList->selectedItems().length()) {
        REQUEST
        WRITE_CHAR(1);
        WRITE_CHAR(cid_editLesson);
        WRITE_CHAR(4);
        INIT_STATIC_STRING_WRITE(NAME_LENGTH)
                WRITE_STATIC_STRING(ui->lessonsList->selectedItems()[0]->text(), NAME_LENGTH);
                WRITE_STATIC_STRING(ui->lessonsAddField->text(), NAME_LENGTH);

        send(requestData);

        ui->lessonsList->selectedItems()[0]->setText(ui->lessonsAddField->text());
        ui->lessonsAddField->setText("");
    }
}


void MainWindow::on_parentsChoose_activated(int index)
{
    REQUEST

    WRITE_CHAR(1);
    WRITE_CHAR(cid_getParents);
    WRITE_INT(ownedStudents[index].id);

    send(requestData);
}

void MainWindow::on_parentsList_itemDoubleClicked(QListWidgetItem *item)
{
    for (int i = 0; i < parents.length(); i++)
        if (item->text() == parents[i].second) {
            goProfile(parents[i].first);
            break;
        }
}


void MainWindow::on_pushButton_clicked()
{
    REQUEST

    WRITE_CHAR(1);

    if (lastPostId == -1) {
        WRITE_CHAR(cid_getLastId);
        WRITE_CHAR(0); // Поста
        WRITE_CHAR(1); // Сквозной байт подмода, вернётся обратно в обработчик
        // В данном случае он просит сделать ещё один запрос,
        // чтобы получить посты, после чего следующий обработчик их отобразит.
    } else {
        WRITE_CHAR(cid_getPosts);
        WRITE_INT(lastPostId);
        WRITE_CHAR(5);
    }

    send(requestData);
}


void MainWindow::on_editTimetableNumber_activated(int index)
{
    Q_UNUSED(index)
}

void MainWindow::on_editTimetableNumber_currentIndexChanged(int index)
{
    if (timetables[index].size()) {
        ui->editTimetableChoose->clear();
        ui->editTimetableChoose->addItems(timetables[index].keys());
        ui->editTimetableChoose->activated(0);
    } else {
        REQUEST

        WRITE_CHAR(1);
        WRITE_CHAR(cid_getTimetables);
        WRITE_CHAR(index);

        send(requestData);
    }
}

void MainWindow::on_editTimetableAccept_clicked()
{
    REQUEST

    INIT_STATIC_STRING_WRITE(GROUP_LENGTH)
            WRITE_CHAR(1);
            WRITE_CHAR(cid_editTimetable);
            WRITE_CHAR((ui->editTimetableNumber->currentIndex() + 1));
            WRITE_STATIC_STRING(ui->editTimetableChoose->currentText(), GROUP_LENGTH);

    for (int i = 0; i < 8; i++) {
        auto e = ui->editTimetableDay0->item(i);
        int j = lessons.indexOf(e->text());
        if (j != -1) {
            WRITE_INT(j);

        }
        else {
            QMessageBox msgBox;
            msgBox.setText("Нет урока " + e->text() + ".");
            msgBox.exec();
            return;
        }
    }
    for (int i = 0; i < 8; i++) {
        auto e = ui->editTimetableDay1->item(i);
        int j = lessons.indexOf(e->text());
        if (j != -1)
            WRITE_INT(j);
        else {
            QMessageBox msgBox;
            msgBox.setText("Нет урока " + e->text() + ".");
            msgBox.exec();
            return;
        }
    }
    for (int i = 0; i < 8; i++) {
        auto e = ui->editTimetableDay2->item(i);
        int j = lessons.indexOf(e->text());
        if (j != -1)
            WRITE_INT(j);
        else {
            QMessageBox msgBox;
            msgBox.setText("Нет урока " + e->text() + ".");
            msgBox.exec();
            return;
        }
    }
    for (int i = 0; i < 8; i++) {
        auto e = ui->editTimetableDay3->item(i);
        int j = lessons.indexOf(e->text());
        if (j != -1)
            WRITE_INT(j);
        else {
            QMessageBox msgBox;
            msgBox.setText("Нет урока " + e->text() + ".");
            msgBox.exec();
            return;
        }
    }
    for (int i = 0; i < 8; i++) {
        auto e = ui->editTimetableDay4->item(i);
        int j = lessons.indexOf(e->text());
        if (j != -1)
            WRITE_INT(j);
        else {
            QMessageBox msgBox;
            msgBox.setText("Нет урока " + e->text() + ".");
            msgBox.exec();
            return;
        }
    }
    for (int i = 0; i < 8; i++) {
        auto e = ui->editTimetableDay5->item(i);
        int j = lessons.indexOf(e->text());
        if (j != -1)
            WRITE_INT(j);
        else {
            QMessageBox msgBox;
            msgBox.setText("Нет урока " + e->text() + ".");
            msgBox.exec();
            return;
        }
    }

    send(requestData);
}

void MainWindow::on_editTimetableChoose_currentIndexChanged(int index)
{
    Q_UNUSED(index)
}

void MainWindow::on_editTimetableChoose_editTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
}

void MainWindow::on_editTimetableChoose_activated(int index)
{
    Q_UNUSED(index)
    auto map = timetables[ui->editTimetableNumber->currentIndex()];
    if (!map.contains(ui->editTimetableChoose->currentText())) {
        map.insert(ui->editTimetableChoose->currentText(),
               new Timetable(QStringList(48), ui->editTimetableNumber->currentIndex()));
    }

    Timetable * t = map.value(ui->editTimetableChoose->currentText());
    QStringList day;

    day = t->getDay(0);
    for (int i = 0; i < 8; i++)
        ui->editTimetableDay0->item(i)->setText(day[i]);
    day = t->getDay(1);
    for (int i = 0; i < 8; i++)
        ui->editTimetableDay1->item(i)->setText(day[i]);
    day = t->getDay(2);
    for (int i = 0; i < 8; i++)
        ui->editTimetableDay2->item(i)->setText(day[i]);
    day = t->getDay(3);
    for (int i = 0; i < 8; i++)
        ui->editTimetableDay3->item(i)->setText(day[i]);
    day = t->getDay(4);
    for (int i = 0; i < 8; i++)
        ui->editTimetableDay4->item(i)->setText(day[i]);
    day = t->getDay(5);
    for (int i = 0; i < 8; i++)
        ui->editTimetableDay5->item(i)->setText(day[i]);
}


int getQuarterNumber() {
    auto d = QDateTime::currentDateTime().date();
    if ((d.month() == 8) ||
        (d.month() == 9) ||
        (d.month() == 10 && d.day() < 5))
        return 1;

    if ((d.month() == 10) ||
        (d.month() == 11) ||
        (d.month() ==  1 && d.day() < 9))
        return 2;

    if ((d.month() ==  1) ||
        (d.month() ==  2) ||
        (d.month() ==  3 && d.day() < 3))
        return 3;

    return 4;
}

int getCurrentYearNumber() {
    auto date = QDateTime::currentDateTime().date();
    int year = date.year();
    int month = date.month();
    if (month < 7)
        year--;

    return year;
}

void MainWindow::on_calendarStack_currentChanged(int index)
{
    if (ui->calendarGroup->currentIndex())
        ui->calendarGroup->setCurrentIndex(0);
    else
        on_calendarGroup_currentIndexChanged(0);

    switch (index) {
    case 0:
        ui->calendarTitle->setText(QString::number(getCurrentYearNumber()) + " - " + QString::number(getCurrentYearNumber() + 1) + " Год");
        break;
    case 1:
        ui->calendarTitle->setText(QString::number(getQuarterNumber()) + " Четверть");
        break;
    case 2:
        ui->calendarTitle->setText(QString::number(QDateTime::currentDateTime().date().weekNumber()) + " Неделя");
        break;
    }
}

QString formCalendarTitle(int type, int * number) {

    switch (type) {
    case 0:
        if (*number < 2022)
            *number = 2022;
        return QString::number(*number) + " - " + QString::number(*number + 1) + " Год";
    case 1:
        if (*number < 1)
            *number = 1;
        else if (*number > 4)
            *number = 4;
        return QString::number(*number) + " Четверть";
    case 2:
        if (*number < 1)
            *number = 1;
        else if (*number > 54)
            *number = 54;
        return QString::number(*number) + " Неделя";
    default:
        return "0 Error";
    }
}

void fillGetCalendarRequest(QDataStream & request, int type, int number, QString group, int personId) {
    INIT_STATIC_STRING_WRITE(GROUP_LENGTH);
        WRITE_CHAR(1);
        WRITE_CHAR(cid_getTimetable);
        WRITE_CHAR(type);
        WRITE_STATIC_STRING(group, GROUP_LENGTH);
        WRITE_INT(personId);
}

void MainWindow::on_calendarNext_clicked()
{
    int type = ui->calendarStack->currentIndex();
    int n = ui->calendarTitle->text().split(" ")[0].toInt() + 1;
    ui->calendarTitle->setText(formCalendarTitle(type, &n));

    REQUEST
    fillGetCalendarRequest(request, type, n,
            ui->calendarGroup->currentText(),
            getUser(ui->calendarPerson->currentText().split(" "), 0)->id);
    send(requestData);

    on_calendarPerson_currentIndexChanged(ui->calendarPerson->currentIndex());
}

void MainWindow::on_calendarBack_clicked()
{
    int type = ui->calendarStack->currentIndex();
    int n = ui->calendarTitle->text().split(" ")[0].toInt() - 1;
    ui->calendarTitle->setText(formCalendarTitle(type, &n));

    REQUEST
    fillGetCalendarRequest(request, type, n,
            ui->calendarGroup->currentText(),
            getUser(ui->calendarPerson->currentText().split(" "), 0)->id);
    send(requestData);

    on_calendarPerson_currentIndexChanged(ui->calendarPerson->currentIndex());
}

void MainWindow::on_calendarGroup_currentIndexChanged(int index)
{
    Q_UNUSED(index)
}

void MainWindow::on_calendarPerson_currentIndexChanged(int index)
{
    Q_UNUSED(index)

    REQUEST

    INIT_STATIC_STRING_WRITE(GROUP_LENGTH)
            WRITE_CHAR(1);
            WRITE_CHAR(cid_getTimetable);
            WRITE_CHAR(ui->calendarStack->currentIndex());
            WRITE_INT(ui->calendarTitle->text().split(" ")[0].toInt());
            WRITE_STATIC_STRING(ui->calendarGroup->currentText(), GROUP_LENGTH);
            if (ui->calendarPerson->currentText() == "-")
            WRITE_INT(-1);
            else
            WRITE_INT(getUser(ui->calendarPerson->currentText().split(" "), 0)->id);

    send(requestData);
}

void MainWindow::on_calendarPerson_activated(int index)
{
    Q_UNUSED(index)
}


void MainWindow::on_editTimetableAddCondition_clicked()
{
    ui->editTimetableConditions_2->layout()->addWidget(new TimetableConditionWidget());
}

void MainWindow::on_editTimetableGenerate_clicked()
{
    REQUEST
    INIT_STATIC_STRING_WRITE(GROUP_LENGTH)

    WRITE_CHAR(1);
    WRITE_CHAR(cid_generateTimetable);

    WRITE_INT(ui->editTimetableConditions_2->children().count() - 1);

    for (int i = 1; i < ui->editTimetableConditions_2->children().count(); i++) {
        auto w = static_cast<TimetableConditionWidget *>(ui->editTimetableConditions_2->children()[i]);

        QString error = w->form_valid();
        if (error.length()) {
            QMessageBox msgBox;
            msgBox.setText("Ошибка в форме: " + QString::number(i) + ":" + error + "!");
            msgBox.exec();
            return;
        }

        WRITE_CHAR(w->form_type());
        if (w->form_type() % 2 == 0) {
            WRITE_STATIC_STRING(w->form_subdata(), GROUP_LENGTH);
        }

        WRITE_INT(w->form_rows());
        for (int i = 0; i < w->form_rows(); i++) {
            QStringList v = w->form_row(i);
            QStringList b;
            switch (w->form_type()) {
            case 0:
                WRITE_INT(lessons.indexOf(v[0]));
                WRITE_CHAR(v[1].toInt());
               break;
            case 1:
                WRITE_INT(getUser(v[0].split(" "))->id);

                b = v[1].split(", ", Qt::SkipEmptyParts);
                WRITE_INT(b.length());
                for (QString s : b)
                    WRITE_INT(lessons.indexOf(s));

                b = v[2].split(", ", Qt::SkipEmptyParts);
                WRITE_CHAR(b.length());
                for (QString s : b)
                    WRITE_CHAR(QString("ОТРЕЯУ").indexOf(s[1]));
               break;
            case 2:
                WRITE_INT(getUser(v[0].split(" "))->id);

                b = v[1].split(", ", Qt::SkipEmptyParts);
                WRITE_CHAR(b.length());
                for (QString s : b)
                    WRITE_CHAR(QString("ОТРЕЯУ").indexOf(s[1]));

                b = v[2].split(", ", Qt::SkipEmptyParts);
                WRITE_INT(b.length());
                for (QString s : b)
                    WRITE_INT(lessons.indexOf(s));

                b = v[2].split(", ", Qt::SkipEmptyParts);
                if (b.length() == 2)
                    WRITE_CHAR(3);
                else
                    WRITE_CHAR(b[0].toInt());
               break;
            case 3:
                WRITE_INT(lessons.indexOf(v[0]));

                b = v[1].split(", ", Qt::SkipEmptyParts);
                WRITE_CHAR(b.length());
                for (QString s : b)
                    WRITE_CHAR(QString("ОТРЕЯУ").indexOf(s[1]));

                b = v[2].split(", ", Qt::SkipEmptyParts);
                WRITE_CHAR(b.length());
                for (QString s : b)
                    WRITE_CHAR(s.toInt());
               break;
            case 4:
                WRITE_INT(lessons.indexOf(v[0]));
                WRITE_INT(v[1].toInt());
                WRITE_CHAR((v[2] == "Да"));

                b = v[3].split(", ", Qt::SkipEmptyParts);
                WRITE_INT((b[0].toInt() | b[1].toInt()));
                break;
           }
        }
    }

    send(requestData);
}


void MainWindow::on_calendarGroup_activated(int index)
{
    ui->calendarPerson->clear();
    if (USER_ROLE == 2)
        ui->calendarPerson->addItem(USER_NAME + " " + USER_SURNAME);
    else {
        ui->calendarPerson->addItem("-");
        for (int id : groupMembers[ui->calendarGroup->currentText()])
            ui->calendarPerson->addItem(getUser(id)->toString());
    }

    ui->calendarPerson->activated(0);
}


void MainWindow::on_yearContent_cellChanged(int row, int column)
{
    if (column == 0 || editTimetable)
        return;
    int mark = ui->yearContent->item(row, column)->text().toInt();
    if (mark >= 1 && mark <= 5) {
        REQUEST

                WRITE_CHAR(1);
                WRITE_CHAR(cid_setMark);

                WRITE_CHAR(0);
                WRITE_INT(ui->calendarTitle->text().split(" ")[0].toInt());
                if (ui->calendarPerson->currentText() == "-")
                WRITE_INT(-1);
                else
                WRITE_INT(getUser(ui->calendarPerson->currentText().split(" "), 0)->id);
                WRITE_INT(orderedLessons[row]);
                WRITE_CHAR(mark);
                WRITE_CHAR(column);

        send(requestData);
    } else
        on_calendarPerson_currentIndexChanged(ui->calendarPerson->currentIndex());
}


void MainWindow::on_quarterContent_cellChanged(int row, int column)
{
    if (row == 0 || column == 0 || editTimetable)
        return;
    int mark = ui->quarterContent->item(row, column)->text().toInt();
     if (mark >= 1 && mark <= 5) {
         REQUEST

                 WRITE_CHAR(1);
                 WRITE_CHAR(cid_setMark);

                 WRITE_CHAR(1);
                 WRITE_INT(ui->calendarTitle->text().split(" ")[0].toInt());
                 if (ui->calendarPerson->currentText() == "-")
                 WRITE_INT(-1);
                 else
                 WRITE_INT(getUser(ui->calendarPerson->currentText().split(" "), 0)->id);
                 WRITE_INT(orderedLessons[row - 1]);
                 WRITE_CHAR(mark);
                 WRITE_INT((column == ui->quarterContent->columnCount() - 1? 0:ui->quarterContent->item(0, column)->text().toInt()));

         send(requestData);
     } else
         on_calendarPerson_currentIndexChanged(ui->calendarPerson->currentIndex());
}


void MainWindow::onCalendarChanged(int row, int column, QTableWidget * day, int dayId) {
    if (column == 0 || editTimetable)
        return;

    if (!day->item(row, 0))
        return;

    if (ui->calendarPerson->currentIndex() == 0)
        on_calendarPerson_currentIndexChanged(0);

    int week = ui->calendarTitle->text().split(" ")[0].toInt();

    if (column == 1) {
        int mark = day->item(row, 1)->text().toInt();
        if (mark < 1 || mark > 5) {
            on_calendarPerson_currentIndexChanged(ui->calendarPerson->currentIndex());
            return;
        }

        REQUEST
                WRITE_CHAR(1);
                WRITE_CHAR(cid_setMark);

                WRITE_CHAR(2);
                WRITE_INT(week);
                if (ui->calendarPerson->currentText() == "-")
                WRITE_INT(-1);
                else
                WRITE_INT(getUser(ui->calendarPerson->currentText().split(" "), 0)->id);
                WRITE_INT(lessons.indexOf(day->item(row, 0)->text()));
                WRITE_CHAR(mark);
                WRITE_INT(week * 7 + dayId);

        send(requestData);
    } else if (column == 2) {
        REQUEST
        INIT_STATIC_STRING_WRITE(GROUP_LENGTH)
                WRITE_CHAR(1);
                WRITE_CHAR(cid_manageTask);
                WRITE_STATIC_STRING(ui->calendarGroup->currentText(), GROUP_LENGTH);
                WRITE_DYNAMIC_STRING(day->item(row, 2)->text(), short);
                WRITE_INT(week * 7 + dayId);
                WRITE_INT(row);

        send(requestData);
    }
}

void MainWindow::on_calendarDay0_cellChanged(int row, int column)
{
    onCalendarChanged(row, column, ui->calendarDay0, 0);
}

void MainWindow::on_calendarDay1_cellChanged(int row, int column)
{
    onCalendarChanged(row, column, ui->calendarDay1, 1);
}

void MainWindow::on_calendarDay2_cellChanged(int row, int column)
{
    onCalendarChanged(row, column, ui->calendarDay2, 2);
}

void MainWindow::on_calendarDay3_cellChanged(int row, int column)
{
    onCalendarChanged(row, column, ui->calendarDay3, 3);
}

void MainWindow::on_calendarDay4_cellChanged(int row, int column)
{
    onCalendarChanged(row, column, ui->calendarDay4, 4);
}

void MainWindow::on_calendarDay5_cellChanged(int row, int column)
{
    onCalendarChanged(row, column, ui->calendarDay5, 5);
}
