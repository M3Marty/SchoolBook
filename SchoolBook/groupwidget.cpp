#include "groupwidget.h"
#include "ui_groupwidget.h"

#include <mainwindow.h>

GroupWidget::GroupWidget(QString name, QVector<int> members, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GroupWidget)
{
    ui->setupUi(this);

    this->members = members.sliced(1);

    for (auto id : this->members) {
        User * u = MainWindow::MAIN->getUser(id);
        if (u)
            ui->members->addItem(u->toString());
        else
            ui->members->addItem("Unknown ID " + QString::number(id));
    }

    ui->title->setText(name);
    ui->leader->setText(MainWindow::MAIN->getUser(members[0])->toString());
    ui->members->setVisible(false);
}

GroupWidget::~GroupWidget()
{
    delete ui;
}

void GroupWidget::on_title_clicked()
{
    ui->members->setVisible(!ui->members->isVisible());
}

void GroupWidget::on_members_itemDoubleClicked(QListWidgetItem *item)
{
    MainWindow::MAIN->goProfile(MainWindow::MAIN->getUser(item->text().split(" "))->id);
}

void GroupWidget::on_leader_clicked()
{
    MainWindow::MAIN->goProfile(MainWindow::MAIN->getUser(ui->leader->text().split(" "))->id);
}

