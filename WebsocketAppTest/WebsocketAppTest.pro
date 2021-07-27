QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++98

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
DEFINES += SHOW_DEBUG_LOG NOPOLL_64BIT_PLATFORM
DEFINES +=__DEBUG__
SOURCES += \
    graphqlsubscriber.cpp \
    main.cpp \
    mainwindow.cpp \
    graphqlsubscriberpool.cpp

HEADERS += \
    graphqlsubscriber.h \
    mainwindow.h \
    graphqlsubscriberpool.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target



INCLUDEPATH += /usr/include/openssl
unix:!macx: LIBS += -lssl
LIBS += -lm -lcrypto

unix:!macx: LIBS += -L$$PWD/WebSocketLib/ -lWebSocketLib

INCLUDEPATH += $$PWD/WebSocketLib
QMAKE_LFLAGS += -Wl,-rpath,"'\$$ORIGIN'"
