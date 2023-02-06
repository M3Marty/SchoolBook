QT += core gui widgets
QT += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    chatwidget.cpp \
    groupwidget.cpp \
    loginwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    post.cpp \
    postwidget.cpp \
    receiver.cpp \
    taskwidget.cpp \
    timetable.cpp \
    timetableconditionwidget.cpp \
    user.cpp

HEADERS += \
    chatwidget.h \
    groupwidget.h \
    loginwindow.h \
    mainwindow.h \
    post.h \
    postwidget.h \
    receiver.h \
    sb_master.h \
    taskwidget.h \
    timetable.h \
    timetableconditionwidget.h \
    user.h\
    ui_taskwidget.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

FORMS += \
    chatwidget.ui \
    groupwidget.ui \
    loginwindow.ui \
    mainwindow.ui \
    postwidget.ui \
    taskwidget.ui \
    timetableconditionwidget.ui
