#ifndef RECEIVER_H
#define RECEIVER_H

#include <QDataStream>

class Receiver
{
private:
    Receiver();
public:
    static void receive(QDataStream &);
};

#endif // RECEIVER_H
