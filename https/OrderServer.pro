TEMPLATE = app
CONFIG += c++11 console
CONFIG -= app_bundle
CONFIG -= qt

DESTDIR = ../../bin

TARGET = ApiServer
DEFINES += MUDUO_STD_STRING
DEFINES += _USE_REDLOCK_
DEFINES += _EVENTLOOP_CONTEXT_
DEFINES += _STAT_ORDER_QPS_
DEFINES += _STAT_ORDER_QPS_DETAIL_
DEFINES += _USE_SCORE_ORDERS_

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.

LIBS +=-pthread -lrt -lm

#LIBS += $$PWD/../libs/libmuduo.a
LIBS += $$PWD/../libs/libmuduo-vip.a
LIBS += $$PWD/../libs/libwebsocket.a
LIBS += -lboost_locale -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread -lboost_regex
LIBS += -lcurl -lcrypto -lssl -lsnappy -pthread -lrt -lm -lz -ldl

INCLUDEPATH += ../../
INCLUDEPATH += ../../public
INCLUDEPATH += ../include
INCLUDEPATH += ../include/hiredis-vip
INCLUDEPATH += ../include/Json/include

INCLUDEPATH += /usr/local /include/google/protobuf
LIBS += /usr/local/lib/libprotobuf.a
LIBS += $$PWD/../libs/libJson.a
#LIBS += -L$$PWD/../libs -lmongocxx -lbsoncxx -lmongoc-1.0

LIBS += $$PWD/../libs/libmongocxx.so
LIBS += $$PWD/../libs/libbsoncxx.so

LIBS += $$PWD/../libs/libgameCrypto.a

#LIBS += $$PWD/../libs/libmongocxx.so.3.4.0
#LIBS += $$PWD/../libs/libbsoncxx.so.3.4.0
INCLUDEPATH += /usr/local/include
INCLUDEPATH += /usr/local/include/zookeeper
LIBS += $$PWD/../libs/libzookeeperclient.a
LIBS += /usr/local/lib/libzookeeper_mt.a

LIBS += $$PWD/../libs/libRedisClient.a
LIBS += $$PWD/../libs/libhiredis-vip.a
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    OrderServer.cpp \
    AES_Cipher.cpp \
    IPFinder.cpp \
    mymd5.cpp \
    urlcodec.cpp \
    base64.cpp \
    crypto.cpp \
    ../../public/GlobalFunc.cpp \
    ../../public/RedisClient/redlock.cpp \
    ../../public/ThreadLocalSingletonMongoDBClient.cpp \
    ../../public/zookeeperclient/zookeeperclient.cpp \
    ../../public/zookeeperclient/zookeeperclientutils.cpp \
    ../../proto/Game.Common.pb.cc \
    ../../proto/HallServer.Message.pb.cc \
    ../../proto/ProxyServer.Message.pb.cc \
    ../../proto/GameServer.Message.pb.cc

HEADERS += \
    net/libwebsocket/base.h \
    net/libwebsocket/IBytesBuffer.h \
    net/libwebsocket/ICallback.h \
    net/libwebsocket/IContext.h \
    net/libwebsocket/IHttpContext.h \
    net/libwebsocket/ITimestamp.h \
    net/libwebsocket/ssl.h \
    net/libwebsocket/websocket.h \
    net/libwebsocket/server.h \
    OrderServer.h \
    AES_Cipher.h \
    IPFinder.h \
    mymd5.h \
    urlcodec.h \
    base64.h \
    crypto.h \
    ../../public/gameDefine.h \
    ../../public/GlobalFunc.h \
    ../../public/ThreadLocalSingletonMongoDBClient.h \
    ../../proto/Game.Common.pb.h \
    ../../proto/HallServer.Message.pb.h \
    ../../proto/ProxyServer.Message.pb.h \
    ../../proto/GameServer.Message.pb.h

DISTFILES += \
    ../../proto/src/Game.Common.proto \
    ../../proto/src/ProxyServer.Message.proto \
    ../../proto/src/HallServer.Message.proto \
    ../../proto/src/GameServer.Message.proto


unix {
    target.path = /usr/lib
    INSTALLS += target
}

