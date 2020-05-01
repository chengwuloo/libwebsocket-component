#-------------------------------------------------
#
# Project created by QtCreator 2018-05-04T16:09:14
#
#-------------------------------------------------

QT       -= core gui

TARGET = muduo-vip
TEMPLATE = lib
CONFIG += staticlib
CONFIG += c++11

DESTDIR = ../../libs

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += MUDUO_STD_STRING

INCLUDEPATH += ..

LIBS += -pthread -lrt -lm

LIBS += /usr/local/lib/libboost_filesystem.a
LIBS += /usr/local/lib/libboost_system.a
LIBS += /usr/local/lib/libboost_system.a

LIBS += $$PWD/../../libs/libwebsocket.a

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    base/AsyncLogging.cc \
    base/Condition.cc \
    base/CountDownLatch.cc \
    base/Date.cc \
    base/Exception.cc \
    base/FileUtil.cc \
    base/LogFile.cc \
    base/Logging.cc \
    base/LogStream.cc \
    base/ProcessInfo.cc \
    base/Thread.cc \
    base/ThreadPool.cc \
    base/Timestamp.cc \
    base/TimeZone.cc \
    net/http/HttpContext.cc \
    net/http/HttpResponse.cc \
    net/http/HttpServer.cc \
    net/inspect/Inspector.cc \
    net/inspect/PerformanceInspector.cc \
    net/inspect/ProcessInspector.cc \
    net/inspect/SystemInspector.cc \
    net/poller/DefaultPoller.cc \
    net/poller/EPollPoller.cc \
    net/poller/PollPoller.cc \
    net/protobuf/ProtobufCodecLite.cc \
    net/protorpc/RpcChannel.cc \
    net/protorpc/RpcCodec.cc \
    net/protorpc/RpcServer.cc \
    net/protorpc/rpc.pb.cc \
    net/protorpc/rpcservice.pb.cc \
    net/Acceptor.cc \
    net/BoilerPlate.cc \
    net/Buffer.cc \
    net/Channel.cc \
    net/Connector.cc \
    net/EventLoop.cc \
    net/EventLoopThread.cc \
    net/EventLoopThreadPool.cc \
    net/InetAddress.cc \
    net/Poller.cc \
    net/Socket.cc \
    net/SocketsOps.cc \
    net/TcpClient.cc \
    net/TcpConnection.cc \
    net/TcpServer.cc \
    net/Timer.cc \
    net/TimerQueue.cc \
    base/CurrentThread.cc \
    net/Reactor.cc \
    net/libwebsocket/context.cc \
    net/libwebsocket/server.cc

HEADERS += \
    base/AsyncLogging.h \
    base/Atomic.h \
    base/BlockingQueue.h \
    base/BoundedBlockingQueue.h \
    base/Condition.h \
    base/copyable.h \
    base/CountDownLatch.h \
    base/CurrentThread.h \
    base/Date.h \
    base/Exception.h \
    base/FileUtil.h \
    base/GzipFile.h \
    base/LogFile.h \
    base/Logging.h \
    base/LogStream.h \
    base/Mutex.h \
    base/ProcessInfo.h \
    base/Singleton.h \
    base/StringPiece.h \
    base/Thread.h \
    base/ThreadLocal.h \
    base/ThreadLocalSingleton.h \
    base/ThreadPool.h \
    base/Timestamp.h \
    base/TimeZone.h \
    base/Types.h \
    base/WeakCallback.h \
    net/http/HttpContext.h \
    net/http/HttpRequest.h \
    net/http/HttpResponse.h \
    net/http/HttpServer.h \
    net/inspect/Inspector.h \
    net/inspect/PerformanceInspector.h \
    net/inspect/ProcessInspector.h \
    net/inspect/SystemInspector.h \
    net/poller/EPollPoller.h \
    net/poller/PollPoller.h \
    net/protobuf/BufferStream.h \
    net/protobuf/ProtobufCodecLite.h \
    net/protorpc/google-inl.h \
    net/protorpc/RpcChannel.h \
    net/protorpc/RpcCodec.h \
    net/protorpc/RpcServer.h \
    net/protorpc/rpc.pb.h \
    net/protorpc/rpcservice.pb.h \
    net/Acceptor.h \
    net/Buffer.h \
    net/Callbacks.h \
    net/Channel.h \
    net/Connector.h \
    net/Endian.h \
    net/EventLoop.h \
    net/EventLoopThread.h \
    net/EventLoopThreadPool.h \
    net/InetAddress.h \
    net/Poller.h \
    net/Socket.h \
    net/SocketsOps.h \
    net/TcpClient.h \
    net/TcpConnection.h \
    net/TcpServer.h \
    net/Timer.h \
    net/TimerId.h \
    net/TimerQueue.h \
    net/ZlibStream.h \
    net/BoilerPlate.h \
    net/Reactor.h \
    net/libwebsocket/base.h \
    net/libwebsocket/IBytesBuffer.h \
    net/libwebsocket/ICallback.h \
    net/libwebsocket/IContext.h \
    net/libwebsocket/IHttpContext.h \
    net/libwebsocket/ITimestamp.h \
    net/libwebsocket/ssl.h \
    net/libwebsocket/websocket.h \
    net/libwebsocket/context.h \
    net/libwebsocket/server.h \

unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    net/protorpc/rpc.proto \
    net/protorpc/rpcservice.proto \
    net/protorpc/README



