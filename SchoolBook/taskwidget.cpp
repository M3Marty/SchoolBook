#include "taskwidget.h"
#include "ui_taskwidget.h"

#include <mainwindow.h>

#define GENERAL_INCLUDE
#define REQUEST_WRITE
#define CID_EXTERN
#include <sb_master.h>

TaskWidget::TaskWidget(int id, int authorId, QString name, QString text, char status, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TaskWidget)
{
    ui->setupUi(this);

    this->id = id;
    this->authorId = authorId;
    this->status = status;

    ui->author->setText(name);
    ui->text->setText(text);
}

TaskWidget::~TaskWidget()
{
    delete ui;
}

void TaskWidget::on_back_clicked()
{
    if (status == 2) {
        ui->next->setVisible(true);
        status = 1;
        MainWindow::MAIN->moveTask(this, 2, 1);
    } else {
        ui->back->setVisible(false);
        status = 0;
        MainWindow::MAIN->moveTask(this, 1, 0);
    }

    REQUEST

    WRITE_CHAR(1);
    WRITE_CHAR(cid_updateTaskStatus);
    WRITE_INT(id);
    WRITE_CHAR(status);

    MainWindow::MAIN->send(requestData);
}


void TaskWidget::on_author_clicked()
{
    MainWindow::MAIN->goProfile(authorId);
}


void TaskWidget::on_next_clicked()
{
    if (status == 0) {
        ui->back->setVisible(true);
        status = 1;
        MainWindow::MAIN->moveTask(this, 0, 1);
    } else {
        ui->next->setVisible(false);
        status = 2;
        MainWindow::MAIN->moveTask(this, 1, 2);
    }

    REQUEST

    WRITE_CHAR(1);
    WRITE_CHAR(cid_updateTaskStatus);
    WRITE_INT(id);
    WRITE_CHAR(status);

    MainWindow::MAIN->send(requestData);
}

