/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#include "connector.h"
#include  <assert.h>

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

void TcpClient::reconnect() {
#if 0
	client_.connect();
#else
	client_.reconnect();
#endif
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
	
	conn->getLoop()->assertInLoopThread();
	
	if (conn->connected()) {
		owner_->onConnected(conn, shared_from_this());
	}
	else {
		owner_->onClosed(conn, client_.name());
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

//remove
void Connector::remove(std::string const& name) {
	loop_->runInLoop(
		std::bind(&Connector::removeInLoop, this, name));
}

//check
void Connector::check(std::string const& name, bool exist) {
	loop_->runInLoop(
		std::bind(&Connector::checkInLoop, this, name, exist));
}

//getAll
void Connector::getAll(ClientConnList* clients) {
	assert(clients && clients->size() == 0);
	bool bok = false;
	loop_->runInLoop(
		std::bind(&Connector::getAllInLoop, this, clients, &bok));
	//spin lock until getAllInLoop return
	while (!bok);
}

void Connector::getAllInLoop(ClientConnList* clients, bool* bok) {
	loop_->assertInLoopThread();
	for (TcpClientMap::const_iterator it = clients_.begin();
		it != clients_.end(); ++it) {
		TcpClientState const& state = it->second;
		TcpClientPtr const& client = state.first;
		clients->emplace_back(ClientConn(it->first, client->connection()));
	}
	*bok = true;
}

void Connector::createInLoop(
	std::string const& name,
	const muduo::net::InetAddress& serverAddr) {
	
	loop_->assertInLoopThread();
	
	TcpClientMap::iterator it = clients_.find(name);
	if (it == clients_.end()) {
		//name新节点
		TcpClientState state(TcpClientPtr(new TcpClient(loop_, serverAddr, name, this)), false);
		TcpClientPtr const& client = state.first;
		clients_[client->name()] = state;
		client->enableRetry();
		client->connect();
	}
	else {
		//name已存在
		TcpClientState& state = it->second;
		TcpClientPtr& client = state.first;
		if (client) {
			if (!CONNECTED(state)) {
				assert(
					!client->connection() ||
					!client->connection()->connected());
				//连接断开则重连
				client->reconnect();
			}
			else {
				assert(
					client->connection() &&
					client->connection()->connected());
			}
		}
		else {
			assert(!CONNECTED(state));
			state.first.reset(new TcpClient(loop_, serverAddr, name, this));
			client->enableRetry();
			client->connect();
		}
	}
}

void Connector::checkInLoop(std::string const& name, bool exist) {

	loop_->assertInLoopThread();

	TcpClientMap::const_iterator it = clients_.find(name);
	if (it == clients_.end()) {
		//name不存在
		if (exist) {
			assert(false);
		}
	}
	else {
		//name已存在
		TcpClientState const& state = it->second;
		TcpClientPtr const& client = state.first;
		if (exist) {
			assert(CONNECTED(state));
			assert(client);
			assert(
				client->connection() &&
				client->connection()->connected());
		}
		else {
			//连接断开
			assert(!CONNECTED(state));
			if (client) {
				assert(
					!client->connection() ||
					!client->connection()->connected());
			}
		}
	}
}

void Connector::closeAll() {
	for (TcpClientMap::const_iterator it = clients_.begin();
		it != clients_.end(); ++it) {
		TcpClientState const& state = it->second;
		TcpClientPtr const& client = state.first;
		client->disconnect();
	}
}

void Connector::onConnected(const muduo::net::TcpConnectionPtr& conn, const TcpClientPtr& client) {
	
	conn->getLoop()->assertInLoopThread();
	
	int32_t num = numConnected_.incrementAndGet();
	
	loop_->runInLoop(
		std::bind(&Connector::newConnection, this, conn, client));
}

void Connector::newConnection(const muduo::net::TcpConnectionPtr& conn, const TcpClientPtr& client) {
	
	loop_->assertInLoopThread();
	{
#if 0
		clients_[client->name()] = client;
		conn->setTcpNoDelay(true);
#else
		TcpClientMap::iterator it = clients_.find(client->name());
		assert(it != clients_.end());
		TcpClientState& state = it->second;
		state.second = true;
#endif
	}
	
	conn->getLoop()->runInLoop(std::bind(&Connector::connectionCallback, this, conn));
}

void Connector::onClosed(const muduo::net::TcpConnectionPtr& conn, const std::string& name) {
	
	conn->getLoop()->assertInLoopThread();
	
	int32_t num = numConnected_.decrementAndGet();
	//if (num == 0) {
	//	conn->getLoop()->queueInLoop(
	//		std::bind(&muduo::net::EventLoop::quit, conn->getLoop()));
	//}
	
	loop_->runInLoop(
		std::bind(&Connector::removeConnection, this, conn, name));
}

void Connector::removeConnection(const muduo::net::TcpConnectionPtr& conn, const std::string& name) {
	
	loop_->assertInLoopThread();
	{
#if 1
		if (1 == removes_.erase(name)) {
			TcpClientMap::const_iterator it = clients_.find(name);
			assert(it != clients_.end());
			//reset connector state
			it->second.first->stop();
			clients_.erase(it);
		}
		else {
			TcpClientMap::iterator it = clients_.find(name);
			assert(it != clients_.end());
			TcpClientState& state = it->second;
			//reset connector state
			it->second.first->stop();
			state.second = false;
			//state.first.reset();
		}
#else
		size_t n = clients_.erase(name);
		(void)n;
		assert(n == 1);
#endif
	}
	conn->getLoop()->runInLoop(std::bind(&Connector::connectionCallback, this, conn));
}

void Connector::connectionCallback(const muduo::net::TcpConnectionPtr& conn) {

	conn->getLoop()->assertInLoopThread();

	if (connectionCallback_) {
		connectionCallback_(conn);
	}
}

void Connector::removeInLoop(std::string const& name) {
	
	loop_->assertInLoopThread();
	
	TcpClientMap::const_iterator it = clients_.find(name);
	if (it != clients_.end()) {
		if (!CONNECTED(it->second)) {
			clients_.erase(it);
		}
		else {
			removes_[name] = true;
		}
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