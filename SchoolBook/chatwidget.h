#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>

namespace Ui {
class ChatWidget;
}

class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWidget(int id, QString name, QString message,
                        long long timeStamp, QWidget *parent = nullptr);
    ~ChatWidget();

    int id;

    void update(QString message, long long timeStamp);

private:
    Ui::ChatWidget *ui;
};

#endif // CHATWIDGET_H
