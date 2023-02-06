#include "postwidget.h"
#include "ui_postwidget.h"

#include <mainwindow.h>

#include <QDateTime>
#include <QTcpSocket>

PostWidget::PostWidget(Post * post, QWidget *parent) : QWidget(parent),
    ui(new Ui::PostWidget)
{
    ui->setupUi(this);

    ui->title->setText(post->title);
    ui->text->setPlainText(post->text);
    ui->date->setText(QDateTime::fromMSecsSinceEpoch(post->timeStamp).toString("yyyy.MM.dd"));
    ui->goAuthor->setText(post->author);

    int role = MainWindow::USER_ROLE;
    QString name = MainWindow::USER_NAME + " " + MainWindow::USER_SURNAME;

    if (name != post->author) {
        ui->editPost->setVisible(false);
        if (role)
            ui->deletePost->setVisible(false);
    }

    int minimumSize = (int) (post->text.length() / 100.0 * 21);
    if (minimumSize > 100) {
        if (minimumSize < 400)
            ui->text->setMinimumHeight(minimumSize);
        else
            ui->text->setMinimumHeight(400);
    }

    this->post = *post;
}

PostWidget::~PostWidget()
{
    delete ui;
}

// From main.cpp
QTcpSocket * getLoginSocket();

void PostWidget::on_goAuthor_clicked()
{
    MainWindow::MAIN->goProfile(post.authorId);
}

void PostWidget::on_editPost_clicked()
{
    // TODO
}

void PostWidget::on_deletePost_clicked()
{
    QByteArray requestData;
    QDataStream request(&requestData, QIODevice::ReadWrite);
    request.setVersion(QDataStream::Version::Qt_6_4);

    char pages = 1;
    char command = 7; // Изменение поста
    char mode = 1; // Удаление
    request << pages << command << mode << this->post.postId;
    QTcpSocket * q = getLoginSocket();
    q->write(requestData);

    delete this;
}

