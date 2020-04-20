#include "demo/echo.h"
#include <muduo/base/Logging.h>
#include <muduo/net/libwebsocket/server.h>
#include <muduo/net/libwebsocket/ssl.h>

EchoServer::EchoServer(muduo::net::EventLoop* loop,
                       const muduo::net::InetAddress& listenAddr,
	std::string const& cert_path, std::string const& private_key_path,
	std::string const& client_ca_cert_file_path,
	std::string const& client_ca_cert_dir_path)
    : server_(loop, listenAddr, "EchoServer") {
	//ConnectionCallback
	server_.setConnectionCallback(
		std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
	//MessageCallback
	server_.setMessageCallback(
		std::bind(&muduo::net::websocket::onMessage,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
#if 1
	//添加OpenSSL认证支持 ///
	muduo::net::ssl::SSL_CTX_Init(
		cert_path,
		private_key_path,
		client_ca_cert_file_path, client_ca_cert_dir_path);

	//指定SSL_CTX
	server_.set_SSL_CTX(muduo::net::ssl::SSL_CTX_Get());
#endif
}

EchoServer::~EchoServer() {
#if 1
	//释放SSL_CTX
	muduo::net::ssl::SSL_CTX_free();
#endif
}

//EventLoop one polling one thread
void EchoServer::setThreadNum(int numThreads) {

	server_.setThreadNum(numThreads);
}

void EchoServer::start() {
	//使用ET模式accept/read/write
	server_.start(true);
}

void EchoServer::onConnection(const muduo::net::TcpConnectionPtr& conn) {
	
	if (conn->connected()) {
		LOG_INFO << "EchoServer - " << conn->peerAddress().toIpPort() << " -> "
			<< conn->localAddress().toIpPort() << " is "
			<< (conn->connected() ? "UP" : "DOWN");
		
		//////////////////////////////////////////////////////////////////////////
		//websocket::hook
		//////////////////////////////////////////////////////////////////////////
		muduo::net::websocket::hook(
			std::bind(&EchoServer::onConnected, this,
				std::placeholders::_1, std::placeholders::_2),
			std::bind(&EchoServer::onMessage, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4),
			conn);
	}
	else {
		LOG_INFO << "EchoServer - " << conn->peerAddress().toIpPort() << " -> "
			<< conn->localAddress().toIpPort() << " is "
			<< (conn->connected() ? "UP" : "DOWN");
		//////////////////////////////////////////////////////////////////////////
		//websocket::reset
		//////////////////////////////////////////////////////////////////////////
		muduo::net::websocket::reset(conn);
	}
}

void EchoServer::onConnected(
	const muduo::net::TcpConnectionPtr& conn,
	std::string const& ipaddr) {

	LOG_INFO << "EchoServer - onConnected - " << ipaddr;
}

void EchoServer::onMessage(
	const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf, int msgType,
	muduo::Timestamp receiveTime) {
#if 1
	printf("%s recv %d bytes, str = \"%.*s\" received at %s\n",
		conn->name().c_str(),
		buf->readableBytes(),
		buf->readableBytes(), buf->peek(),
		receiveTime.toFormattedString().c_str());
#else
	printf(
		"%s recv %u bytes, received at %s\n",
		conn->name().c_str(),
		buf->readableBytes(),
		receiveTime.toFormattedString().c_str());
#endif
	std::string str = "pack_unmask_data_frame:  ";
	std::string result(buf->peek(), buf->readableBytes());
	std::string suffix("[wss://192.168.2.93:10000]");
	buf->retrieveAll();
	//str += result;
	str += suffix;
	muduo::net::websocket::send(conn, str.c_str(), str.length());
}
