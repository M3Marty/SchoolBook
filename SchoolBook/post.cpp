#include "post.h"

Post::Post(QString title, QString text, long long timeStamp,
           QString author, int authorId, int postId)
{
    this->title = title;
    this->text = text;
    this->timeStamp = timeStamp;
    this->author = author;
    this->authorId = authorId;
    this->postId = postId;
}

Post::Post()
{

}
