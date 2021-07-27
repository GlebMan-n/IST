QT -= gui

TEMPLATE = lib
#DEFINES += WEBSOCKETLIB_LIBRARY SHOW_DEBUG_LOG NOPOLL_64BIT_PLATFORM

CONFIG += c++98

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += /usr/include/openssl
unix:!macx: LIBS += -lssl -lcrypto

SOURCES += \
    nopoll.c \
    nopoll_conn.c \
    nopoll_conn_opts.c \
    nopoll_ctx.c \
    nopoll_decl.c \
    nopoll_io.c \
    nopoll_listener.c \
    nopoll_log.c \
    nopoll_loop.c \
    nopoll_msg.c \
    nopoll_win32.c \
    websocketlib.cpp

HEADERS += \
    WebSocketLib_global.h \
    nopoll.h \
    nopoll_config.h \
    nopoll_config_win32.h \
    nopoll_config_win64.h \
    nopoll_conn.h \
    nopoll_conn_opts.h \
    nopoll_ctx.h \
    nopoll_decl.h \
    nopoll_handlers.h \
    nopoll_io.h \
    nopoll_listener.h \
    nopoll_log.h \
    nopoll_loop.h \
    nopoll_msg.h \
    nopoll_private.h \
    nopoll_win32.h \
    websocketlib.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    libnopoll.def
