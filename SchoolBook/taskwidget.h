#ifndef TASKWIDGET_H
#define TASKWIDGET_H

#include <QWidget>

namespace Ui {
class TaskWidget;
}

class TaskWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TaskWidget(int id, int authorId, QString name, QString text, char status, QWidget *parent = nullptr);
    ~TaskWidget();

    int id;
    int authorId;
    char status;
private slots:
    void on_back_clicked();

    void on_author_clicked();

    void on_next_clicked();

private:
    Ui::TaskWidget *ui;
};

#endif // TASKWIDGET_H
