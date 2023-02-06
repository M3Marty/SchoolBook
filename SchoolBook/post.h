#ifndef POST_H
#define POST_H

#include <QString>

class Post
{
public:
    Post(QString title, QString text, long long timeStamp,
         QString author, int authorId, int postId);

    Post();

    QString title;
    QString text;
    long long timeStamp;
    QString author;
    int authorId;
    int postId;
};

#endif // POST_H
