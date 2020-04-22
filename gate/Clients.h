/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#ifndef CLIENTS_INCLUDE_H
#define CLIENTS_INCLUDE_H

#include "muduo/base/Logging.h"

#include "muduo/net/TcpClient.h"
#include "muduo/base/Thread.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThreadPool.h"
#include "muduo/net/InetAddress.h"

#include <utility>
#include <map>

#include <stdio.h>
#include <unistd.h>
#include <memory>

class Connector;

//@@ TcpClient
class TcpClient : muduo::noncopyable,
	public std::enable_shared_from_this<TcpClient> {
public:
	TcpClient(muduo::net::EventLoop* loop,
		const muduo::net::InetAddress& serverAddr,
		const std::string& name, Connector* owner);
	
	const std::string& name() const;
	muduo::net::TcpConnectionPtr connection() const;
	muduo::net::EventLoop* getLoop() const;

	void connect();
	void reconnect();
	void disconnect();
	void stop();

	bool retry() const;
	void enableRetry();

private:
	void onConnection(
		const muduo::net::TcpConnectionPtr& conn);
	void onMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, muduo::Timestamp receiveTime);
private:
	muduo::net::TcpClient client_;
	Connector* owner_;
};

typedef std::shared_ptr<TcpClient> TcpClientPtr;
typedef std::weak_ptr<TcpClient> WeakTcpClientPtr;

typedef std::pair<std::string,
	muduo::net::WeakTcpConnectionPtr> ClientConn;
typedef std::vector<ClientConn> ClientConnList;

//@@ Connector
class Connector : muduo::noncopyable {
public:
	friend class TcpClient;
public:
	typedef std::map<std::string, TcpClientPtr> TcpClientMap;
public:
	Connector(muduo::net::EventLoop* loop);
	~Connector();

	void setConnectionCallback(const muduo::net::ConnectionCallback& cb)
	{
		connectionCallback_ = cb;
	}
	void setMessageCallback(const muduo::net::MessageCallback& cb)
	{
		messageCallback_ = cb;
	}
	//add
	void add(
		std::string const& name,
		const muduo::net::InetAddress& serverAddr);
	//remove
	void remove(std::string const& name, bool lazy = false);
	//check
	void check(std::string const& name, bool exist);
	//exists
	bool exists(std::string const& name) /*const*/;
	//count
	size_t const count() /*const*/;
	//get
	void get(std::string const& name, ClientConn& client);
	//getAll
	void getAll(ClientConnList& clients);
	//closeAll
	void closeAll();
protected:
	void onConnected(const muduo::net::TcpConnectionPtr& conn, const TcpClientPtr& client);
	void onClosed(const muduo::net::TcpConnectionPtr& conn, const std::string& name);
	void onMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

	void addInLoop(
		std::string const& name,
		const muduo::net::InetAddress& serverAddr);

	void newConnection(const muduo::net::TcpConnectionPtr& conn, const TcpClientPtr& client);
	void removeConnection(const muduo::net::TcpConnectionPtr& conn, const std::string& name);

	void connectionCallback(const muduo::net::TcpConnectionPtr& conn);

	void countInLoop(size_t& size, bool& bok);
	void checkInLoop(std::string const& name, bool exist);
	void existInLoop(std::string const& name, bool& exist, bool& bok);
	void getInLoop(std::string const& name, ClientConn& client, bool& bok);
	void getAllInLoop(ClientConnList& clients, bool& bok);
	void removeInLoop(std::string const& name, bool lazy);
	void cleanupInLoop();
	void closeAllInLoop();
private:
	muduo::net::EventLoop* loop_;
	TcpClientMap clients_;
	std::map<std::string, bool> removes_;
	muduo::AtomicInt32 numConnected_;
	muduo::net::ConnectionCallback connectionCallback_;
	muduo::net::MessageCallback messageCallback_;
};

#endif