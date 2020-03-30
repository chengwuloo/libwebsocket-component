/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#include <sstream>
#include <fstream>
#include <functional>
#include <sys/types.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include "proto/Game.Common.pb.h"
#include "proto/ProxyServer.Message.pb.h"
#include "proto/HallServer.Message.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "Gateway.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

Gateway::Gateway(muduo::net::EventLoop* loop,
	const muduo::net::InetAddress& listenAddr,
	const muduo::net::InetAddress& listenAddrInn,
	const muduo::net::InetAddress& listenAddrHttp,
	std::string const& cert_path, std::string const& private_key_path,
	std::string const& client_ca_cert_file_path,
	std::string const& client_ca_cert_dir_path)
    : server_(loop, listenAddr, "Gateway", cert_path, private_key_path, client_ca_cert_file_path, client_ca_cert_dir_path)
	, innServer_(loop, listenAddrInn, "GatewayInner")
	, hallConector_(loop)
	, gameConnector_(loop)
	, kTimeoutSeconds_(3)
	, kMaxConnections_(15000)
	, serverState_(kRunning)
	, isdebug_(false) {

	//网络I/O线程池，处理网络I/O读写：数据收/发 recv(read)send(write)
	muduo::net::ReactorSingleton::inst(loop, "RWIOThreadPool");

	//网关服[S]端 <- 客户端[C]端，websocket
	server_.setConnectionCallback(
		std::bind(&Gateway::onConnection, this, std::placeholders::_1));
	server_.setWsConnectedCallback(
		std::bind(&Gateway::onConnected, this, std::placeholders::_1, std::placeholders::_2));
	server_.setWsMessageCallback(
		std::bind(&Gateway::onMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	//网关服[S]端 <- 推送服[C]端，通知服
	innServer_.setConnectionCallback(
		std::bind(&Gateway::onInnConnection, this, std::placeholders::_1));
	innServer_.setMessageCallback(
		std::bind(&Gateway::onInnMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	//网关服[C]端 -> 大厅服[S]端
	hallConector_.setConnectionCallback(
		std::bind(&Gateway::onHallConnection, this, std::placeholders::_1));
	hallConector_.setMessageCallback(
		std::bind(&Gateway::onHallMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

Gateway::~Gateway() {
}

void Gateway::quit() {

}

//zookeeper
bool Gateway::initZookeeper(std::string const& ipaddr) {
	zkclient_.reset(new ZookeeperClient(ipaddr));
	zkclient_->SetConnectedWatcherHandler(
		std::bind(&Gateway::ZookeeperConnectedHandler, this));
	if (!zkclient_->connectServer()) {
		abort();
		return false;
	}
	return true;
}

//RedisCluster
bool Gateway::initRedisCluster(std::string const& ipaddr, std::string const& passwd) {
	
	redisClient_.reset(new RedisClient());
	if (!redisClient_->initRedisCluster(ipaddr, passwd)) {
		return false;
	}
	redisIpaddr_ = ipaddr;
	redisPasswd_ = passwd;
	redisClient_->startSubThread();
	return true;
}

//RedisCluster
bool Gateway::initRedisCluster() {

	if (!REDISCLIENT.initRedisCluster(redisIpaddr_, redisPasswd_)) {
		abort();
		return false;
	}
	return true;
}

//RedisLock
bool Gateway::initRedisLock() {
#if 0
	for (std::vector<std::string>::const_iterator it = redlockVec_.begin();
		it != redlockVec_.end(); ++it) {
		std::vector<std::string> vec;
		boost::algorithm::split(vec, *it, boost::is_any_of(":"));
		LOG_INFO << __FUNCTION__ << " --- *** " << "\nredisLock " << vec[0].c_str() << ":" << vec[1].c_str();
		REDISLOCK.AddServerUrl(vec[0].c_str(), atol(vec[1].c_str()));
	}
#endif
	return true;
}

//MongoDB
bool Gateway::initMongoDB(std::string url) {
#if 0
	//http://mongocxx.org/mongocxx-v3/tutorial/
	LOG_INFO << __FUNCTION__ << " --- *** " << url;
	mongocxx::instance instance{};
	mongoDBUrl_ = url;
#endif
	return true;
}

//MongoDB
bool Gateway::initMongoDB() {
#if 0
	//http://mongocxx.org/mongocxx-v3/tutorial/
	//mongodb://admin:6pd1SieBLfOAr5Po@192.168.0.171:37017,192.168.0.172:37017,192.168.0.173:37017/?connect=replicaSet;slaveOk=true&w=1&readpreference=secondaryPreferred&maxPoolSize=50000&waitQueueMultiple=5
	MongoDBClient::ThreadLocalSingleton::setUri(mongoDBUrl_);
	static __thread mongocxx::database db = MONGODBCLIENT["gamemain"];
	static __thread mongocxx::database* dbgamemain_;
	dbgamemain_ = &db;
#endif
	return true;
}

//MongoDB/RedisCluster/RedisLock
void Gateway::threadInit() {
	initRedisCluster();
	initMongoDB();
	initRedisLock();
}

void Gateway::ZookeeperConnectedHandler() {
	if (ZNONODE == zkclient_->existsNode("/GAME"))
		zkclient_->createNode("/GAME", "Landy");

	if (ZNONODE == zkclient_->existsNode("/GAME/ProxyServers"))
		zkclient_->createNode("/GAME/ProxyServers", "ProxyServers");

	if (ZNONODE == zkclient_->existsNode("/GAME/HallServers"))
		zkclient_->createNode("/GAME/HallServers", "HallServers");

	if (ZNONODE == zkclient_->existsNode("/GAME/HallServersInvaild"))
		zkclient_->createNode("/GAME/HallServersInvaild", "HallServersInvaild");

	if (ZNONODE == zkclient_->existsNode("/GAME/GameServers"))
		zkclient_->createNode("/GAME/GameServers", "GameServers");
	{
		//指定网卡ip:port
		std::vector<std::string> vec;
		//server_ ip:port
		boost::algorithm::split(vec, server_.server_.ipPort(), boost::is_any_of(":"));
		nodeValue_ = strIpAddr_ + ":" + vec[1];
#if 1
		//innServer_ ip:port
		boost::algorithm::split(vec, innServer_.ipPort(), boost::is_any_of(":"));
		nodeValue_ += ":" + vec[1] + ":" + std::to_string(getpid());
#endif
		nodePath_ = "/GAME/ProxyServers/" + nodeValue_;
		//启动时自注册自身节点
		zkclient_->createNode(nodePath_, nodeValue_, true);
	}
	{
		//大厅服 ip:port
		std::vector<std::string> ipaddrs;
		
		//看门狗zk监控节点
		if (ZOK == zkclient_->getClildren(
			"/GAME/HallServers",
			ipaddrs,
			std::bind(
				&Gateway::GetHallChildrenWatcherHandler, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4,
				std::placeholders::_5),
			this)) {
			
			int i = 0;
			for (std::string const& ip : ipaddrs) {
				LOG_ERROR << __FUNCTION__ << " --- *** " << "zkclient_->getClildren hallIps[" << i++ << "] " << ip;
			}
			
			muduo::MutexLockGuard lock(hallIps_mutex_);
			hallIps_.assign(ipaddrs.begin(), ipaddrs.end());
		}
	}

	connectHallServers();
}

void Gateway::connectHallServers() {
	for (std::vector<std::string>::const_iterator it = hallIps_.begin();
		it != hallIps_.end(); ++it) {
		connectHallServer(*it);
	}
}

void Gateway::connectHallServer(std::string const& ipaddr) {
	//获取网络I/O模型EventLoop池
	//std::shared_ptr<muduo::net::EventLoopThreadPool> threadPool = server_.server_.threadPool();
	//std::vector<muduo::net::EventLoop*> loops = threadPool->getAllLoops();

	std::vector<std::string> vec;
	boost::algorithm::split(vec, ipaddr, boost::is_any_of(":"));

	//vec：ip:port
	muduo::net::InetAddress serverAddr(vec[0], stoi(vec[1]));
	//muduo::net::EventLoop* ioLoop = threadPool->getNextLoop();
	hallConector_.check(ipaddr, false);
	hallConector_.create(ipaddr, serverAddr);
}

void Gateway::onHallConnection(const muduo::net::TcpConnectionPtr& conn)
{
	if (conn->connected()) {
		int32_t num = numConnected_.incrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "网关服[" << conn->localAddress().toIpPort() << "] -> 大厅服["
			<< conn->peerAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;
	}
	else {
		int32_t num = numConnected_.decrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "网关服[" << conn->localAddress().toIpPort() << "] -> 大厅服["
			<< conn->peerAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;
	}
}

void Gateway::ProcessHallIps(std::vector<std::string> const& newIps)
{
	std::set<std::string> oldset, newset(newIps.begin(), newIps.end());
	{
		muduo::MutexLockGuard lock(hallIps_mutex_);
		for (std::string const& ip : hallIps_) {
			oldset.emplace(ip);
		}
	}
	//失效节点：hallIps_中有，而newIps中没有
	std::vector<std::string> diff(oldset.size());
	std::vector<std::string>::iterator it;
	it = set_difference(oldset.begin(), oldset.end(), newset.begin(), newset.end(), diff.begin());
	diff.resize(it - diff.begin());
	for (std::string const& ip : diff) {
		//hallIps_中有
		assert(std::find(
			std::begin(hallIps_),
			std::end(hallIps_), ip) != hallIps_.end());
		//newIps中没有
		assert(std::find(
			std::begin(newIps),
			std::end(newIps), ip) == newIps.end());
		hallConector_.check(ip, false);
	}
	//新生节点：newIps中有，而hallIps_中没有
	diff.clear();
	diff.resize(newset.size());
	it = set_difference(newset.begin(), newset.end(), oldset.begin(), oldset.end(), diff.begin());
	diff.resize(it - diff.begin());
	for (std::string const& ip : diff) {
		//hallIps_中没有
		assert(std::find(
			std::begin(hallIps_),
			std::end(hallIps_), ip) == hallIps_.end());
		//newIps中有
		assert(std::find(
			std::begin(newIps),
			std::end(newIps), ip) != newIps.end());
		//连接大厅服
		connectHallServer(ip);
	}
	{
		//添加newIps到hallIps_
		muduo::MutexLockGuard lock(hallIps_mutex_);
		hallIps_.assign(newIps.begin(), newIps.end());
	}
}

void Gateway::GetHallChildrenWatcherHandler(
	int type, int state, const std::shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context)
{
	//大厅服 ip:port
	std::vector<std::string> ipaddrs;
	
	//看门狗zk监控节点
	if (ZOK == zkclient_->getClildren(
		"/GAME/HallServers",
		ipaddrs,
		std::bind(
			&Gateway::GetHallChildrenWatcherHandler, this,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5),
		this)) {

		int i = 0;
		for (std::string const& ip : ipaddrs) {
			LOG_ERROR << __FUNCTION__ << " --- *** " << "zkclient_->getClildren hallIps[" << i++ << "] " << ip;
		}
		ProcessHallIps(ipaddrs);
	}
}

//启动websocket(TCP)业务线程
//启动websocket(TCP)监听，客户端交互
void Gateway::start(int numThreads, int numWorkerThreads, int maxSize)
{
	//网络I/O线程数
	numThreads_ = numThreads;
	/*server_.*/muduo::net::ReactorSingleton::setThreadNum(numThreads);
	
	//创建若干worker线程，启动线程池
	for (int i = 0; i < numWorkerThreads; ++i) {
		std::shared_ptr<muduo::ThreadPool> threadPool = std::make_shared<muduo::ThreadPool>("ThreadPool:" + std::to_string(i));
		threadPool->setThreadInitCallback(std::bind(&Gateway::threadInit, this));
		threadPool->setMaxQueueSize(maxSize);
		threadPool->start(1);
		threadPool_.push_back(threadPool);
	}

	//worker线程数，最好 numWorkerThreads = n * numThreads
	numWorkerThreads_ = numWorkerThreads;

	LOG_INFO << __FUNCTION__ << " --- *** "
		<< "\nGateway = " << server_.server_.ipPort()
		<< " 网络I/O线程数 = " << numThreads
		<< " worker线程数 = " << numWorkerThreads;

	//启动网络I/O线程池，I/O收发读写 recv(read)/send(write)
	muduo::net::ReactorSingleton::start();

	//启动TCP监听客户端，websocket
	//使用ET模式accept/read/write
	server_.start(true);

	//启动TCP监听客户端，内部服务器之间交互
	innServer_.start(true);

	//等server_所有的网络I/O线程都启动起来
	//sleep(2);

	//获取网络I/O模型EventLoop池
	std::shared_ptr<muduo::net::EventLoopThreadPool> threadPool = 
		/*server_.server_.*/muduo::net::ReactorSingleton::threadPool();
	std::vector<muduo::net::EventLoop*> loops = threadPool->getAllLoops();
	
	//为各网络I/O线程绑定Bucket
	for (size_t index = 0; index < loops.size(); ++index) {
		bucketsPool_.push_back(ConnectionBucket(*loops[index], index, kTimeoutSeconds_));
		loops[index]->setContext(EventLoopContext(index));
	}
	
	//为每个网络I/O线程绑定若干worker线程(均匀分配)
	int next = 0;
	for (size_t i = 0; i < threadPool_.size(); ++i) {
		EventLoopContext* context = boost::any_cast<EventLoopContext>(loops[next]->getMutableContext());
		assert(context);
		context->addWorkerIndex(i);
		if (++next >= loops.size()) {
			next = 0;
		}
	}
	
	//定时器超时检查，间隔1s
	for (size_t index = 0; index < loops.size(); ++index) {
		loops[index]->runAfter(1.0f, std::bind(&ConnectionBucket::onTimer, &bucketsPool_[index]));
	}
}

//网关服[S]端 <- 推送服[C]端，通知服
void Gateway::onInnConnection(const muduo::net::TcpConnectionPtr& conn) {
	
	if (conn->connected()) {
		int32_t num = numConnected_.incrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "网关服[" << conn->localAddress().toIpPort() << "] <- 推送服["
			<< conn->peerAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;
	}
	else {
		int32_t num = numConnected_.decrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "网关服[" << conn->localAddress().toIpPort() << "] <- 推送服["
			<< conn->peerAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;
	}
}

//网关服[S]端 <- 推送服[C]端，通知服
void Gateway::onInnMessage(
	const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {

	//解析TCP数据包，先解析包头(header)，再解析包体(body)，避免粘包出现
	while (buf->readableBytes() >= packet::kMinPacketSZ) {

		const uint16_t len = buf->peekInt16();

		//数据包太大或太小
		if (likely(len > packet::kMaxPacketSZ ||
				   len < packet::kMinPacketSZ)) {
			if (conn) {
#if 0
				//不再发送数据
				conn->shutdown();
#else
				//直接强制关闭连接
				conn->forceClose();
#endif
			}
			break;
		}
		//数据包不足够解析，等待下次接收再解析
		else if (likely(len > buf->readableBytes())) {
			break;
		}
		else /*if (likely(len <= buf->readableBytes()))*/ {
			BufferPtr buffer(new muduo::net::Buffer(len));
			buffer->append(buf->peek(), static_cast<size_t>(len));
			buf->retrieve(len);
			packet::internal_prev_header_t* pre_header = (packet::internal_prev_header_t*)buffer->peek();
			std::string session((char const*)pre_header->session, sizeof(pre_header->session));
#if 0
			//////////////////////////////////////////////////////////////////////////
			//session -> hash(session) -> index
			//////////////////////////////////////////////////////////////////////////
			int index = hash_session_(session) % threadPool_.size();
#else
			//////////////////////////////////////////////////////////////////////////
			//session -> entry -> entryContext -> index
			//////////////////////////////////////////////////////////////////////////
			WeakEntryPtr weakEntry;
			{
				READ_LOCK(sessInfos_mutex_);
				SessionInfosMap::const_iterator it = sessInfos_.find(session);
				if (it != sessInfos_.end()) {
					weakEntry = it->second;
				}
			}
			EntryPtr entry(weakEntry.lock());
			if (likely(entry)) {
				Entry::Context* entryContext = boost::any_cast<Entry::Context>(entry->getMutableContext());
				assert(entryContext);

				int index = entryContext->getWorkerIndex();
				assert(index >= 0 && index < threadPool_.size());

				//扔给任务消息队列处理
				threadPool_[index]->run(
					std::bind(
						&Gateway::asyncInnHandler,
						this,
						conn, weakEntry, buffer, receiveTime));
			}
#endif
		}
	}
}

//网关服[S]端 <- 推送服[C]端，通知服
void Gateway::asyncInnHandler(
	const muduo::net::TcpConnectionPtr& conn,
	WeakEntryPtr const& weakEntry,
	BufferPtr& buf,
	muduo::Timestamp receiveTime) {
	//内部消息头internal_prev_header_t + 命令消息头header_t
	if (buf->readableBytes() < packet::kPrevHeaderLen + packet::kHeaderLen) {
		return;
	}
	//内部消息头internal_prev_header_t
	packet::internal_prev_header_t /*const*/* pre_header = (packet::internal_prev_header_t /*const*/*)buf->peek();
	//session
	std::string session((char const*)pre_header->session, sizeof(pre_header->session));
	//userid
	int64_t userId = pre_header->userID;
	EntryPtr entry(weakEntry.lock());
	if (likely(entry)) {
		Entry::Context* entryContext = boost::any_cast<Entry::Context>(entry->getMutableContext());
		assert(entryContext);
		muduo::net::TcpConnectionPtr playerConn(entry->getWeakConnPtr().lock());
		if (likely(playerConn)) {
			//命令消息头header_t
			packet::header_t /*const*/* header = (packet::header_t /*const*/*)(buf->peek() + packet::kPrevHeaderLen);
			//校验CRC
			uint16_t crc = packet::getCheckSum((uint8_t const*)header->ver, packet::kHeaderLen - 4);
			assert(header->crc == crc);
			TraceMessageID(header->mainID, header->subID);
			if (header->mainID == ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL &&
				pre_header->ok == 0) {

			}
			server_.send(playerConn, (uint8_t const*)buf->peek() + packet::kPrevHeaderLen, header->len);
			return;
		}
		LOG_ERROR << __FUNCTION__ << " --- *** " << "playerConn(entry->getWeakConnPtr().lock()) failed";
		return;
	}
	LOG_ERROR << __FUNCTION__ << " --- *** " << "entry(weakEntry.lock()) failed";
}

//网关服[C]端 -> 大厅服[S]端
void Gateway::onHallMessage(const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf,
	muduo::Timestamp receiveTime) {

	//解析TCP数据包，先解析包头(header)，再解析包体(body)，避免粘包出现
	while (buf->readableBytes() >= packet::kMinPacketSZ) {
		
		const uint16_t len = buf->peekInt16();

		//数据包太大或太小
		if (likely(len > packet::kMaxPacketSZ ||
				   len < packet::kMinPacketSZ)) {
			if (conn) {
#if 0
				//不再发送数据
				conn->shutdown();
#else
				//直接强制关闭连接
				conn->forceClose();
#endif
			}
			break;
		}
		//数据包不足够解析，等待下次接收再解析
		else if (likely(len > buf->readableBytes())) {
			break;
		}
		else /*if (likely(len <= buf->readableBytes()))*/ {
			BufferPtr buffer(new muduo::net::Buffer(len));
			buffer->append(buf->peek(), static_cast<size_t>(len));
			buf->retrieve(len);
			packet::internal_prev_header_t* pre_header = (packet::internal_prev_header_t*)buffer->peek();
			std::string session((char const*)pre_header->session, sizeof(pre_header->session));
#if 0
			//////////////////////////////////////////////////////////////////////////
			//session -> hash(session) -> index
			//////////////////////////////////////////////////////////////////////////
			int index = hash_session_(session) % threadPool_.size();
			
			//扔给任务消息队列处理
			threadPool_[index]->run(
				std::bind(
					&Gateway::asyncHallHandler,
					this,
					conn, buffer, receiveTime));
#else
			//////////////////////////////////////////////////////////////////////////
			//session -> entry -> entryContext -> index
			//////////////////////////////////////////////////////////////////////////
			WeakEntryPtr weakEntry;
			{
				READ_LOCK(sessInfos_mutex_);
				SessionInfosMap::const_iterator it = sessInfos_.find(session);
				if (it != sessInfos_.end()) {
					weakEntry = it->second;
				}
			}
			EntryPtr entry(weakEntry.lock());
			if (likely(entry)) {
				Entry::Context* entryContext = boost::any_cast<Entry::Context>(entry->getMutableContext());
				assert(entryContext);

				int index = entryContext->getWorkerIndex();
				assert(index >= 0 && index < threadPool_.size());

				//扔给任务消息队列处理
				threadPool_[index]->run(
					std::bind(
						&Gateway::asyncHallHandler,
						this,
						conn, weakEntry, buffer, receiveTime));
			}
#endif
		}
	}
}
//网关服[C]端 -> 大厅服[S]端
void Gateway::asyncHallHandler(
	const muduo::net::TcpConnectionPtr& conn,
	WeakEntryPtr const& weakEntry,
	BufferPtr& buf,
	muduo::Timestamp receiveTime) {
	//内部消息头internal_prev_header_t + 命令消息头header_t
	if (buf->readableBytes() < packet::kPrevHeaderLen + packet::kHeaderLen) {
		return;
	}
	//内部消息头internal_prev_header_t
	packet::internal_prev_header_t /*const*/* pre_header = (packet::internal_prev_header_t /*const*/*)buf->peek();
	//session
	std::string session((char const*)pre_header->session, sizeof(pre_header->session));
	//userid
	int64_t userId = pre_header->userID;
	EntryPtr entry(weakEntry.lock());
	if (likely(entry)) {
		Entry::Context* entryContext = boost::any_cast<Entry::Context>(entry->getMutableContext());
		assert(entryContext);
		//userid
		entryContext->setUserId(pre_header->userID);
		muduo::net::TcpConnectionPtr playerConn(entry->getWeakConnPtr().lock());
		if (likely(playerConn)) {

			//命令消息头header_t
			packet::header_t /*const*/* header = (packet::header_t /*const*/*)(buf->peek() + packet::kPrevHeaderLen);
			//校验CRC
			uint16_t crc = packet::getCheckSum((uint8_t const*)header->ver, packet::kHeaderLen - 4);
			assert(header->crc == crc);
			TraceMessageID(header->mainID, header->subID);
			if (
				header->mainID == ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL &&
				header->subID == ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_RES &&
				pre_header->ok == 1) {
				//登陆成功，需要处理异地登陆/断线重连/顶号，清除旧的conn
			}
			else if (
				header->mainID == ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL &&
				header->subID == ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_GAME_SERVER_MESSAGE_RES &&
				pre_header->ok == 1) {
				//std::string ip;
				//if (REDISCLIENT.GetUserOnlineInfoIP(userId, ip))
				{
					//muduo::MutexLockGuard lock(m_gameIPServerMapMutex);
					//if (m_gameIPServerMap.count(ip))
					{
						//LOG_DEBUG << " >>> bind game server succeeded, userId:" << userId << ", ip:" << ip;
						//shared_ptr<TcpClient> gameTcpClientPtr = m_gameIPServerMap[ip];
						//weak_ptr<TcpClient>   gameTcpClient(gameTcpClientPtr);
						//if (gameTcpClientPtr)
						//{
						///	//entry->setGameTcpClient(gameTcpClient);
						//}
					}
				}
			}
			server_.send(playerConn, (uint8_t const*)buf->peek() + packet::kPrevHeaderLen, header->len);
			return;
		}
		LOG_ERROR << __FUNCTION__ << " --- *** " << "playerConn(entry->getWeakConnPtr().lock()) failed";
		return;
	}
	LOG_ERROR << __FUNCTION__ << " --- *** " << "entry(weakEntry.lock()) failed";
}

//网关服[C]端 -> 大厅服[S]端
void Gateway::sendHallMessage(WeakEntryPtr const& weakEntry, BufferPtr& buf, int64_t userId) {
	
	EntryPtr entry(weakEntry.lock());
	if (likely(entry)) {
		Entry::Context* entryContext = boost::any_cast<Entry::Context>(entry->getMutableContext());
		assert(entryContext);
		ClientConn const& clientConn = entryContext->getClientConn(servTyE::kHallTy);
		muduo::net::TcpConnectionPtr hallConn(clientConn.second.lock());
		if (hallConn) {
			//用户大厅服有效
			assert(hallConn->connected());
			assert(
				std::find(
				std::begin(hallIps_),
				std::end(hallIps_),
				clientConn.first) == hallIps_.end());
			hallConector_.check(clientConn.first, true);
			hallConn->send(buf.get());
		}
		else {
			//用户大厅服无效，重新分配
			bool bok = false;
			ClientConnList clients;
			//异步函数
			hallConector_.getAll(clients, bok);
			//自旋锁
			while (!bok);
			if (clients.size() > 0) {
				int index = randomHall_.betweenInt(0, clients.size() - 1).randInt_mt();
				assert(index >= 0 && index < hallIps_.size());
				ClientConn const& clientConn = clients[index];
				entryContext->setClientConn(servTyE::kHallTy, clientConn);
				hallConn->send(buf.get());
			}
		}
		return;
	}
	LOG_ERROR << __FUNCTION__ << " --- *** " << "entry(weakEntry.lock()) failed";
}

//createUUID 创建uuid
static std::string createUUID() {
	boost::uuids::random_generator rgen;
	boost::uuids::uuid u = rgen();
	std::string uuid;
	uuid.assign(u.begin(), u.end());
	return uuid;
}

//buffer2HexStr
static std::string buffer2HexStr(unsigned char const* buf, size_t len)
{
	std::ostringstream oss;
	oss << std::hex << std::uppercase << std::setfill('0');
	for (size_t i = 0; i < len; ++i) {
		oss << std::setw(2) << (unsigned int)(buf[i]);
	}
	return oss.str();
}

//网关服[S]端 <- 客户端[C]端，websocket
void Gateway::onConnection(const muduo::net::TcpConnectionPtr& conn) {
	
	if (conn->connected()) {
		int32_t num = numConnected_.incrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "客户端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
			<< conn->localAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;
		
		//累计接收请求数
		numTotalReq_.incrementAndGet();

		//最大连接数限制
		if (num > kMaxConnections_) {
#if 0
			//不再发送数据
			conn->shutdown();
#elif 1
			//直接强制关闭连接
			conn->forceClose();
#else
			//延迟0.2s强制关闭连接
			conn->forceCloseWithDelay(0.2f);
#endif
			//会调用onMessage函数
			assert(conn->getContext().empty());

			//累计未处理请求数
			numTotalBadReq_.incrementAndGet();
			return;
		}
		EventLoopContext* context = boost::any_cast<EventLoopContext>(conn->getLoop()->getMutableContext());
		assert(context);
#ifndef NDEBUG
		EntryPtr entry(new Entry(muduo::net::WeakTcpConnectionPtr(conn), Entry::Context()));
#else
		EntryPtr entry(new Entry(muduo::net::WeakTcpConnectionPtr(conn), Entry::Context(context->allocWorkerIndex())));
#endif
		{
			Entry::Context* entryContext = boost::any_cast<Entry::Context>(entry->getMutableContext());
			assert(entryContext);
#ifndef NDEBUG
			//给新conn绑定一个worker线程，之后所有conn相关逻辑业务都在该worker线程中处理
			int index = context->allocWorkerIndex();
			assert(index >= 0 && index < threadPool_.size());
			entryContext->setWorkerIndex(index);
#endif
		}
		{
			//获取EventLoop关联的Bucket
			int index = context->getBucketIndex();
			assert(index >= 0 && index < bucketsPool_.size());
			//连接成功，压入桶元素
			conn->getLoop()->runInLoop(
				std::bind(&ConnectionBucket::pushBucket, &bucketsPool_[index], entry));
		}
		{
			//指定conn上下文信息
			conn->setContext(WeakEntryPtr(entry));
		}
		{
			//TCP_NODELAY
			conn->setTcpNoDelay(true);
		}
	}
	else {
		int32_t num = numConnected_.decrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "客户端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
			<< conn->localAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;

		WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(conn->getContext()));
		EntryPtr entry(weakEntry.lock());
		if (likely(entry)) {
			Entry::Context* entryContext = boost::any_cast<Entry::Context>(entry->getMutableContext());
			assert(entryContext);
			//userId
			int64_t userId = entryContext->getUserId();
			//session
			std::string const& session = entryContext->getSession();
			{
				//清除玩家session
// 				WRITE_LOCK(sessions_mutex_);
// 				MapSessionPtr::const_iterator it = sessions_.find(session);
// 				if (it != sessions_.end()) {
// 					sessions_.erase(session);
// 				}
			}
		}
	}
}

//网关服[S]端 <- 客户端[C]端，websocket
void Gateway::onConnected(
	const muduo::net::TcpConnectionPtr& conn,
	std::string const& ipaddr) {
	int32_t num = numConnected_.decrementAndGet();
	LOG_INFO << __FUNCTION__ << " --- *** " << "客户端[" << ipaddr << "] -> 网关服["
		<< conn->localAddress().toIpPort() << "] "
		<< (conn->connected() ? "UP" : "DOWN") << " " << num;
	assert(!conn->getContext().empty());
	WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(conn->getContext()));
	EntryPtr entry(weakEntry.lock());
	if (likely(entry)) {
		Entry::Context* entryContext = boost::any_cast<Entry::Context>(entry->getMutableContext());
		assert(entryContext);
		{
			//保存conn真实ipaddr到Entry::Context上下文
			muduo::net::InetAddress address(ipaddr, 0);
			entryContext->setIpaddr(address.ipNetEndian());
		}
		//生成uuid/session
		std::string uuid = createUUID();
		std::string session = buffer2HexStr((unsigned char const*)uuid.data(), uuid.length());
		{
			//conn绑定session
			//优化前，conn->name()断线重连->session变更->重新登陆->异地登陆通知
			//优化后，conn->name()断线重连->session过期检查->登陆校验->异地登陆判断
			entryContext->setSession(session);
		}
		{
			//添加玩家session
			//WRITE_LOCK(sessions_mutex_);
			//sessions_[session] = muduo::net::WeakTcpConnectionPtr(conn);
		}
	}
}

//网关服[S]端 <- 客户端[C]端，websocket
void Gateway::onMessage(
	const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf, int msgType,
	muduo::Timestamp receiveTime) {
	//超过最大连接数限制
	if (!conn || conn->getContext().empty()) {
		return;
	}
	const uint16_t len = buf->peekInt16();
	//数据包太大或太小
	if (likely(len > packet::kMaxPacketSZ ||
			   len < packet::kMinPacketSZ)) {
		if (conn) {
#if 0
			//不再发送数据
			conn->shutdown();
#else
			//直接强制关闭连接
			conn->forceClose();
#endif
		}
		//累计未处理请求数
		numTotalBadReq_.incrementAndGet();
	}
	///数据包不足够解析，等待下次接收再解析
	else if (likely(len > buf->readableBytes())) {
		if (conn) {
#if 0
			//不再发送数据
			conn->shutdown();
#else
			//直接强制关闭连接
			conn->forceClose();
#endif
		}
		//累计未处理请求数
		numTotalBadReq_.incrementAndGet();
	}
	else /*if (likely(len <= buf->readableBytes()))*/ {
	
		WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(conn->getContext()));
		EntryPtr entry(weakEntry.lock());
		if (likely(entry)) {
			{
				EventLoopContext* context = boost::any_cast<EventLoopContext>(conn->getLoop()->getMutableContext());
				assert(context);
				int index = context->getBucketIndex();
				assert(index >= 0 && index < bucketsPool_.size());
				//收到消息包，更新桶元素
				conn->getLoop()->runInLoop(
					std::bind(&ConnectionBucket::updateBucket, &bucketsPool_[index], entry));
			}
			{
				Entry::Context* entryContext = boost::any_cast<Entry::Context>(entry->getMutableContext());
				assert(entryContext);
#if 0
				//理论上session不过期，worker线程不变，实际上每次断线重连，session都会变更
				std::string const& session = entryContext->getSession();
				int index = hash_session_(session) % threadPool_.size();
#else
				//获取绑定的worker线程
				int index = entryContext->getWorkerIndex();
				assert(index >= 0 && index < threadPool_.size());
#endif
#if 0
				BufferPtr buffer(new muduo::net::Buffer(buf->readableBytes()));
				buffer->swap(*buf);
#else
				BufferPtr buffer(new muduo::net::Buffer(buf->readableBytes()));
				buffer->append(buf->peek(), static_cast<size_t>(buf->readableBytes()));
				buf->retrieve(buf->readableBytes());
#endif
				//扔给任务消息队列处理
				threadPool_[index]->run(
					std::bind(&Gateway::asyncClientHandler,
						this, weakEntry, buffer, receiveTime));
			}
			return;
		}
		//累计未处理请求数
		numTotalBadReq_.incrementAndGet();
	}
}

//网关服[S]端 <- 客户端[C]端，websocket 异步回调
void Gateway::asyncClientHandler(
	const WeakEntryPtr& weakEntry,
	BufferPtr& buf,
	muduo::Timestamp receiveTime) {
	if (buf->readableBytes() < packet::kHeaderLen) {
		//累计未处理请求数
		numTotalBadReq_.incrementAndGet();
		return;
	}
	//data, dataLen
	uint8_t const* data = (uint8_t const*)buf->peek();
	size_t dataLen = buf->readableBytes();
	//packet::header_t
	packet::header_t* commandHeader = (packet::header_t*)(&data[0]);
	//CRC校验位
	//command_header.len uint16_t
	//command_header.crc uint16_t
	//command_header.ver ~ command_header.realSize + protobuf
	uint16_t crc = packet::getCheckSum(&data[4], dataLen - 4);

	if (commandHeader->len == dataLen &&
		commandHeader->crc == crc &&
		commandHeader->ver == 1 &&
		commandHeader->sign == HEADER_SIGN) {
		//锁定conn操作
		//刚开始还在想，会不会出现超时conn被异步关闭释放掉，而业务逻辑又被处理了，却发送不了的尴尬情况，
		//假如因为超时entry弹出bucket，引用计数减1，处理业务之前这里使用shared_ptr，持有entry引用计数(加1)，
		//如果持有失败，说明弹出bucket计数减为0，entry被析构释放，conn被关闭掉了，也就不会执行业务逻辑处理，
		//如果持有成功，即使超时entry弹出bucket，引用计数减1，但并没有减为0，entry也就不会被析构释放，conn也不会被关闭，
		//直到业务逻辑处理完并发送，entry引用计数减1变为0，析构被调用关闭conn(如果conn还存在的话，业务处理完也会主动关闭conn)
		EntryPtr entry(weakEntry.lock());
		if (likely(entry)) {
			muduo::net::TcpConnectionPtr conn(entry->getWeakConnPtr().lock());
			if (conn) {
 				//mainID
 				switch (commandHeader->mainID) {
 				case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PROXY: {
 					//网关服(Gateway)
 					switch (commandHeader->enctype) {
					case packet::PUBENC_PROTOBUF_NONE: {
						//NONE
						TraceMessageID(commandHeader->mainID, commandHeader->subID);
						break;
					}
					case packet::PUBENC_PROTOBUF_RSA: {
						//RSA
						TraceMessageID(commandHeader->mainID, commandHeader->subID);
						break;
					}
 					case packet::PUBENC_PROTOBUF_AES: {
 						//AES
						TraceMessageID(commandHeader->mainID, commandHeader->subID);
 						break;
 					}
					default: {
						//累计未处理请求数
						numTotalBadReq_.incrementAndGet();
						break;
					}
 					}
 					break;
 				}
				case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL: {
					//大厅服(HallS)
					TraceMessageID(commandHeader->mainID, commandHeader->subID);
					{
						Entry::Context* entryContext = boost::any_cast<Entry::Context>(entry->getMutableContext());
						assert(entryContext);
						
						int64_t userId = entryContext->getUserId();
						
						std::string const& session = entryContext->getSession();
						std::string const& aesKey = entryContext->getAesKey();
						
						uint32_t clientip = entryContext->getIpaddr();

						ClientConn const& clientConn = entryContext->getClientConn(servTyE::kHallTy);
						muduo::net::TcpConnectionPtr hallConn(clientConn.second.lock());
						//
						assert(commandHeader->len == buf->readableBytes());
						uint16_t len = packet::kPrevHeaderLen + commandHeader->len;
						packet::internal_prev_header_t pre_header = { 0 };
						memset(&pre_header, 0, packet::kPrevHeaderLen);
						pre_header.len = len;
						pre_header.userID = userId;
						pre_header.ipaddr = clientip;
						memcpy(pre_header.session, session.c_str(), sizeof(pre_header.session));
						memcpy(pre_header.aesKey, aesKey.c_str(), min(sizeof(pre_header.aesKey), aesKey.size()));
#if 0
						//////////////////////////////////////////////////////////////////////////
						//玩家登陆网关服信息
						//使用hash	h.usr:proxy[1001] = session|ip:port:port:pid<弃用>
						//使用set	s.uid:1001:proxy = session|ip:port:port:pid<使用>
						//网关服ID格式：session|ip:port:port:pid
						//第一个ip:port是网关服监听客户端的标识
						//第二个ip:port是网关服监听订单服的标识
						//pid标识网关服进程id
						//////////////////////////////////////////////////////////////////////////
						//网关服servid session|ip:port:port:pid
						std::string const& svrid = nodeValue_;
						assert(svrid.length() <= sizeof(pre_header.servID));
						memcpy(pre_header.servID, svrid.c_str(), std::min(sizeof(pre_header.servID), svrid.length()));
#endif
						packet::setCheckSum(&pre_header);
						BufferPtr buffer(new muduo::net::Buffer(len));
						buffer->append((const char*)&pre_header, packet::kPrevHeaderLen);
						buffer->append(buf->peek(), commandHeader->len);
						//发送大厅消息
						sendHallMessage(weakEntry, buffer, userId);
					}
					break;
				}
				case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER: {
					//游戏服(GameS)
					TraceMessageID(commandHeader->mainID, commandHeader->subID);
					break;
				}
				case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC: {
					//逻辑服(LogicS，逻辑子游戏libGame_xxx.so)
					TraceMessageID(commandHeader->mainID, commandHeader->subID);
					break;
				}
				default: {
					//累计未处理请求数
					numTotalBadReq_.incrementAndGet();
					break;
				}
 				}
				return;
 			}
		}
	}
	//累计未处理请求数
	numTotalBadReq_.incrementAndGet();
}