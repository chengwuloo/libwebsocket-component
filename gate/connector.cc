/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#include "connector.h"

//@@ TcpClient
TcpClient::TcpClient(
	muduo::net::EventLoop* loop,
	const muduo::net::InetAddress& serverAddr,
	const std::string& name,
	Connector* owner)
	: client_(loop, serverAddr, name)
    , owner_(owner) {
	client_.setConnectionCallback(
		std::bind(&TcpClient::onConnection, this, std::placeholders::_1));
	client_.setMessageCallback(
		std::bind(&TcpClient::onMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

const std::string& TcpClient::name() const {
	return client_.name();
}

muduo::net::TcpConnectionPtr TcpClient::connection() const {
	return client_.connection();
}

muduo::net::EventLoop* TcpClient::getLoop() const {
	return client_.getLoop();
}

void TcpClient::connect() {
	client_.connect();
}

void TcpClient::disconnect() {
	client_.disconnect();
}

void TcpClient::stop() {
	client_.stop();
}

bool TcpClient::retry() const {
	return client_.retry();
}

void TcpClient::enableRetry() {
	client_.enableRetry();
}

void TcpClient::onConnection(const muduo::net::TcpConnectionPtr& conn) {
	if (conn->connected()) {
		owner_->onConnected(conn, shared_from_this());
	}
	else {
		owner_->onClosed(conn, shared_from_this());
	}
}

void TcpClient::onMessage(
	const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {
	owner_->onMessage(conn, buf, receiveTime);
}

//@@ Connector
Connector::Connector(
	muduo::net::EventLoop* loop)
	: loop_(CHECK_NOTNULL(loop)) {
}

Connector::~Connector() {
//	closeAll();
//	quit();
}

//create
void Connector::create(
	std::string const& name,
	const muduo::net::InetAddress& serverAddr) {
	loop_->runInLoop(
		std::bind(&Connector::createInLoop, this, name, serverAddr));
}

//check
void Connector::check(std::string const& name, bool exist) {
	loop_->runInLoop(
		std::bind(&Connector::checkInLoop, this, name, exist));
}

//getAll
void Connector::getAll(ClientConnList& clients) {
	bool bok = false;
	loop_->runInLoop(
		std::bind(&Connector::getAllInLoop, this, clients, bok));
	//spin lock until getAllInLoop return
	while (!bok);
}

void Connector::getAllInLoop(ClientConnList& clients, bool& bok) {
	loop_->assertInLoopThread();
	for (TcpClientMap::iterator it = clients_.begin();
		it != clients_.end(); ++it) {
		clients.emplace_back(ClientConn(it->first, it->second->connection()));
	}
	bok = true;
}

void Connector::createInLoop(
	std::string const& name,
	const muduo::net::InetAddress& serverAddr) {
	
	loop_->assertInLoopThread();
	
	TcpClientMap::const_iterator it = clients_.find(name);
	if (it == clients_.end()) {
		TcpClientPtr client(new TcpClient(loop_, serverAddr, name, this));

		client->enableRetry();
		client->connect();
	}
	else {
		TcpClientPtr const& client = it->second;
		if (client) {
			assert(client->connection());
			assert(client->connection()->connected());
		}
	}
}

void Connector::checkInLoop(std::string const& name, bool exist) {

	loop_->assertInLoopThread();

	TcpClientMap::const_iterator it = clients_.find(name);
	if (it != clients_.end()) {
		TcpClientPtr const& client = it->second;
		if (client) {
			assert(
				exist ?
				( client->connection() &&  client->connection()->connected()) :
				(!client->connection() || !client->connection()->connected()));
		}
	}
}

void Connector::closeAll() {
	for (TcpClientMap::iterator it = clients_.begin();
		it != clients_.end(); ++it) {
		it->second->disconnect();
	}
}

void Connector::onConnected(const muduo::net::TcpConnectionPtr& conn, const TcpClientPtr& client) {
	
	loop_->assertInLoopThread();
	
	int32_t num = numConnected_.incrementAndGet();
	
	if (connectionCallback_) {
		connectionCallback_(conn);
	}
	{
		clients_[client->name()] = client;
		conn->setTcpNoDelay(true);
	}
}

void Connector::onClosed(const muduo::net::TcpConnectionPtr& conn, const TcpClientPtr& client) {
	
	loop_->assertInLoopThread();
	
	int32_t num = numConnected_.decrementAndGet();
	//if (num == 0) {
	//	conn->getLoop()->queueInLoop(
	//		std::bind(&muduo::net::EventLoop::quit, conn->getLoop()));
	//}
	
	if (connectionCallback_) {
		connectionCallback_(conn);
	}
	{
#if 1
		TcpClientMap::const_iterator it = clients_.find(client->name());
		assert(it != clients_.end());
		clients_.erase(it);
#else
		size_t n = clients_.erase(client->name());
		(void)n;
		assert(n == 1);
#endif
	}
	
}

void Connector::onMessage(
	const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {
	if (messageCallback_) {
		messageCallback_(conn, buf, receiveTime);
	}
}

void Connector::quit() {
	loop_->queueInLoop(
		std::bind(&muduo::net::EventLoop::quit, loop_));
}