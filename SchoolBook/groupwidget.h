#ifndef GROUPWIDGET_H
#define GROUPWIDGET_H

#include <QWidget>
#include <QListWidgetItem>

namespace Ui {
class GroupWidget;
}

class GroupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GroupWidget(QString name, QVector<int> members, QWidget *parent = nullptr);
    ~GroupWidget();

    QVector<int> members;

private slots:
    void on_title_clicked();

    void on_members_itemDoubleClicked(QListWidgetItem *item);

    void on_leader_clicked();

private:
    Ui::GroupWidget *ui;
};

#endif // GROUPWIDGET_H
