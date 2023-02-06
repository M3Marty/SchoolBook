#include "chatwidget.h"
#include "ui_chatwidget.h"

#include <QDateTime>

ChatWidget::ChatWidget(int id, QString name, QString message, long long timeStamp, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatWidget)
{
    ui->setupUi(this);

    this->id = id;

    ui->name->setText(name);

    ui->lastMessage->setText(message);
    ui->lastTimestamp->setText(QDateTime::fromMSecsSinceEpoch(timeStamp).toString("yyyy.MM.dd"));
}

ChatWidget::~ChatWidget()
{
    delete ui;
}

void ChatWidget::update(QString message, long long timeStamp) {
    ui->lastMessage->setText(message);
    ui->lastTimestamp->setText(QDateTime::fromMSecsSinceEpoch(timeStamp).toString("yyyy.MM.dd"));
}
