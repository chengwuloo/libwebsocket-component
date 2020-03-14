#include "demo/echo.h"
#include "muduo/base/Logging.h"
#include <libwebsocket/websocket.h>
#include <libwebsocket/ssl.h>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

EchoServer::EchoServer(muduo::net::EventLoop* loop,
                       const muduo::net::InetAddress& listenAddr,
	std::string const& cert_path, std::string const& private_key_path,
	std::string const& client_ca_cert_file_path,
	std::string const& client_ca_cert_dir_path)
    : server_(loop, listenAddr, "EchoServer") {

	//TCP callback ///
	server_.setConnectionCallback(
		std::bind(
			&EchoServer::onConnection, this,
			std::placeholders::_1));
	server_.setMessageCallback(
		std::bind(&EchoServer::onMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	
	//using websocket protocol ///
	server_.enableWebsocket(true);

	//websocket callback ///
	server_.setWsConnectedCallback(
		std::bind(&EchoServer::onWebSocketConnected, this,
			std::placeholders::_1, std::placeholders::_2));
	server_.setWsMessageCallback(
		std::bind(
			&EchoServer::onWebSocketMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	server_.setWsClosedCallback(
		std::bind(
			&EchoServer::onWebSocketClosed, this,
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
	server_.start();
}

void EchoServer::onConnection(const muduo::net::TcpConnectionPtr& conn) {
	
	if (conn->connected()) {
		LOG_INFO << "EchoServer - " << conn->peerAddress().toIpPort() << " -> "
			<< conn->localAddress().toIpPort() << " is "
			<< (conn->connected() ? "UP" : "DOWN");
	}
	else {
		LOG_INFO << "EchoServer - " << conn->peerAddress().toIpPort() << " -> "
			<< conn->localAddress().toIpPort() << " is "
			<< (conn->connected() ? "UP" : "DOWN");
	}
}

void EchoServer::onMessage(const muduo::net::TcpConnectionPtr& conn,
                           muduo::net::Buffer* buf,
                           muduo::Timestamp receiveTime) {

	//////////////////////////////////////////////////////////////////////////
	//parse_message_frame
	//////////////////////////////////////////////////////////////////////////
	muduo::net::websocket::parse_message_frame(
		conn->getWebsocketContext(),
		buf,
		&receiveTime);
}

//onWebSocketConnected websocket
void EchoServer::onWebSocketConnected(
	const muduo::net::TcpConnectionPtr& conn,
	std::string const& ipaddr) {
		
	LOG_INFO << "EchoServer - onWebSocketConnected - " << ipaddr << " -> "
		<< conn->localAddress().toIpPort() << " is "
		<< (conn->connected() ? "UP" : "DOWN");
}

//onWebSocketMessage websocket
void EchoServer::onWebSocketMessage(
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
	//////////////////////////////////////////////////////////////////////////
	//pack_unmask_data_frame
	//////////////////////////////////////////////////////////////////////////
	muduo::net::Buffer rspbuf;
	muduo::net::websocket::pack_unmask_data_frame(
		&rspbuf,
		str.c_str(), str.length(),
		muduo::net::websocket::MessageT::TyTextMessage, 1);
	conn->send(&rspbuf);
}

//onWebSocketClosed websocket
void EchoServer::onWebSocketClosed(
	const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {

	LOG_INFO << "EchoServer - onWebSocketClosed - ";// << conn->peerAddress().toIpPort() << " -> "
		//<< conn->localAddress().toIpPort() << " is "
		//<< (conn->connected() ? "UP" : "DOWN");
	
	//////////////////////////////////////////////////////////////////////////
	//pack_unmask_data_frame
	//////////////////////////////////////////////////////////////////////////
	muduo::net::Buffer rspdata;
	muduo::net::websocket::pack_unmask_close_frame(
		&rspdata,
		buf->peek(), buf->readableBytes());
	conn->send(&rspdata);
}

//processRequest
void EchoServer::processRequest(const muduo::net::TcpConnectionPtr& conn,
	std::vector<uint8_t> &buf,
	muduo::Timestamp receiveTime) {

}