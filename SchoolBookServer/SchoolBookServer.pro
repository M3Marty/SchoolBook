QT += network
QT += sql
QT += core
QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        dbaccessor.cpp \
        main.cpp \
        receiver.cpp \
        server.cpp \
        timetablegenerator.cpp \
        user.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    ../SchoolBook/sb_master.h \
    dbaccessor.h \
    receiver.h \
    server.h \
    timetablegenerator.h \
    user.h
