#ifndef TIMETABLECONDITIONWIDGET_H
#define TIMETABLECONDITIONWIDGET_H

#include <QWidget>

namespace Ui {
class TimetableConditionWidget;
}

class TimetableConditionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimetableConditionWidget(QWidget *parent = nullptr);
    ~TimetableConditionWidget();

    QString form_valid();
    char form_type();
    QString form_subdata();
    int form_rows();
    QStringList form_row(int i);

private slots:
    void on_teachersTable_cellChanged(int row, int column);

    void on_typeChoose_activated(int index);

    void on_deleteButton_clicked();

    void on_hoursTable_cellChanged(int row, int column);

    void on_hoursChooser_activated(int index);

    void on_specialsLessonsTable_cellChanged(int row, int column);

    void on_specialsTeachersChoose_activated(int index);

    void on_specialsTeachersTable_cellChanged(int row, int column);

    void on_groupsChooser_activated(int index);

    void on_groupsTable_cellActivated(int row, int column);

    void on_groupsTable_cellChanged(int row, int column);

private:
    Ui::TimetableConditionWidget *ui;
};

#endif // TIMETABLECONDITIONWIDGET_H
