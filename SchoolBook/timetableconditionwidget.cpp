#include "timetableconditionwidget.h"
#include "ui_timetableconditionwidget.h"

#include <mainwindow.h>

#include <QT>

#define M MainWindow::MAIN

const static QBrush rightText(QColor(0xbbffbb));
const static QBrush wrongText(QColor(0xffbbbb));
const static QStringList days = QString("Понедельник Вторник Среда Четверг Пятница Суббота").split(" ");

TimetableConditionWidget::TimetableConditionWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TimetableConditionWidget)
{
    ui->setupUi(this);

    ui->stackedWidget->setCurrentIndex(0);

    ui->hoursChooser->addItems(M->leadGroups.filter("Класс"));
    ui->specialsTeachersChoose->addItems(M->leadGroups.filter("Класс"));
}

TimetableConditionWidget::~TimetableConditionWidget()
{
    delete ui;
}

QString testDays(QStringList l) {
    for (QString s:l)
        if (!days.contains(s))
            return s;

    return "";
}

QString testLessons(QStringList l) {
    for (QString s:l)
        if (!M->lessons.contains(s))
            return s;

    return "";
}

int testNumbers(QStringList l, int max = 8) {
    for (QString s:l) {
        int n = s.toInt();
        if (n > 0 && n < max + 1)
            continue;
        return n;
    }
    return 1;
}

QString testTeachers(QStringList l) {
    for (QString s:l)
        if (!((M->getUser(s.split(" "), 0)->role + 1) & 0b10))
            return s;

    return "";
}


void TimetableConditionWidget::on_typeChoose_activated(int index)
{
    ui->stackedWidget->setCurrentIndex(index);
}

void TimetableConditionWidget::on_deleteButton_clicked()
{
    delete this;
}


void TimetableConditionWidget::on_hoursChooser_activated(int index)
{
    Q_UNUSED(index)
}

void TimetableConditionWidget::on_hoursTable_cellChanged(int row, int column)
{
    auto item = ui->hoursTable->item(row, column);

    switch (column) {
    case 0:
        if (item->text().length() && M->lessons.contains(item->text()))
            item->setBackground(rightText);
        else
            item->setBackground(wrongText);
        break;
    case 1:
        if (item->text().toInt() > 0)
            item->setBackground(rightText);
        else
            item->setBackground(wrongText);
        break;
    }

    if (row == ui->hoursTable->rowCount() - 1 && column == 0)
        ui->hoursTable->insertRow(ui->hoursTable->rowCount());
    else if (row == ui->hoursTable->rowCount() - 2 && column == 0 &&
             ui->hoursTable->item(row, column)->text() == "")
        ui->hoursTable->removeRow(row + 1);
}


void TimetableConditionWidget::on_specialsTeachersTable_cellChanged(int row, int column)
{
    auto item = ui->specialsTeachersTable->item(row, column);

    switch (column) {
    case 0:
        if ((M->getUser(item->text().split(" "), 0)->role + 1) & 0b10)
            item->setBackground(rightText);
        else
            item->setBackground(wrongText);
        break;
    case 1:
        if (item->text().length() == 0 || days.contains(item->text()) ||
                testDays(item->text().split(", ", Qt::SkipEmptyParts)).length() == 0)
            item->setBackground(rightText);
        else
            item->setBackground(wrongText);
        break;
    case 2:
        if (item->text().length() == 0 || M->lessons.contains(item->text()) ||
                testLessons(item->text().split(", ", Qt::SkipEmptyParts)).length() == 0)
            item->setBackground(rightText);
        else
            item->setBackground(wrongText);
        break;

    }

    if (row == ui->specialsTeachersTable->rowCount() - 1 && column == 0)
        ui->specialsTeachersTable->insertRow(ui->specialsTeachersTable->rowCount());
    else if (row == ui->specialsTeachersTable->rowCount() - 2 && column == 0 &&
             ui->specialsTeachersTable->item(row, column)->text() == "")
        ui->specialsTeachersTable->removeRow(row + 1);
}

void TimetableConditionWidget::on_specialsTeachersChoose_activated(int index)
{
    Q_UNUSED(index)
}


void TimetableConditionWidget::on_teachersTable_cellChanged(int row, int column)
{
    auto item = ui->teachersTable->item(row, column);

    switch (column) {
    case 0:
        if ((M->getUser(item->text().split(" "), 0)->role + 1) & 0b10)
            item->setBackground(rightText);
        else
            item->setBackground(wrongText);
        break;
    case 1:
        if (item->text().length() != 0 && (M->lessons.contains(item->text()) ||
                testLessons(item->text().split(", ", Qt::SkipEmptyParts)).length() == 0))
            item->setBackground(rightText);
        else
            item->setBackground(wrongText);
        break;
    case 2:
        if (item->text().length() == 0 || days.contains(item->text()) ||
                testDays(item->text().split(", ", Qt::SkipEmptyParts)).length() == 0)
            item->setBackground(rightText);
        else
            item->setBackground(wrongText);
        break;
    case 3:
        if (item->text() == "1" || item->text() == "2" ||
                item->text() == "1, 2" || item->text() == "2, 1")
            item->setBackground(rightText);
        else
            item->setBackground(wrongText);
    }

    if (row == ui->teachersTable->rowCount() - 1 && column == 0)
        ui->teachersTable->insertRow(ui->teachersTable->rowCount());
    else if (row == ui->teachersTable->rowCount() - 2 && column == 0 &&
             ui->teachersTable->item(row, column)->text() == "")
        ui->teachersTable->removeRow(row + 1);
}


void TimetableConditionWidget::on_specialsLessonsTable_cellChanged(int row, int column)
{
    auto item = ui->specialsLessonsTable->item(row, column);

    switch (column) {
    case 0:
        if (item->text().length() && M->lessons.contains(item->text()))
            item->setBackground(rightText);
        else
            item->setBackground(wrongText);
        break;
    case 1:
        if (item->text().length() == 0 || days.contains(item->text()) ||
                testDays(item->text().split(", ", Qt::SkipEmptyParts)).length() == 0)
            item->setBackground(rightText);
        else
            item->setBackground(wrongText);
        break;
    case 2:
        if (item->text().length() == 0 || (item->text().toInt() > 0 && item->text().toInt() < 9)
                || testNumbers(item->text().split(", ", Qt::SkipEmptyParts)) == 1)
            item->setBackground(rightText);
        else
            item->setBackground(wrongText);
        break;
    }

    if (row == ui->specialsLessonsTable->rowCount() - 1 && column == 0)
        ui->specialsLessonsTable->insertRow(ui->specialsLessonsTable->rowCount());
    else if (row == ui->specialsLessonsTable->rowCount() - 2 && column == 0 &&
             ui->specialsLessonsTable->item(row, column)->text() == "")
        ui->specialsLessonsTable->removeRow(row + 1);
}


void TimetableConditionWidget::on_groupsChooser_activated(int index)
{
    Q_UNUSED(index)
}

void TimetableConditionWidget::on_groupsTable_cellActivated(int row, int column)
{
    Q_UNUSED(row)
    Q_UNUSED(column)
}

void TimetableConditionWidget::on_groupsTable_cellChanged(int row, int column)
{
    auto item = ui->groupsTable->item(row, column);

    switch (column) {
    case 0:
        if (item->text().length() && M->lessons.contains(item->text()))
            item->setBackground(rightText);
        else
            item->setBackground(wrongText);
        break;
    case 1: {
        bool sex = ui->groupsTable->item(row, 2)? ui->groupsTable->item(row, 2)->text() == "Да": false;
        if (item->text().toInt() > 1 && (!sex || item->text().toInt() == 2)) {
            on_groupsTable_cellChanged(row, 3);
            item->setBackground(rightText);
        }
        else
            item->setBackground(wrongText);
        break;
    }
    case 2:
        if (item->text() == "Да" || item->text() == "Нет" || item->text() == "") {
            item->setBackground(rightText);
            ui->groupsTable->setItem(row, 1, new QTableWidgetItem("2"));
        }
        else
            item->setBackground(wrongText);
        break;
    case 3: {
        QStringList teachers = item->text().split(", ", Qt::SkipEmptyParts);
        int groups = ui->groupsTable->item(row, 1)? ui->groupsTable->item(row, 1)->text().toInt(): 0;
        bool sex = ui->groupsTable->item(row, 2)? ui->groupsTable->item(row, 2)->text() == "Да": false;
        if (item->text().length() == 0 ||
            (testTeachers(teachers).length() == 0 && (
                (!sex && !groups) ||
                (teachers.length() == groups) ||
                (teachers.length() == 2 && sex)
            ))) {
            item->setBackground(rightText);
            if (!groups)
                ui->groupsTable->setItem(row, 1, new QTableWidgetItem(QString::number(teachers.length())));
        }
        else
            item->setBackground(wrongText);
    }
    }

    if (row == ui->groupsTable->rowCount() - 1 && column == 0)
        ui->groupsTable->insertRow(ui->groupsTable->rowCount());
    else if (row == ui->groupsTable->rowCount() - 2 && column == 0 &&
             ui->groupsTable->item(row, column)->text() == "")
        ui->groupsTable->removeRow(row + 1);

}


QString TimetableConditionWidget::form_valid() {

    auto e = ui->hoursTable;

    switch (ui->stackedWidget->currentIndex()) {
    case 0:
        for (int i = 0; i < e->rowCount() - 1; i++) {
            if (!e->item(i, 0) || e->item(i, 0)->background() != rightText)
                return QString::number(i + 1) + ":1";
            if (!e->item(i, 1) || e->item(i, 1)->background() != rightText)
                return QString::number(i + 1) + ":2";
        }
        break;
    case 1:
        e = ui->teachersTable;
        for (int i = 0; i < e->rowCount() - 1; i++) {
            if (!e->item(i, 0) || e->item(i, 0)->background() != rightText)
                return QString::number(i + 1) + ":1";
            if (!e->item(i, 1) || e->item(i, 1)->background() != rightText)
                return QString::number(i + 1) + ":2";
            if (e->item(i, 2) && e->item(i, 2)->background() == wrongText)
                return QString::number(i + 1) + ":3";
        }
        break;
    case 2:
        e = ui->specialsTeachersTable;
        for (int i = 0; i < e->rowCount() - 1; i++) {
            if (!e->item(i, 0) || e->item(i, 0)->background() != rightText)
                return QString::number(i + 1) + ":1";
            if (!e->item(i, 1) || e->item(i, 1)->background() == wrongText)
                return QString::number(i + 1) + ":2";
            if (e->item(i, 2) && e->item(i, 2)->background() == wrongText)
                return QString::number(i + 1) + ":3";
        }
        break;
    case 3:
        e = ui->specialsLessonsTable;
        for (int i = 0; i < e->rowCount() - 1; i++) {
            if (!e->item(i, 0) || e->item(i, 0)->background() != rightText)
                return QString::number(i + 1) + ":1";
            if (e->item(i, 1) && e->item(i, 1)->background() == wrongText)
                return QString::number(i + 1) + ":2";
            if (e->item(i, 2) && e->item(i, 2)->background() == wrongText)
                return QString::number(i + 1) + ":3";
        }
        break;
    case 4:
        e = ui->groupsTable;
        for (int i = 0; i < e->rowCount() - 1; i++) {
            if (!e->item(i, 0) || e->item(i, 0)->background() != rightText)
                return QString::number(i + 1) + ":1";
            if ((!e->item(i, 1) && !e->item(i, 2)) ||
                    (e->item(i, 1) && e->item(i, 1)->background() != rightText) ||
                    (e->item(i, 2) && e->item(i, 2)->background() != rightText))
                return QString::number(i + 1) + ":2,3";
            if (e->item(i, 3) && e->item(i, 3)->background() == wrongText)
                return QString::number(i + 1) + ":4";
        }
        break;
    }

    return "";
}

char TimetableConditionWidget::form_type() {
    return ui->stackedWidget->currentIndex();
}

QString TimetableConditionWidget::form_subdata() {
    switch (ui->stackedWidget->currentIndex()) {
    case 1:
    case 3:
        return "";
    case 0:
        return ui->hoursChooser->currentText();
    case 2:
        return ui->specialsTeachersChoose->currentText();
    case 4:
        return ui->groupsChooser->currentText();
    }
}

int TimetableConditionWidget::form_rows() {
    switch (ui->stackedWidget->currentIndex()) {
    case 0:
        return ui->hoursTable->rowCount() - 1;
    case 1:
        return ui->teachersTable->rowCount() - 1;
    case 2:
        return ui->specialsTeachersTable->rowCount() - 1;
    case 3:
        return ui->specialsLessonsTable->rowCount() - 1;
    case 4:
        return ui->groupsTable->rowCount() - 1;
    }

    return 0;
}

QStringList formRowReturn(QString c0, QString c1, QString c2="", QString c3="") {
    QStringList r;
    r.append(c0);
    r.append(c1);
    if (c2.length()) {
        r.append(c2);
        if (c3.length())
            r.append(c3);
    }

    return r;
}

QStringList TimetableConditionWidget::form_row(int i) {
    switch (ui->stackedWidget->currentIndex()) {
    case 0:
        return formRowReturn(ui->hoursTable->item(i, 0)->text(), ui->hoursTable->item(i, 1)->text());
    case 1:
        return formRowReturn(ui->teachersTable->item(i, 0)->text(), ui->teachersTable->item(i, 1)->text(),
                       ui->teachersTable->item(i, 2)->text());
    case 2:
        return formRowReturn(ui->specialsTeachersTable->item(i, 0)->text(), ui->specialsTeachersTable->item(i, 1)->text(),
                       ui->specialsTeachersTable->item(i, 2)->text());
    case 3:
        return formRowReturn(ui->specialsLessonsTable->item(i, 0)->text(), ui->specialsLessonsTable->item(i, 1)->text(),
                       ui->specialsLessonsTable->item(i, 2)->text());
    case 4:
        return formRowReturn(ui->groupsTable->item(i, 0)->text(), ui->groupsTable->item(i, 1)->text(),
                       ui->groupsTable->item(i, 2)->text(), ui->groupsTable->item(i, 3)->text());
    }

    return QStringList();
}
