#ifndef POSTWIDGET_H
#define POSTWIDGET_H

#include <QWidget>
#include <post.h>

namespace Ui {
class PostWidget;
}

class PostWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PostWidget(Post * post, QWidget *parent = nullptr);
    ~PostWidget();

    Post post;
private slots:
    void on_goAuthor_clicked();

    void on_editPost_clicked();

    void on_deletePost_clicked();

private:
    Ui::PostWidget *ui;
};

#endif // POSTWIDGET_H
