/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#include <sstream>
#include <fstream>
#include <functional>
#include <sys/types.h>

#include <muduo/net/Reactor.h>
#include <muduo/net/libwebsocket/context.h>
#include <muduo/net/libwebsocket/server.h>
#include <muduo/net/libwebsocket/ssl.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/proto/detail/ignore_unused.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time.hpp>
#include <boost/thread.hpp>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include "proto/Game.Common.pb.h"
#include "proto/ProxyServer.Message.pb.h"
#include "proto/HallServer.Message.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "public/SubNetIP.h"
#include "public/NetCardIP.h"
#include "public/utils.h"

#include "public/codec/aes.h"
#include "public/codec/mymd5.h"
#include "public/codec/base64.h"
#include "public/codec/htmlcodec.h"
#include "public/codec/urlcodec.h"

#include "Gateway.h"

Gateway::Gateway(muduo::net::EventLoop* loop,
	const muduo::net::InetAddress& listenAddr,
	const muduo::net::InetAddress& listenAddrInn,
	const muduo::net::InetAddress& listenAddrHttp,
	std::string const& cert_path, std::string const& private_key_path,
	std::string const& client_ca_cert_file_path,
	std::string const& client_ca_cert_dir_path)
    : server_(loop, listenAddr, "Gateway[client]")
	, innServer_(loop, listenAddrInn, "Gateway[inner]")
	, httpServer_(loop, listenAddrHttp, "Gateway[http]")
	, hallClients_(loop)
	, gameClients_(loop)
	, kTimeoutSeconds_(3)
	, kHttpTimeoutSeconds_(3)
	, kMaxConnections_(15000)
	, serverState_(ServiceStateE::kRunning)
	, threadTimer_(new muduo::net::EventLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "EventLoopThreadTimer"))
	, isdebug_(false) {

	init();

	//网络I/O线程池，I/O收发读写 recv(read)/send(write)
	muduo::net::ReactorSingleton::inst(loop, "RWIOThreadPool");

	//网关服[S]端 <- 客户端[C]端，websocket
	server_.setConnectionCallback(
		std::bind(&Gateway::onConnection, this, std::placeholders::_1));
	server_.setMessageCallback(
		std::bind(&muduo::net::websocket::onMessage,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	
	//网关服[S]端 <- 推送服[C]端，推送通知服务
	innServer_.setConnectionCallback(
		std::bind(&Gateway::onInnConnection, this, std::placeholders::_1));
	innServer_.setMessageCallback(
		std::bind(&Gateway::onInnMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	
	//网关服[S]端 <- HTTP客户端[C]端，WEB前端
	httpServer_.setConnectionCallback(
		std::bind(&Gateway::onHttpConnection, this, std::placeholders::_1));
	httpServer_.setMessageCallback(
		std::bind(&Gateway::onHttpMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	
	//网关服[C]端 -> 大厅服[S]端，内部交互
	hallClients_.setConnectionCallback(
		std::bind(&Gateway::onHallConnection, this, std::placeholders::_1));
	hallClients_.setMessageCallback(
		std::bind(&Gateway::onHallMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	clients_[servTyE::kHallTy].clients_ = &hallClients_;
	clients_[servTyE::kHallTy].ty_ = servTyE::kHallTy;

	//网关服[C]端 -> 游戏服[S]端，内部交互
	gameClients_.setConnectionCallback(
		std::bind(&Gateway::onGameConnection, this, std::placeholders::_1));
	gameClients_.setMessageCallback(
		std::bind(&Gateway::onGameMessage, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	clients_[servTyE::kGameTy].clients_ = &gameClients_;
	clients_[servTyE::kGameTy].ty_ = servTyE::kGameTy;

	//添加OpenSSL认证支持 httpServer_&server_ 共享证书
	muduo::net::ssl::SSL_CTX_Init(
		cert_path,
		private_key_path,
		client_ca_cert_file_path, client_ca_cert_dir_path);

	//指定SSL_CTX
	server_.set_SSL_CTX(muduo::net::ssl::SSL_CTX_Get());
	httpServer_.set_SSL_CTX(muduo::net::ssl::SSL_CTX_Get());
}

Gateway::~Gateway() {
}

void Gateway::init() {
	handlers_[packet::enword(
		::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::Game::Common::CLIENT_TO_PROXY_GET_AES_KEY_MESSAGE_REQ)] =
		std::bind(&Gateway::onGetAesKey, this, std::placeholders::_1, std::placeholders::_2);
}

void Gateway::quit() {
	hallClients_.closeAll();
	gameClients_.closeAll();
	for (size_t i = 0; i < threadPool_.size(); ++i) {
		threadPool_[i]->stop();
	}
	if (zkclient_) {
		zkclient_->closeServer();
	}
	if (redisClient_) {
		redisClient_->unsubscribe();
	}
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
	//跨网关顶号处理(异地登陆)
	redisClient_->subscribeUserLoginMessage(bind(&Gateway::onUserReLoginNotify, this, std::placeholders::_1));
	//跑马灯通告消息
	redisClient_->subscribePublishMsg(1, CALLBACK_1(Gateway::onMarqueeNotify, this));
	//幸运转盘消息
	redisClient_->subscribePublishMsg(0, [&](std::string const& msg) {
		threadTimer_->getLoop()->runAfter(10, std::bind(&Gateway::onLuckPushNotify, this, msg));
		});
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
	//网关自己
	if (ZNONODE == zkclient_->existsNode("/GAME/ProxyServers"))
		zkclient_->createNode("/GAME/ProxyServers", "ProxyServers");
	//大厅
	if (ZNONODE == zkclient_->existsNode("/GAME/HallServers"))
		zkclient_->createNode("/GAME/HallServers", "HallServers");
	//大厅维护
	//if (ZNONODE == zkclient_->existsNode("/GAME/HallServersInvalid"))
	//	zkclient_->createNode("/GAME/HallServersInvalid", "HallServersInvalid");
	//游戏
	if (ZNONODE == zkclient_->existsNode("/GAME/GameServers"))
		zkclient_->createNode("/GAME/GameServers", "GameServers");
	//游戏维护
	//if (ZNONODE == zkclient_->existsNode("/GAME/GameServersInvalid"))
	//	zkclient_->createNode("/GAME/GameServersInvalid", "GameServersInvalid");
	{
		//指定网卡ip:port
		std::vector<std::string> vec;
		//server_ ip:port
		boost::algorithm::split(vec, server_.ipPort(), boost::is_any_of(":"));
		nodeValue_ = strIpAddr_ + ":" + vec[1];
#if 1
		//innServer_ ip:port
		boost::algorithm::split(vec, innServer_.ipPort(), boost::is_any_of(":"));
		nodeValue_ += ":" + vec[1] + ":" + std::to_string(getpid());
#endif
		nodePath_ = "/GAME/ProxyServers/" + nodeValue_;
		//启动时自注册自身节点
		zkclient_->createNode(nodePath_, nodeValue_, false);
		//挂维护中的节点
		invalidNodePath_ = "/GAME/ProxyServersInvalid/" + nodeValue_;
	}
	{
		//大厅服 ip:port
		std::vector<std::string> names;
		if (ZOK == zkclient_->getClildren(
			"/GAME/HallServers",
			names,
			std::bind(
				&Gateway::onHallWatcherHandler, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4,
				std::placeholders::_5),
			this)) {
			
			clients_[servTyE::kHallTy].add(names);
		}
	}
	{
		//游戏服 ip:port
		std::vector<std::string> names;
		if (ZOK == zkclient_->getClildren(
			"/GAME/GameServers",
			names,
			std::bind(
				&Gateway::onGameWatcherHandler, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4,
				std::placeholders::_5),
			this)) {

			clients_[servTyE::kGameTy].add(names);
		}
	}
}

void Gateway::onHallWatcherHandler(
	int type, int state, const std::shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context)
{
	LOG_ERROR << __FUNCTION__;
	//大厅服 ip:port
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/HallServers",
		names,
		std::bind(
			&Gateway::onHallWatcherHandler, this,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5),
		this)) {

		clients_[servTyE::kHallTy].process(names);
	}
}

void Gateway::onGameWatcherHandler(
	int type, int state, const std::shared_ptr<ZookeeperClient>& zkClientPtr,
	const std::string& path, void* context)
{
	LOG_ERROR << __FUNCTION__;
	//游戏服 roomid:ip:port
	std::vector<std::string> names;
	if (ZOK == zkclient_->getClildren(
		"/GAME/GameServers",
		names,
		std::bind(
			&Gateway::onGameWatcherHandler, this,
			std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5),
		this)) {

		clients_[servTyE::kGameTy].process(names);
	}
}

//启动worker线程
//启动TCP监听客户端，websocket
//启动TCP监听客户端，内部推送通知服务
//启动TCP监听客户端，HTTP
void Gateway::start(int numThreads, int numWorkerThreads, int maxSize)
{
	//网络I/O线程数
	numThreads_ = numThreads;
	muduo::net::ReactorSingleton::setThreadNum(numThreads);
	//启动网络I/O线程池，I/O收发读写 recv(read)/send(write)
	muduo::net::ReactorSingleton::start();

	//worker线程数，最好 numWorkerThreads = n * numThreads
	numWorkerThreads_ = numWorkerThreads;
	//创建若干worker线程，启动worker线程池
	for (int i = 0; i < numWorkerThreads; ++i) {
		std::shared_ptr<muduo::ThreadPool> threadPool = std::make_shared<muduo::ThreadPool>("ThreadPool:" + std::to_string(i));
		threadPool->setThreadInitCallback(std::bind(&Gateway::threadInit, this));
		threadPool->setMaxQueueSize(maxSize);
		threadPool->start(1);
		threadPool_.push_back(threadPool);
	}

	LOG_INFO << __FUNCTION__ << " --- *** "
		<< "\nGateway = " << server_.ipPort()
		<< " 网络I/O线程数 = " << numThreads
		<< " worker线程数 = " << numWorkerThreads;

	//Accept时候判断，socket底层控制，否则开启异步检查
	if (blackListControl_ == IpVisitCtrlE::kOpenAccept) {
		server_.setConditionCallback(std::bind(&Gateway::onCondition, this, std::placeholders::_1));
	}

	//Accept时候判断，socket底层控制，否则开启异步检查
	if (whiteListControl_ == IpVisitCtrlE::kOpenAccept) {
		httpServer_.setConditionCallback(std::bind(&Gateway::onHttpCondition, this, std::placeholders::_1));
	}

	//启动TCP监听客户端，websocket
	//使用ET模式accept/read/write
	server_.start(true);

	//启动TCP监听客户端，内部推送通知服务
	//使用ET模式accept/read/write
	innServer_.start(true);

	//启动TCP监听客户端，HTTP
	//使用ET模式accept/read/write
	httpServer_.start(true);

	//等server_所有的网络I/O线程都启动起来
	//sleep(2);

	//获取网络I/O模型EventLoop池
	std::shared_ptr<muduo::net::EventLoopThreadPool> threadPool = 
		/*server_.*/muduo::net::ReactorSingleton::threadPool();
	std::vector<muduo::net::EventLoop*> loops = threadPool->getAllLoops();
	
	//为各网络I/O线程绑定Bucket
	for (size_t index = 0; index < loops.size(); ++index) {
#if 0
		ConnBucketPtr bucket(new ConnBucket(
				loops[index], index, kTimeoutSeconds_));
		bucketsPool_.emplace_back(std::move(bucket));
#else
		bucketsPool_.emplace_back(
			ConnBucketPtr(new ConnBucket(
			loops[index], index, kTimeoutSeconds_)));
#endif
		loops[index]->setContext(EventLoopContextPtr(new EventLoopContext(index)));
	}
	//为每个网络I/O线程绑定若干worker线程(均匀分配)
	{
		int next = 0;
		for (size_t index = 0; index < threadPool_.size(); ++index) {
			EventLoopContextPtr context = boost::any_cast<EventLoopContextPtr>(loops[next]->getContext());
			assert(context);
			context->addWorkerIndex(index);
			if (++next >= loops.size()) {
				next = 0;
			}
		}
	}
	//启动连接超时定时器检查，间隔1s
	for (size_t index = 0; index < loops.size(); ++index) {
		{
			assert(bucketsPool_[index]->index_ == index);
			assert(bucketsPool_[index]->loop_ == loops[index]);
		}
		{
			EventLoopContextPtr context = boost::any_cast<EventLoopContextPtr>(loops[index]->getContext());
			assert(context->index_ == index);
		}
		loops[index]->runAfter(1.0f, std::bind(&ConnBucket::onTimer, bucketsPool_[index].get()));
	}

	//公共定时器
	threadTimer_->startLoop();
}

//网关服[S]端 <- 推送服[C]端，推送通知服务
void Gateway::onInnConnection(const muduo::net::TcpConnectionPtr& conn) {

	conn->getLoop()->assertInLoopThread();

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

//网关服[S]端 <- 推送服[C]端，推送通知服务
void Gateway::onInnMessage(
	const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {

	conn->getLoop()->assertInLoopThread();

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
			assert(!session.empty() && session.size() == packet::kSessionSZ);
#if 0
			//////////////////////////////////////////////////////////////////////////
			//session -> hash(session) -> index
			//////////////////////////////////////////////////////////////////////////
			int index = hash_session_(session) % threadPool_.size();
#else
			//////////////////////////////////////////////////////////////////////////
			//session -> conn -> entryContext -> index
			//////////////////////////////////////////////////////////////////////////
			muduo::net::WeakTcpConnectionPtr weakConn = entities_.get(session);
			muduo::net::TcpConnectionPtr peer(weakConn.lock());
			if (peer) {
				ContextPtr entryContext(boost::any_cast<ContextPtr>(peer->getContext()));
				assert(entryContext);

				int index = entryContext->getWorkerIndex();
				assert(index >= 0 && index < threadPool_.size());

				//扔给任务消息队列处理
				threadPool_[index]->run(
					std::bind(
						&Gateway::asyncInnHandler,
						this,
						weakConn, buffer, receiveTime));
			}
#endif
		}
	}
}

//网关服[S]端 <- 推送服[C]端，推送通知服务
void Gateway::asyncInnHandler(
	muduo::net::WeakTcpConnectionPtr const& weakConn,
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
	assert(!session.empty() && session.size() == packet::kSessionSZ);
	//userid
	int64_t userid = pre_header->userID;
	muduo::net::TcpConnectionPtr peer(weakConn.lock());
	if (peer) {
		//命令消息头header_t
		packet::header_t /*const*/* header = (packet::header_t /*const*/*)(buf->peek() + packet::kPrevHeaderLen);
			
		//校验CRC header->len = packet::kHeaderLen + len
		uint16_t crc = packet::getCheckSum((uint8_t const*)&header->ver, header->len - 4);
		assert(header->crc == crc);
			
		TraceMessageID(header->mainID, header->subID);
			
		if (header->mainID == ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL &&
			pre_header->ok == 0) {

		}
		muduo::net::websocket::send(peer, (uint8_t const*)buf->peek() + packet::kPrevHeaderLen, header->len);
		return;
	}
	LOG_ERROR << __FUNCTION__ << " --- *** " << "peer(entry->getWeakConnPtr().lock()) failed";
}

//网关服[S]端 <- 推送服[C]端，推送通知服务
void Gateway::onMarqueeNotify(std::string const& msg) {
	LOG_WARN << "跑马灯消息\n" << msg;
	broadcastNoticeMsg("跑马灯消息", msg, 0, 2);
}

//网关服[S]端 <- 推送服[C]端，推送通知服务
void Gateway::onLuckPushNotify(std::string const& msg) {
	LOG_WARN << "免费送金消息\n" << msg;
	broadcastNoticeMsg("免费送金消息", msg, 0, 1);
}

//网关服[S]端 <- HTTP客户端[C]端，WEB前端
bool Gateway::onHttpCondition(const muduo::net::InetAddress& peerAddr) {
	//Accept时候判断，socket底层控制，否则开启异步检查
	assert(whiteListControl_ == IpVisitCtrlE::kOpenAccept);
	//安全断言
	httpServer_.getLoop()->assertInLoopThread();
	{
		//管理员挂维护/恢复服务
		std::map<in_addr_t, IpVisitE>::const_iterator it = adminList_.find(peerAddr.ipNetEndian());
		if (it != adminList_.end()) {
			return true;
		}
	}
	{
		//192.168.2.21:3640 192.168.2.21:3667
		std::map<in_addr_t, IpVisitE>::const_iterator it = whiteList_.find(peerAddr.ipNetEndian());
		return (it != whiteList_.end()) && (IpVisitE::kEnable == it->second);
	}
#if 0
	//节点维护中
	if (serverState_ == ServiceStateE::kRepairing) {
		return false;
	}
#endif
	return true;
}

//网关服[S]端 <- HTTP客户端[C]端，WEB前端
void Gateway::onHttpConnection(const muduo::net::TcpConnectionPtr& conn) {

	conn->getLoop()->assertInLoopThread();

	if (conn->connected()) {
		int32_t num = numConnected_.incrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "WEB前端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
			<< conn->localAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;

		//累计接收请求数
		numTotalReq_.incrementAndGet();

		//最大连接数限制
		if (num > kMaxConnections_) {
#if 0
			//不再发送数据
			conn->shutdown();
#elif 0
			//直接强制关闭连接
			conn->forceClose();
#else
			//HTTP应答包(header/body)
			muduo::net::HttpResponse rsp(false);
			setFailedResponse(rsp,
				muduo::net::HttpResponse::k404NotFound,
				"HTTP/1.1 600 访问量限制(" + std::to_string(kMaxConnections_) + ")\r\n\r\n");
			muduo::net::Buffer buf;
			rsp.appendToBuffer(&buf);
			conn->send(&buf);

			//延迟0.2s强制关闭连接
			conn->forceCloseWithDelay(0.2f);
#endif
			//会调用onHttpMessage函数
			assert(conn->getContext().empty());

			//累计未处理请求数
			numTotalBadReq_.incrementAndGet();
			return;
		}
		EventLoopContextPtr context = boost::any_cast<EventLoopContextPtr>(conn->getLoop()->getContext());
		assert(context);

		EntryPtr entry(new Entry(muduo::net::WeakTcpConnectionPtr(conn)));
		
		//指定conn上下文信息
		ContextPtr entryContext(new Context(WeakEntryPtr(entry), muduo::net::HttpContext()));
		conn->setContext(entryContext);
		{
			//给新conn绑定一个worker线程，之后所有conn相关逻辑业务都在该worker线程中处理
			int index = context->allocWorkerIndex();
			assert(index >= 0 && index < threadPool_.size());

			//对于HTTP请求来说，每一个conn都应该是独立的，指定一个独立线程处理即可，避免锁开销与多线程竞争抢占共享资源带来的性能损耗
			entryContext->setWorkerIndex(index);
		}
		{
			//获取EventLoop关联的Bucket
			int index = context->getBucketIndex();
			assert(index >= 0 && index < bucketsPool_.size());
			//连接成功，压入桶元素
			conn->getLoop()->runInLoop(
				std::bind(&ConnBucket::pushBucket, bucketsPool_[index].get(), entry));
		}
		{
			//TCP_NODELAY
			conn->setTcpNoDelay(true);
		}
	}
	else {
		int32_t num = numConnected_.decrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "WEB前端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
			<< conn->localAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;
	}
}

//网关服[S]端 <- HTTP客户端[C]端，WEB前端
void Gateway::onHttpMessage(
	const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {

	conn->getLoop()->assertInLoopThread();

	//超过最大连接数限制
	if (!conn || conn->getContext().empty()) {
		//LOG_ERROR << __FUNCTION__ << " --- *** " << "TcpConnectionPtr.conn max";
		return;
	}

	//LOG_ERROR << __FUNCTION__ << " --- *** ";
	//printf("----------------------------------------------\n");
	//printf("%.*s\n", buf->readableBytes(), buf->peek());

	//先确定是HTTP数据报文，再解析 ///
	//assert(buf->readableBytes() > 4 && buf->findCRLFCRLF());

	ContextPtr entryContext(boost::any_cast<ContextPtr>(conn->getContext()));
	assert(entryContext);
	muduo::net::HttpContext* httpContext = boost::any_cast<muduo::net::HttpContext>(entryContext->getMutableContext());
	assert(httpContext);
	//解析HTTP数据包
	if (!httpContext->parseRequest(buf, receiveTime)) {
		//发生错误
	}
	else if (httpContext->gotAll()) {
		//Accept时候判断，socket底层控制，否则开启异步检查
		if (whiteListControl_ == IpVisitCtrlE::kOpen) {
			std::string ipaddr;
			{
				std::string ipaddrs = httpContext->request().getHeader("X-Forwarded-For");
				if (ipaddrs.empty()) {
					ipaddr = conn->peerAddress().toIp();
				}
				else {
#if 0
					//第一个IP为客户端真实IP，可伪装，第二个IP为一级代理IP，第三个IP为二级代理IP
					std::string::size_type spos = ipaddrs.find_first_of(',');
					if (spos == std::string::npos) {
					}
					else {
						ipaddr = ipaddrs.substr(0, spos);
					}
#else
					boost::replace_all<std::string>(ipaddrs, " ", "");
					std::vector<std::string> vec;
					boost::algorithm::split(vec, ipaddrs, boost::is_any_of(","));
					for (std::vector<std::string>::const_iterator it = vec.begin();
						it != vec.end(); ++it) {
						if (!it->empty() &&
							boost::regex_match(*it, boost::regex(
								"^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\." \
								"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
								"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
								"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$"))) {

							if (strncasecmp(it->c_str(), "10.", 3) != 0 &&
								strncasecmp(it->c_str(), "192.168", 7) != 0 &&
								strncasecmp(it->c_str(), "172.16.", 7) != 0) {
								ipaddr = *it;
								break;
							}
						}
					}
#endif
				}
			}
			muduo::net::InetAddress peerAddr(muduo::StringArg(ipaddr), 0, false);
			bool is_ip_allowed = false;
			{
				//管理员挂维护/恢复服务
				std::map<in_addr_t, IpVisitE>::const_iterator it = adminList_.find(peerAddr.ipNetEndian());
				is_ip_allowed = (it != adminList_.end());
			}
			if (!is_ip_allowed) {
				READ_LOCK(whiteList_mutex_);
				std::map<in_addr_t, IpVisitE>::const_iterator it = whiteList_.find(peerAddr.ipNetEndian());
				is_ip_allowed = ((it != whiteList_.end()) && (IpVisitE::kEnable == it->second));
			}
			if (!is_ip_allowed) {
#if 0
				//不再发送数据
				conn->shutdown();
#elif 1
				//直接强制关闭连接
				conn->forceClose();
#else
				//HTTP应答包(header/body)
				muduo::net::HttpResponse rsp(false);
				setFailedResponse(rsp,
					muduo::net::HttpResponse::k404NotFound,
					"HTTP/1.1 500 IP访问限制\r\n\r\n");
				muduo::net::Buffer buf;
				rsp.appendToBuffer(&buf);
				conn->send(&buf);

				//延迟0.2s强制关闭连接
				conn->forceCloseWithDelay(0.2f);
#endif
				//累计未处理请求数
				numTotalBadReq_.incrementAndGet();
				return;
			}
		}
		EntryPtr entry(entryContext->getWeakEntryPtr().lock());
		if (likely(entry)) {
			{
				EventLoopContextPtr context = boost::any_cast<EventLoopContextPtr>(conn->getLoop()->getContext());
				assert(context);
				int index = context->getBucketIndex();
				assert(index >= 0 && index < bucketsPool_.size());

				//收到消息包，更新桶元素
				conn->getLoop()->runInLoop(std::bind(&ConnBucket::updateBucket, bucketsPool_[index].get(), entry));
			}
			{
				//获取绑定的worker线程
				int index = entryContext->getWorkerIndex();
				assert(index >= 0 && index < threadPool_.size());

				//扔给任务消息队列处理
				threadPool_[index]->run(
					std::bind(
						&Gateway::asyncHttpHandler,
						this, muduo::net::WeakTcpConnectionPtr(conn), receiveTime));
			}
			return;
		}
		//累计未处理请求数
		numTotalBadReq_.incrementAndGet();
		LOG_ERROR << __FUNCTION__ << " --- *** " << "entry invalid";
		return;
	}
	//发生错误
	//HTTP应答包(header/body)
	muduo::net::HttpResponse rsp(false);
	setFailedResponse(rsp,
		muduo::net::HttpResponse::k404NotFound,
		"HTTP/1.1 400 Bad Request\r\n\r\n");
	muduo::net::Buffer buffer;
	rsp.appendToBuffer(&buffer);
	conn->send(&buffer);
	//释放HttpContext资源
	httpContext->reset();
#if 0
	//不再发送数据
	conn->shutdown();
#elif 0
	//直接强制关闭连接
	conn->forceClose();
#else
	//延迟0.2s强制关闭连接
	conn->forceCloseWithDelay(0.2f);
#endif
	//累计未处理请求数
	numTotalBadReq_.incrementAndGet();
}

//网关服[S]端 <- HTTP客户端[C]端，WEB前端
void Gateway::asyncHttpHandler(muduo::net::WeakTcpConnectionPtr const& weakConn, muduo::Timestamp receiveTime) {
	//锁定conn操作
	//刚开始还在想，会不会出现超时conn被异步关闭释放掉，而业务逻辑又被处理了，却发送不了的尴尬情况，
	//假如因为超时entry弹出bucket，引用计数减1，处理业务之前这里使用shared_ptr，持有entry引用计数(加1)，
	//如果持有失败，说明弹出bucket计数减为0，entry被析构释放，conn被关闭掉了，也就不会执行业务逻辑处理，
	//如果持有成功，即使超时entry弹出bucket，引用计数减1，但并没有减为0，entry也就不会被析构释放，conn也不会被关闭，
	//直到业务逻辑处理完并发送，entry引用计数减1变为0，析构被调用关闭conn(如果conn还存在的话，业务处理完也会主动关闭conn)
	muduo::net::TcpConnectionPtr conn(weakConn.lock());
	if (conn) {
#if 1
		//Accept时候判断，socket底层控制，否则开启异步检查
		if (whiteListControl_ == IpVisitCtrlE::kOpen) {
			bool is_ip_allowed = false;
			{
				READ_LOCK(whiteList_mutex_);
				std::map<in_addr_t, IpVisitE>::const_iterator it = whiteList_.find(conn->peerAddress().ipNetEndian());
				is_ip_allowed = ((it != whiteList_.end()) && (IpVisitE::kEnable == it->second));
			}
			if (!is_ip_allowed) {
#if 0
				//不再发送数据
				conn->shutdown();
#elif 0
				//直接强制关闭连接
				conn->forceClose();
#else
				//HTTP应答包(header/body)
				muduo::net::HttpResponse rsp(false);
				setFailedResponse(rsp,
					muduo::net::HttpResponse::k404NotFound,
					"HTTP/1.1 500 IP访问限制\r\n\r\n");
				muduo::net::Buffer buf;
				rsp.appendToBuffer(&buf);
				conn->send(&buf);

				//延迟0.2s强制关闭连接
				conn->forceCloseWithDelay(0.2f);
#endif
				return;
			}
		}
#endif
		ContextPtr entryContext(boost::any_cast<ContextPtr>(conn->getContext()));
		assert(entryContext);
		muduo::net::HttpContext* httpContext = boost::any_cast<muduo::net::HttpContext>(entryContext->getMutableContext());
		assert(httpContext);
		assert(httpContext->gotAll());
		const string& connection = httpContext->request().getHeader("Connection");
		//是否保持HTTP长连接
		bool close = (connection == "close") ||
			(httpContext->request().getVersion() == muduo::net::HttpRequest::kHttp10 && connection != "Keep-Alive");
		//HTTP应答包(header/body)
		muduo::net::HttpResponse rsp(close);
		//请求处理回调，但非线程安全的
		processHttpRequest(httpContext->request(), rsp, conn->peerAddress(), receiveTime);
		//应答消息
		{
			muduo::net::Buffer buf;
			rsp.appendToBuffer(&buf);
			conn->send(&buf);
		}
		//非HTTP长连接则关闭
		if (rsp.closeConnection()) {
#if 0
			//不再发送数据
			conn->shutdown();
#elif 0
			//直接强制关闭连接
			conn->forceClose();
#else
			//延迟0.2s强制关闭连接
			conn->forceCloseWithDelay(0.2f);
#endif
		}
		//释放HttpContext资源
		httpContext->reset();
	}
	else {
		//累计未处理请求数
		numTotalBadReq_.incrementAndGet();
		//LOG_ERROR << __FUNCTION__ << " --- *** " << "TcpConnectionPtr.conn invalid";
	}
}

//网关服[S]端 <- HTTP客户端[C]端，WEB前端
std::string Gateway::getRequestStr(muduo::net::HttpRequest const& req) {
	std::string headers;
	for (std::map<string, string>::const_iterator it = req.headers().begin();
		it != req.headers().end(); ++it) {
		headers += it->first + ": " + it->second + "\n";
	}
	std::stringstream ss;
	ss << "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
		<< "<xs:root xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
		<< "<xs:head>" << headers << "</xs:head>"
		<< "<xs:body>"
		<< "<xs:method>" << req.methodString() << "</xs:method>"
		<< "<xs:path>" << req.path() << "</xs:path>"
		<< "<xs:query>" << HTML::Encode(req.query()) << "</xs:query>"
		<< "</xs:body>"
		<< "</xs:root>";
	return ss.str();
}

//网关服[S]端 <- HTTP客户端[C]端，WEB前端
bool Gateway::parseQuery(std::string const& queryStr, HttpParams& params, std::string& errmsg) {
	params.clear();
	LOG_DEBUG << "--- *** " << "\n" << queryStr;
	do {
		std::string subStr;
		std::string::size_type npos = queryStr.find_first_of('?');
		if (npos != std::string::npos) {
			//skip '?' ///
			subStr = queryStr.substr(npos + 1, std::string::npos);
		}
		else {
			subStr = queryStr;
		}
		if (subStr.empty()) {
			break;
		}
		for (;;) {
			//key value separate ///
			std::string::size_type kpos = subStr.find_first_of('=');
			if (kpos == std::string::npos) {
				break;
			}
			//next start ///
			std::string::size_type spos = subStr.find_first_of('&');
			if (spos == std::string::npos) {
				std::string key = subStr.substr(0, kpos);
				//skip '=' ///
				std::string val = subStr.substr(kpos + 1, std::string::npos);
				params[key] = val;
				break;
			}
			else if (kpos < spos) {
				std::string key = subStr.substr(0, kpos);
				//skip '=' ///
				std::string val = subStr.substr(kpos + 1, spos - kpos - 1);
				params[key] = val;
				//skip '&' ///
				subStr = subStr.substr(spos + 1, std::string::npos);
			}
			else {
				break;
			}
		}
	} while (0);
	std::string keyValues;
	for (auto param : params) {
		keyValues += "\n--- **** " + param.first + "=" + param.second;
	}
	//LOG_DEBUG << "--- *** " << keyValues;
	return true;
}

//网关服[S]端 <- HTTP客户端[C]端，WEB前端
void Gateway::processHttpRequest(
	const muduo::net::HttpRequest& req, muduo::net::HttpResponse& rsp,
	muduo::net::InetAddress const& peerAddr,
	muduo::Timestamp receiveTime) {
	//LOG_INFO << __FUNCTION__ << " --- *** " << getRequestStr(req);
	rsp.setStatusCode(muduo::net::HttpResponse::k200Ok);
	rsp.setStatusMessage("OK");
	//注意要指定connection状态
	rsp.setCloseConnection(true);
	rsp.addHeader("Server", "MUDUO");
	if (req.path() == "/") {
#if 0
		rsp.setContentType("text/html;charset=utf-8");
		std::string now = muduo::Timestamp::now().toFormattedString();
		rsp.setBody("<html><body>Now is " + now + "</body></html>");
#else
		//HTTP应答包(header/body)
		setFailedResponse(rsp,
			muduo::net::HttpResponse::k404NotFound,
			"HTTP/1.1 404 Not Found\r\n\r\n");
#endif
	}
	else if (req.path() == "/GameHandle") {
		LOG_ERROR << "--- *** " << req.methodString() << "\n" << req.query();
		rsp.setContentType("application/xml;charset=utf-8");
		rsp.setBody(getRequestStr(req));
	}
	//刷新客户端访问IP黑名单信息
	else if (req.path() == "/refreshBlackList") {
		//管理员挂维护/恢复服务
		std::map<in_addr_t, IpVisitE>::const_iterator it = adminList_.find(peerAddr.ipNetEndian());
		if (it != adminList_.end()) {
			rsp.setContentType("text/plain;charset=utf-8");
			refreshBlackList();
			rsp.setBody("success");
		}
		else {
			//HTTP应答包(header/body)
			setFailedResponse(rsp,
				muduo::net::HttpResponse::k404NotFound,
				"HTTP/1.1 504 权限不够\r\n\r\n");
		}
	}
	//刷新HTTP访问IP白名单信息
	else if (req.path() == "/refreshWhiteList") {
		//管理员挂维护/恢复服务
		std::map<in_addr_t, IpVisitE>::const_iterator it = adminList_.find(peerAddr.ipNetEndian());
		if (it != adminList_.end()) {
			rsp.setContentType("text/plain;charset=utf-8");
			refreshWhiteList();
			rsp.setBody("success");
		}
		else {
			//HTTP应答包(header/body)
			setFailedResponse(rsp,
				muduo::net::HttpResponse::k404NotFound,
				"HTTP/1.1 504 权限不够\r\n\r\n");
		}
	}
	//请求挂维护/恢复服务 status=0挂维护 status=1恢复服务
	else if (req.path() == "/repairServer") {
		//管理员挂维护/恢复服务
		std::map<in_addr_t, IpVisitE>::const_iterator it = adminList_.find(peerAddr.ipNetEndian());
		if (it != adminList_.end()) {
			rsp.setContentType("text/plain;charset=utf-8");
			std::string rspdata;
			repairServer(req.query(), rspdata);
			rsp.setContentType("application/json;charset=utf-8");
			rsp.setBody(rspdata);
		}
		else {
			//HTTP应答包(header/body)
			setFailedResponse(rsp,
				muduo::net::HttpResponse::k404NotFound,
				"HTTP/1.1 504 权限不够\r\n\r\n");
		}
	}
	else if (req.path() == "/help") {
		//管理员挂维护/恢复服务
		std::map<in_addr_t, IpVisitE>::const_iterator it = adminList_.find(peerAddr.ipNetEndian());
		if (it != adminList_.end()) {
			rsp.setContentType("text/html;charset=utf-8");
			rsp.setBody("<html>"
				"<head><title>help</title></head>"
				"<body>"
				"<h4>/refreshAgentInfo</h4>"
				"<h4>/refreshWhiteList</h4>"
				"<h4>/repairServer?type=HallServer&name=192.168.2.158:20001&status=0|1(status=0挂维护 status=1恢复服务)</h4>"
				"<h4>/repairServer?type=GameServer&name=4001:192.168.0.1:5847&status=0|1(status=0挂维护 status=1恢复服务)</h4>"
				"</body>"
				"</html>");
		}
		else {
			//HTTP应答包(header/body)
			setFailedResponse(rsp,
				muduo::net::HttpResponse::k404NotFound,
				"HTTP/1.1 504 权限不够\r\n\r\n");
		}
	}
	else {
#if 1
		//HTTP应答包(header/body)
		setFailedResponse(rsp,
			muduo::net::HttpResponse::k404NotFound,
			"HTTP/1.1 404 Not Found\r\n\r\n");
#else
		rsp.setBody("<html><head><title>httpServer</title></head>"
			"<body><h1>Not Found</h1>"
			"</body></html>");
		//rsp.setStatusCode(muduo::net::HttpResponse::k404NotFound);
#endif
	}
}

//网关服[S]端 <- HTTP客户端[C]端，WEB前端
void Gateway::refreshWhiteList() {
	if (whiteListControl_ == IpVisitCtrlE::kOpenAccept) {
		//Accept时候判断，socket底层控制，否则开启异步检查
		httpServer_.getLoop()->runInLoop(std::bind(&Gateway::refreshWhiteListInLoop, this));
	}
	else if (whiteListControl_ == IpVisitCtrlE::kOpen) {
		//同步刷新IP访问白名单
		refreshWhiteListSync();
	}
}

//网关服[S]端 <- HTTP客户端[C]端，WEB前端
bool Gateway::refreshWhiteListSync() {
	//Accept时候判断，socket底层控制，否则开启异步检查
	assert(whiteListControl_ == IpVisitCtrlE::kOpen);
	{
		WRITE_LOCK(whiteList_mutex_);
		whiteList_.clear();
	}
	for (std::map<in_addr_t, IpVisitE>::const_iterator it = whiteList_.begin();
		it != whiteList_.end(); ++it) {
		LOG_DEBUG << "--- *** " << "IP访问白名单\n"
			<< "--- *** ipaddr[" << Inet2Ipstr(it->first) << "] status[" << it->second << "]";
	}
	return false;
}

//网关服[S]端 <- HTTP客户端[C]端，WEB前端
bool Gateway::refreshWhiteListInLoop() {
	//Accept时候判断，socket底层控制，否则开启异步检查
	assert(whiteListControl_ == IpVisitCtrlE::kOpenAccept);
	httpServer_.getLoop()->assertInLoopThread();
	whiteList_.clear();
	for (std::map<in_addr_t, IpVisitE>::const_iterator it = whiteList_.begin();
		it != whiteList_.end(); ++it) {
		LOG_DEBUG << "--- *** " << "IP访问白名单\n"
			<< "--- *** ipaddr[" << Inet2Ipstr(it->first) << "] status[" << it->second << "]";
	}
 	return false;
}

static void replace(std::string& json, const std::string& placeholder, const std::string& value) {
	boost::replace_all<std::string>(json, "\"" + placeholder + "\"", value);
}

//构造返回结果
static std::string createResponse(
	int opType,//status=1
	std::string const& servname,//type=HallSever
	std::string const& name,//name=192.168.2.93:10000
	int errcode, std::string const& errmsg) {
	boost::property_tree::ptree root, data;
	root.put("op", ":op");
	root.put("type", servname);
	root.put("name", name);
	root.put("code", ":code");
	root.put("errmsg", errmsg);
	std::stringstream s;
	boost::property_tree::json_parser::write_json(s, root, false);
	std::string json = s.str();
	replace(json, ":op", std::to_string(opType));
	replace(json, ":code", std::to_string(errcode));
	boost::replace_all<std::string>(json, "\\", "");
	return json;
}

//请求挂维护/恢复服务 status=0挂维护 status=1恢复服务
bool Gateway::repairServer(servTyE servTy, std::string const& servname, std::string const& name, int status, std::string& rspdata) {
	LOG_ERROR << __FUNCTION__ << " --- *** " << "name[" << name << "] status[" << status << "]";
	static std::string path[kMaxServTy] = {
		"/GAME/HallServers/",
		"/GAME/GameServers/",
	};
	static std::string pathrepair[kMaxServTy] = {
		"/GAME/HallServersInvalid/",
		"/GAME/GameServersInvalid/",
	};
	do {
		//请求挂维护
		if (status == ServiceStateE::kRepairing) {
			/* 如果之前服务中, 尝试挂维护中, 并返回之前状态
			* 如果返回服务中, 说明刚好挂维护成功, 否则说明之前已被挂维护 */
			//if (ServiceStateE::kRunning == __sync_val_compare_and_swap(&serverState_, ServiceStateE::kRunning, ServiceStateE::kRepairing)) {
			//
			//在指定类型服务中，并且不在维护节点中
			//
			if (clients_[servTy].exist(name) && !clients_[servTy].isRepairing(name)) {
				//当前仅有一个提供服务的节点，禁止挂维护
				if (clients_[servTy].remaining() <= 1) {
					LOG_ERROR << __FUNCTION__ << " --- *** " << "当前仅有一个提供服务的节点，禁止挂维护!!!";
					rspdata = createResponse(status, servname, name, 2, "仅剩余一个服务节点，禁止挂维护");
					break;
				}
				//添加 repairnode
				std::string repairnode = pathrepair[servTy] + name;
				//if (ZNONODE == zkclient_->existsNode(repairnode)) {
					//创建维护节点
					//zkclient_->createNode(repairnode, name, false);
					//挂维护中状态
					clients_[servTy].repair(name);
					LOG_ERROR << __FUNCTION__ << " --- *** " << "创建维护节点 " << repairnode;
				//}
				//删除 servicenode
				std::string servicenode = path[servTy] + name;
				//if (ZNONODE != zkclient_->existsNode(servicenode)) {
					//删除服务节点
					//zkclient_->deleteNode(servicenode);
					LOG_ERROR << __FUNCTION__ << " --- *** " << "删除服务节点 " << servicenode;
				//}
				rspdata = createResponse(status, servname, name, 0, "success");
			}
			else {
				rspdata = createResponse(status, servname, name, 0, "节点不存在|已挂了维护");
			}
			return true;
		}
		//请求恢复服务
		else if (status == ServiceStateE::kRunning) {
			/* 如果之前挂维护中, 尝试恢复服务, 并返回之前状态
			* 如果返回挂维护中, 说明刚好恢复服务成功, 否则说明之前已在服务中 */
			//if (ServiceStateE::kRepairing == __sync_val_compare_and_swap(&serverState_, ServiceStateE::kRepairing, ServiceStateE::kRunning)) {
			//
			//在指定类型服务中，并且在维护节点中
			//
			if (clients_[servTy].exist(name) && clients_[servTy].isRepairing(name)) {
				//添加 servicenode
				std::string servicenode = path[servTy] + name;
				//if (ZNONODE == zkclient_->existsNode(servicenode)) {
					//创建服务节点
					//zkclient_->createNode(servicenode, name, false);
					//恢复服务状态
					clients_[servTy].recover(name);
					LOG_ERROR << __FUNCTION__ << " --- *** " << "创建服务节点 " << servicenode;
				//}
				//删除 repairnode
				std::string repairnode = pathrepair[servTy] + name;
				//if (ZNONODE != zkclient_->existsNode(repairnode)) {
					//删除维护节点
					//zkclient_->deleteNode(repairnode);
					LOG_ERROR << __FUNCTION__ << " --- *** " << "删除维护节点 " << repairnode;
				//}
				rspdata = createResponse(status, servname, name, 0, "success");
			}
			else {
				rspdata = createResponse(status, servname, name, 0, "节点不存在|已在服务中");
			}
			return true;
		}
		rspdata = createResponse(status, servname, name, 1, "参数无效，无任何操作");
	} while (0);
	return false;
}

//网关服[S]端 <- HTTP客户端[C]端，WEB前端
bool Gateway::repairServer(std::string const& queryStr, std::string& rspdata) {
	std::string errmsg;
	servTyE servTy;
	std::string name;
	int status;
	do {
		//解析参数
		HttpParams params;
		if (!parseQuery(queryStr, params, errmsg)) {
			break;
		}
		//type
		//type=HallServer name=192.168.2.158:20001
		//type=GameServer name=4001:192.168.0.1:5847
		HttpParams::const_iterator typeKey = params.find("type");
		if (typeKey == params.end() || typeKey->second.empty()) {
			rspdata = createResponse(status, typeKey->second, name, 1, "参数无效，无任何操作");
			break;
		}
		else {
			if (typeKey->second == "HallServer") {
				servTy = servTyE::kHallTy;
			}
			else if (typeKey->second == "GameServer") {
				servTy = servTyE::kGameTy;
			}
			else {
				rspdata = createResponse(status, typeKey->second, name, 1, "参数无效，无任何操作");
				break;
			}
		}
		//name
		HttpParams::const_iterator nameKey = params.find("name");
		if (nameKey == params.end() || nameKey->second.empty()) {
			rspdata = createResponse(status, typeKey->second, name, 1, "参数无效，无任何操作");
			break;
		}
		else {
			name = nameKey->second;
		}
		//status
		HttpParams::const_iterator statusKey = params.find("status");
		if (statusKey == params.end() || statusKey->second.empty() ||
			(status = atol(statusKey->second.c_str())) < 0) {
			rspdata = createResponse(status, typeKey->second, name, 1, "参数无效，无任何操作");
			break;
		}
		//repairServer
		return repairServer(servTy, typeKey->second, name, status, rspdata);
	} while (0);
	return false;
}

//网关服[S]端 <- HTTP客户端[C]端，WEB前端
void Gateway::repairServerNotify(std::string const& msg, std::string& rspdata) {
	std::string errmsg;
	servTyE servTy;
	std::string name;
	int status;
	std::stringstream ss(msg);
	boost::property_tree::ptree root;
	boost::property_tree::read_json(ss, root);
	try {
		do {
			//type
			std::string servname = root.get<std::string>("type");
			if (servname == "HallServer") {
				servTy = servTyE::kHallTy;
			}
			else if (servname == "GameServer") {
				servTy = servTyE::kGameTy;
			}
			else {
				rspdata = createResponse(status, servname, name, 1, "参数无效，无任何操作");
				break;
			}
			//name
			name = root.get<std::string>("name");
			if (name.empty()) {
				rspdata = createResponse(status, servname, name, 1, "参数无效，无任何操作");
				break;
			}
			//status
			status = root.get<int>("status");
			if (status < 0) {
				rspdata = createResponse(status, servname, name, 1, "参数无效，无任何操作");
				break;
			}
			//repairServer
			repairServer(servTy, servname, name, status, rspdata);
		} while (0);
	}
	catch (boost::property_tree::ptree_error & e) {
		LOG_ERROR << __FUNCTION__ << " " << e.what();
	}
}

//网关服[C]端 -> 大厅服[S]端
void Gateway::onHallConnection(const muduo::net::TcpConnectionPtr& conn) {

	conn->getLoop()->assertInLoopThread();

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

//网关服[C]端 -> 大厅服[S]端
void Gateway::onHallMessage(const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf,
	muduo::Timestamp receiveTime) {

	conn->getLoop()->assertInLoopThread();

	//LOG_ERROR << __FUNCTION__;
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
			assert(!session.empty() && session.size() == packet::kSessionSZ);
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
			//session -> conn -> entryContext -> index
			//////////////////////////////////////////////////////////////////////////
			muduo::net::WeakTcpConnectionPtr weakConn = entities_.get(session);
			muduo::net::TcpConnectionPtr peer(weakConn.lock());
			if (peer) {
				ContextPtr entryContext(boost::any_cast<ContextPtr>(peer->getContext()));
				assert(entryContext);

				int index = entryContext->getWorkerIndex();
				assert(index >= 0 && index < threadPool_.size());

				//扔给任务消息队列处理
				threadPool_[index]->run(
					std::bind(
						&Gateway::asyncHallHandler,
						this,
						conn,
						weakConn, buffer, receiveTime));
			}
#endif
		}
	}
}

//网关服[C]端 -> 大厅服[S]端
void Gateway::asyncHallHandler(
	const muduo::net::TcpConnectionPtr& conn,
	muduo::net::WeakTcpConnectionPtr const& weakConn,
	BufferPtr& buf,
	muduo::Timestamp receiveTime) {
	//内部消息头internal_prev_header_t + 命令消息头header_t
	if (buf->readableBytes() < packet::kPrevHeaderLen + packet::kHeaderLen) {
		return;
	}
	muduo::net::TcpConnectionPtr peer(weakConn.lock());
	if (peer) {
		ContextPtr entryContext(boost::any_cast<ContextPtr>(peer->getContext()));
		assert(entryContext);

		//内部消息头internal_prev_header_t
		packet::internal_prev_header_t /*const*/* pre_header = (packet::internal_prev_header_t /*const*/*)buf->peek();
		//session
		std::string session((char const*)pre_header->session, sizeof(pre_header->session));
		assert(!session.empty() && session.size() == packet::kSessionSZ);
		//userid
		int64_t userid = pre_header->userID;
		//命令消息头header_t
		packet::header_t /*const*/* header = (packet::header_t /*const*/*)(buf->peek() + packet::kPrevHeaderLen);
			
		//校验CRC header->len = packet::kHeaderLen + len
		uint16_t crc = packet::getCheckSum((uint8_t const*)&header->ver, header->len - 4);
		assert(header->crc == crc);
			
		TraceMessageID(header->mainID, header->subID);
			
		if (
			header->mainID == ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL &&
			header->subID == ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_RES &&
			pre_header->ok == 1) {
			//////////////////////////////////////////////////////////////////////////
			//登陆成功，指定userid
			//////////////////////////////////////////////////////////////////////////
			entryContext->setUserID(userid);
			//////////////////////////////////////////////////////////////////////////
			//登陆成功，指定玩家大厅
			//////////////////////////////////////////////////////////////////////////
			assert(conn);
			std::vector<std::string> vec;
			boost::algorithm::split(vec, conn->name(), boost::is_any_of(":"));
			std::string const& name = vec[0] + ":" + vec[1];
			ClientConn clientConn(name, conn);
			LOG_WARN << "登陆成功，大厅节点 >>> " << name;
			entryContext->setClientConn(servTyE::kHallTy, clientConn);
			//////////////////////////////////////////////////////////////////////////
			//顶号处理 userid -> conn
			//////////////////////////////////////////////////////////////////////////
			muduo::net::TcpConnectionPtr peer_(sessions_.add(userid, muduo::net::WeakTcpConnectionPtr(peer)).lock());
			if (peer_) {
				assert(peer_ != peer);
				ContextPtr entryContext_(boost::any_cast<ContextPtr>(peer_->getContext()));
				assert(entryContext_);
				std::string const& session_ = entryContext_->getSession();
				assert(session_.size() == packet::kSessionSZ);
				assert(session_ != session);
				BufferPtr buffer = packClientShutdownMsg(userid, 0); assert(buffer);
				muduo::net::websocket::send(peer_, buffer->peek(), buffer->readableBytes());
#if 0
				peer_->getLoop()->runAfter(0.2f, [&]() {
					entry_.reset();
					});
#else
				peer_->forceCloseWithDelay(0.2);
#endif
			}
		}
		else if (
			header->mainID == ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL &&
			header->subID == ::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_GAME_SERVER_MESSAGE_RES &&
			pre_header->ok == 1) {
			//校验userid
			assert(userid == entryContext->getUserID());
			std::string gameIp;
			if (REDISCLIENT.GetUserOnlineInfoIP(userid, gameIp)) {
				//分配用户游戏服
				ClientConn client;
				//异步获取指定游戏服连接
				clients_[servTyE::kGameTy].clients_->get(gameIp, client);
				muduo::net::TcpConnectionPtr gameConn(client.second.lock());
				if (gameConn) {
					entryContext->setClientConn(servTyE::kGameTy, client);
				}
				else {

				}
			}
		}
		muduo::net::websocket::send(peer, (uint8_t const*)buf->peek() + packet::kPrevHeaderLen, header->len);
		return;
	}
	LOG_ERROR << __FUNCTION__ << " --- *** " << "peer(weakConn.lock()) failed";
}

#if 0
//网关服[C]端 -> 大厅服[S]端
void Gateway::sendHallMessage(
	ContextPtr const& entryContext,
	BufferPtr& buf, int64_t userid) {
	//printf("%s %s(%d)\n", __FUNCTION__, __FILE__, __LINE__);
	if (entryContext) {
		//printf("%s %s(%d)\n", __FUNCTION__, __FILE__, __LINE__);
		ClientConn const& clientConn = entryContext->getClientConn(servTyE::kHallTy);
		muduo::net::TcpConnectionPtr hallConn(clientConn.second.lock());
		if (hallConn) {
			assert(hallConn->connected());
#if 0
			assert(
				std::find(
					std::begin(clients_[servTyE::kHallTy].ips_),
					std::end(clients_[servTyE::kHallTy].ips_),
					clientConn.first) != clients_[servTyE::kHallTy].ips_.end());
#endif
			clients_[servTyE::kHallTy].clients_->check(clientConn.first, true);
			if (buf) {
				//printf("len = %d\n", buf->readableBytes());
				hallConn->send(buf.get());
			}
		}
		else {
			LOG_ERROR << __FUNCTION__ << " --- *** " << "用户大厅服无效，重新分配";
			//用户大厅服无效，重新分配
			ClientConnList clients;
			//异步获取全部有效大厅连接
			clients_[servTyE::kHallTy].clients_->getAll(clients);
			if (clients.size() > 0) {
				int index = randomHall_.betweenInt(0, clients.size() - 1).randInt_mt();
				assert(index >= 0 && index < clients.size());
				ClientConn const& clientConn = clients[index];
				muduo::net::TcpConnectionPtr hallConn(clientConn.second.lock());
				if (hallConn) {
					if (entryContext->getUserID() > 0) {
						//账号已经登陆，但登陆大厅失效了，重新指定账号登陆大厅
						entryContext->setClientConn(servTyE::kHallTy, clientConn);
					}
					if (buf) {
						//printf("len = %d\n", buf->readableBytes());
						hallConn->send(buf.get());
					}
				}
				else {

				}
			}
		}
	}
}
#else
//网关服[C]端 -> 大厅服[S]端
void Gateway::sendHallMessage(
	ContextPtr const& entryContext,
	BufferPtr& buf, int64_t userid) {
	//printf("%s %s(%d)\n", __FUNCTION__, __FILE__, __LINE__);
	if (entryContext) {
		//printf("%s %s(%d)\n", __FUNCTION__, __FILE__, __LINE__);
		ClientConn const& clientConn = entryContext->getClientConn(servTyE::kHallTy);
		muduo::net::TcpConnectionPtr hallConn(clientConn.second.lock());
		if (hallConn) {
			assert(hallConn->connected());
			assert(entryContext->getUserID() > 0);
			//判断节点是否维护中
			if (!clients_[servTyE::kHallTy].isRepairing(clientConn.first)) {
#if 0
				assert(
					std::find(
						std::begin(clients_[servTyE::kHallTy].clients_),
						std::end(clients_[servTyE::kHallTy].clients_),
						clientConn.first) != clients_[servTyE::kHallTy].clients_.end());
#endif
				clients_[servTyE::kHallTy].clients_->check(clientConn.first, true);
				if (buf) {
					//printf("len = %d\n", buf->readableBytes());
					hallConn->send(buf.get());
				}
			}
			else {
				LOG_ERROR << __FUNCTION__ << " --- *** " << "用户大厅服维护，重新分配";
				//用户大厅服维护，重新分配
				ClientConnList clients;
				//异步获取全部有效大厅连接
				clients_[servTyE::kHallTy].clients_->getAll(clients);
				if (clients.size() > 0) {
					bool bok = false;
					std::map<std::string, bool> repairs;
					do {
						int index = randomHall_.betweenInt(0, clients.size() - 1).randInt_mt();
						assert(index >= 0 && index < clients.size());
						ClientConn const& clientConn = clients[index];
						muduo::net::TcpConnectionPtr hallConn(clientConn.second.lock());
						if (hallConn) {
							//判断节点是否维护中
							if (bok = !clients_[servTyE::kHallTy].isRepairing(clientConn.first)) {
								//账号已经登陆，但登陆大厅维护中，重新指定账号登陆大厅
								entryContext->setClientConn(servTyE::kHallTy, clientConn);
								if (buf) {
									//printf("len = %d\n", buf->readableBytes());
									hallConn->send(buf.get());
								}
							}
							else {
								repairs[clientConn.first] = true;
							}
						}
					} while (!bok && repairs.size() != clients.size());
				}
			}
		}
		else {
			LOG_ERROR << __FUNCTION__ << " --- *** " << "用户大厅服失效，重新分配";
			//用户大厅服失效，重新分配
			ClientConnList clients;
			//异步获取全部有效大厅连接
			clients_[servTyE::kHallTy].clients_->getAll(clients);
			if (clients.size() > 0) {
				bool bok = false;
				std::map<std::string, bool> repairs;
				do {
					int index = randomHall_.betweenInt(0, clients.size() - 1).randInt_mt();
					assert(index >= 0 && index < clients.size());
					ClientConn const& clientConn = clients[index];
					muduo::net::TcpConnectionPtr hallConn(clientConn.second.lock());
					if (hallConn) {
						assert(hallConn->connected());
						//判断节点是否维护中
						if (bok = !clients_[servTyE::kHallTy].isRepairing(clientConn.first)) {
							if (entryContext->getUserID() > 0) {
								//账号已经登陆，但登陆大厅失效了，重新指定账号登陆大厅
								entryContext->setClientConn(servTyE::kHallTy, clientConn);
							}
							if (buf) {
								//printf("len = %d\n", buf->readableBytes());
								hallConn->send(buf.get());
							}
						}
						else {
							repairs[clientConn.first] = true;
						}
					}
				} while (!bok && repairs.size() != clients.size());
			}
		}
	}
}
#endif

//网关服[C]端 -> 大厅服[S]端，跨网关顶号处理(异地登陆)
void Gateway::onUserReLoginNotify(std::string const& msg) {
	LOG_WARN << __FUNCTION__ << " " << msg;
	std::stringstream ss(msg);
	boost::property_tree::ptree root;
	boost::property_tree::read_json(ss, root);
	try {
		int64_t userid = root.get<int>("userId");
		//////////////////////////////////////////////////////////////////////////
		//账号最新登陆session
		//////////////////////////////////////////////////////////////////////////
		std::string const session = root.get<std::string>("session");
#if 0
		std::string const servid_ = root.get<std::string>("proxyip");
		//排除自己
		std::string const& servid = nodeValue_;
		if (servid == servid_) {
			return;
		}
#endif
		muduo::net::TcpConnectionPtr peer(entities_.get(session).lock());
		if (!peer) {
			//////////////////////////////////////////////////////////////////////////
			//顶号处理 userid -> conn -> session
			//////////////////////////////////////////////////////////////////////////
			muduo::net::TcpConnectionPtr peer_(sessions_.get(userid).lock());
			if (peer_) {
				ContextPtr entryContext_(boost::any_cast<ContextPtr>(peer_->getContext()));
				assert(entryContext_);
				assert(entryContext_->getUserID() == userid);
				//相同账号，不同session，不关心旧的session是多少，只要不是当前最新，关闭之
				if (entryContext_->getSession() != session) {
					BufferPtr buffer = packClientShutdownMsg(userid, 0); assert(buffer);
					muduo::net::websocket::send(peer_, buffer->peek(), buffer->readableBytes());
#if 0
					peer_->getLoop()->runAfter(0.2f, [&]() {
						entry_.reset();
						});
#else
					peer_->forceCloseWithDelay(0.2);
#endif
				}
			}
		}
		else {
#if 0
			{
				ContextPtr entryContext(boost::any_cast<ContextPtr>(peer->getContext()));
				assert(entryContext);
				assert(entryContext->getUserID() == userid);
				assert(entryContext->getSession() == session);
			}
			{
				muduo::net::TcpConnectionPtr peer(sessions_.get(userid).lock());
				assert(peer);
				ContextPtr entryContext(boost::any_cast<ContextPtr>(peer->getContext()));
				assert(entryContext);
				assert(entryContext->getUserID() == userid);
				assert(entryContext->getSession() == session);
			}
#endif
		}
	}
	catch (boost::property_tree::ptree_error & e) {
		LOG_ERROR << __FUNCTION__ << " " << e.what();
	}
}

//网关服[C]端 -> 大厅服[S]端
void Gateway::onUserOfflineHall(ContextPtr const& entryContext) {
	MY_TRY()
	if (entryContext) {
		//userid
		int64_t userid = entryContext->getUserID();
		//clientip
		uint32_t clientip = entryContext->getFromIp();
		//session
		std::string const& session = entryContext->getSession();
		//aeskey
		std::string const& aeskey = entryContext->getAesKey();
		if (userid > 0 && !session.empty()) {
			//packMessage
			BufferPtr buffer = packet::packMessage(
				userid,
				session,
				aeskey,
				clientip,
				0,
				::Game::Common::MAIN_MESSAGE_PROXY_TO_HALL,
				::Game::Common::MESSAGE_PROXY_TO_HALL_SUBID::HALL_ON_USER_OFFLINE,
				NULL);
			if (buffer) {
				TraceMessageID(
					::Game::Common::MAIN_MESSAGE_PROXY_TO_HALL,
					::Game::Common::MESSAGE_PROXY_TO_HALL_SUBID::HALL_ON_USER_OFFLINE);
				assert(buffer->readableBytes() < packet::kMaxPacketSZ);
				sendHallMessage(entryContext, buffer, userid);
			}
		}
	}
	MY_CATCH()
}

//网关服[C]端 -> 游戏服[S]端
void Gateway::onGameConnection(const muduo::net::TcpConnectionPtr& conn) {

	conn->getLoop()->assertInLoopThread();

	if (conn->connected()) {
		int32_t num = numConnected_.incrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "网关服[" << conn->localAddress().toIpPort() << "] -> 游戏服["
			<< conn->peerAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;
	}
	else {
		int32_t num = numConnected_.decrementAndGet();
		LOG_INFO << __FUNCTION__ << " --- *** " << "网关服[" << conn->localAddress().toIpPort() << "] -> 游戏服["
			<< conn->peerAddress().toIpPort() << "] "
			<< (conn->connected() ? "UP" : "DOWN") << " " << num;
	}
}

//网关服[C]端 -> 游戏服[S]端
void Gateway::onGameMessage(const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* buf,
	muduo::Timestamp receiveTime) {

	conn->getLoop()->assertInLoopThread();

	//LOG_ERROR << __FUNCTION__;
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
			assert(!session.empty() && session.size() == packet::kSessionSZ);
#if 0
			//////////////////////////////////////////////////////////////////////////
			//session -> hash(session) -> index
			//////////////////////////////////////////////////////////////////////////
			int index = hash_session_(session) % threadPool_.size();

			//扔给任务消息队列处理
			threadPool_[index]->run(
				std::bind(
					&Gateway::asyncGameHandler,
					this,
					conn, buffer, receiveTime));
#else
			//////////////////////////////////////////////////////////////////////////
			//session -> entry -> conn -> entryContext -> index
			//////////////////////////////////////////////////////////////////////////
			muduo::net::WeakTcpConnectionPtr weakConn = entities_.get(session);
			muduo::net::TcpConnectionPtr peer(weakConn.lock());
			if (peer) {
				ContextPtr entryContext(boost::any_cast<ContextPtr>(peer->getContext()));
				assert(entryContext);

				int index = entryContext->getWorkerIndex();
				assert(index >= 0 && index < threadPool_.size());

				//扔给任务消息队列处理
				threadPool_[index]->run(
					std::bind(
						&Gateway::asyncGameHandler,
						this,
						weakConn, buffer, receiveTime));
			}
#endif
		}
	}
}

//网关服[C]端 -> 游戏服[S]端
void Gateway::asyncGameHandler(
	muduo::net::WeakTcpConnectionPtr const& weakConn,
	BufferPtr& buf,
	muduo::Timestamp receiveTime) {
	//LOG_ERROR << __FUNCTION__;
	//内部消息头internal_prev_header_t + 命令消息头header_t
	if (buf->readableBytes() < packet::kPrevHeaderLen + packet::kHeaderLen) {
		return;
	}
	muduo::net::TcpConnectionPtr peer(weakConn.lock());
	if (peer) {
		ContextPtr entryContext(boost::any_cast<ContextPtr>(peer->getContext()));
		assert(entryContext);
		//内部消息头internal_prev_header_t
		packet::internal_prev_header_t /*const*/* pre_header = (packet::internal_prev_header_t /*const*/*)buf->peek();
		//session
		std::string session((char const*)pre_header->session, sizeof(pre_header->session));
		assert(!session.empty() && session.size() == packet::kSessionSZ);
		//userid
		int64_t userid = pre_header->userID;
		//校验userid
		assert(userid == entryContext->getUserID());
		//校验session
		if (session != entryContext->getSession()) {
			LOG_ERROR << __FUNCTION__ << " --- *** " << "check userid. " << userid
				<< "\nsession = " << buffer2HexStr((unsigned char const*)session.c_str(), session.length())
				<< "\nsave    = " << buffer2HexStr((unsigned char const*)entryContext->getSession().c_str(), entryContext->getSession().length());
			return;
		}
		//命令消息头header_t
		packet::header_t /*const*/* header = (packet::header_t /*const*/*)(buf->peek() + packet::kPrevHeaderLen);
			
		//校验CRC header->len = packet::kHeaderLen + len
		uint16_t crc = packet::getCheckSum((uint8_t const*)&header->ver, header->len - 4);
		assert(header->crc == crc);
			
		TraceMessageID(header->mainID, header->subID);
			
		muduo::net::websocket::send(peer, (uint8_t const*)buf->peek() + packet::kPrevHeaderLen, header->len);
		return;
	}
	LOG_ERROR << __FUNCTION__ << " --- *** " << "peer(entry->getWeakConnPtr().lock()) failed";
}

//网关服[C]端 -> 游戏服[S]端
void Gateway::sendGameMessage(
	ContextPtr const& entryContext,
	BufferPtr& buf, int64_t userid) {
	//printf("%s %s(%d)\n", __FUNCTION__, __FILE__, __LINE__);
	if (entryContext) {
		//printf("%s %s(%d)\n", __FUNCTION__, __FILE__, __LINE__);
		ClientConn const& clientConn = entryContext->getClientConn(servTyE::kGameTy);
		muduo::net::TcpConnectionPtr gameConn(clientConn.second.lock());
		if (gameConn) {
			assert(gameConn->connected());
#if 0
			assert(
				std::find(
					std::begin(clients_[servTyE::kGameTy].ips_),
					std::end(clients_[servTyE::kGameTy].ips_),
					clientConn.first) != clients_[servTyE::kGameTy].ips_.end());
#endif
			clients_[servTyE::kGameTy].clients_->check(clientConn.first, true);
			if (buf) {
				//printf("len = %d\n", buf->readableBytes());
				size_t len = buf->readableBytes();
				char data[len];
				memset(data, 0, len);
				memcpy(data, buf->peek(), len);
				gameConn->send(data, len);
			}
		}
	}
}

//网关服[C]端 -> 游戏服[S]端
void Gateway::onUserOfflineGame(
	ContextPtr const& entryContext, bool leave) {
	MY_TRY()
	if (entryContext) {
		//userid
		int64_t userid = entryContext->getUserID();
		//clientip
		uint32_t clientip = entryContext->getFromIp();
		//session
		std::string const& session = entryContext->getSession();
		//aeskey
		std::string const& aeskey = entryContext->getAesKey();
		if (userid > 0 && !session.empty()) {
			//packMessage
			BufferPtr buffer = packet::packMessage(
				userid,
				session,
				aeskey,
				clientip,
				KICK_LEAVEGS,
				::Game::Common::MAIN_MESSAGE_PROXY_TO_GAME_SERVER,
				::Game::Common::MESSAGE_PROXY_TO_GAME_SERVER_SUBID::GAME_SERVER_ON_USER_OFFLINE,
				NULL);
			if (buffer) {
				TraceMessageID(
					::Game::Common::MAIN_MESSAGE_PROXY_TO_GAME_SERVER,
					::Game::Common::MESSAGE_PROXY_TO_GAME_SERVER_SUBID::GAME_SERVER_ON_USER_OFFLINE);
				assert(buffer->readableBytes() < packet::kMaxPacketSZ);
				sendGameMessage(entryContext, buffer, userid);
			}
		}
	}
	MY_CATCH()
}

//网关服[S]端 <- 客户端[C]端，websocket
bool Gateway::onCondition(const muduo::net::InetAddress& peerAddr) {
	return true;
}

//网关服[S]端 <- 客户端[C]端，websocket
void Gateway::onConnection(const muduo::net::TcpConnectionPtr& conn) {
	
	conn->getLoop()->assertInLoopThread();
	
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
		//////////////////////////////////////////////////////////////////////////
		//websocket::Context::ctor
		//////////////////////////////////////////////////////////////////////////
		muduo::net::websocket::hook(
			std::bind(&Gateway::onConnected, this,
				std::placeholders::_1, std::placeholders::_2),
			std::bind(&Gateway::onMessage, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4),
			conn);

		EventLoopContextPtr context = boost::any_cast<EventLoopContextPtr>(conn->getLoop()->getContext());
		assert(context);
		
		EntryPtr entry(new Entry(muduo::net::WeakTcpConnectionPtr(conn)));
		
		//指定conn上下文信息
		ContextPtr entryContext(new Context(WeakEntryPtr(entry)));
		conn->setContext(entryContext);
		{
			//给新conn绑定一个worker线程，之后所有conn相关逻辑业务都在该worker线程中处理
			int index = context->allocWorkerIndex();
			assert(index >= 0 && index < threadPool_.size());
			entryContext->setWorkerIndex(index);
		}
		{
			//获取EventLoop关联的Bucket
			int index = context->getBucketIndex();
			assert(index >= 0 && index < bucketsPool_.size());
			//连接成功，压入桶元素
			conn->getLoop()->runInLoop(
				std::bind(&ConnBucket::pushBucket, bucketsPool_[index].get(), entry));
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
		assert(!conn->getContext().empty());
		//////////////////////////////////////////////////////////////////////////
		//websocket::Context::dtor
		//////////////////////////////////////////////////////////////////////////
		muduo::net::websocket::reset(conn);
		ContextPtr entryContext(boost::any_cast<ContextPtr>(conn->getContext()));
		assert(entryContext);
#if !defined(MAP_USERID_SESSION) && 0
		//userid
		int64_t userid = entryContext->getUserID();
		if (userid > 0) {		
			//check before remove
			sessions_.remove(userid, conn);
		}
#endif
#if 0
		//理论上session不过期，worker线程不变，实际上每次断线重连，session都会变更
		std::string const& session = entryContext->getSession();
		int index = hash_session_(session) % threadPool_.size();
#else
		//获取绑定的worker线程
		int index = entryContext->getWorkerIndex();
		assert(index >= 0 && index < threadPool_.size());
#endif
		//扔给任务消息队列处理，不要在网络I/O线程中处理业务，容易引发异常
		threadPool_[index]->run(
			std::bind(
				&Gateway::asyncOfflineHandler,
				this, entryContext));
	}
}

//网关服[S]端 <- 客户端[C]端，websocket
void Gateway::onConnected(
	const muduo::net::TcpConnectionPtr& conn,
	std::string const& ipaddr) {

	conn->getLoop()->assertInLoopThread();

	LOG_INFO << __FUNCTION__ << " --- *** " << "客户端真实IP[" << ipaddr << "]";

	assert(!conn->getContext().empty());
	ContextPtr entryContext(boost::any_cast<ContextPtr>(conn->getContext()));
	assert(entryContext);
	{
		//保存conn真实ipaddr到Entry::Context上下文
		muduo::net::InetAddress address(ipaddr, 0);
		entryContext->setFromIp(address.ipNetEndian());
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
		//////////////////////////////////////////////////////////////////////////
		//map[session] = weakConn
		//////////////////////////////////////////////////////////////////////////
		ContextPtr entryContext(boost::any_cast<ContextPtr>(conn->getContext()));
		assert(entryContext);
		entities_.add(session, muduo::net::WeakTcpConnectionPtr(conn));
		LOG_WARN << __FUNCTION__ << " session[ " << session << " ]";
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

	conn->getLoop()->assertInLoopThread();

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
		ContextPtr entryContext(boost::any_cast<ContextPtr>(conn->getContext()));
		assert(entryContext);
		EntryPtr entry(entryContext->getWeakEntryPtr().lock());
		if (likely(entry)) {
			{
				EventLoopContextPtr context = boost::any_cast<EventLoopContextPtr>(conn->getLoop()->getContext());
				assert(context);
				
				int index = context->getBucketIndex();
				assert(index >= 0 && index < bucketsPool_.size());
				
				//收到消息包，更新桶元素
				conn->getLoop()->runInLoop(
					std::bind(&ConnBucket::updateBucket, bucketsPool_[index].get(), entry));
			}
			{				
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
					std::bind(
						&Gateway::asyncClientHandler,
						this, muduo::net::WeakTcpConnectionPtr(conn), buffer, receiveTime));
			}
			return;
		}
		//累计未处理请求数
		numTotalBadReq_.incrementAndGet();
	}
}

//网关服[S]端 <- 客户端[C]端，websocket 异步回调
void Gateway::asyncClientHandler(
	muduo::net::WeakTcpConnectionPtr const& weakConn,
	BufferPtr& buf,
	muduo::Timestamp receiveTime) {
	if (buf->readableBytes() < packet::kHeaderLen) {
		//累计未处理请求数
		numTotalBadReq_.incrementAndGet();
		return;
	}
	//data, dataLen
	uint8_t const* data = (uint8_t const*)buf->peek();
	size_t len = buf->readableBytes();
	//packet::header_t
	packet::header_t* header = (packet::header_t*)(&data[0]);
	//校验CRC header->len = packet::kHeaderLen + len
	//header.len uint16_t
	//header.crc uint16_t
	//header.ver ~ header.realsize + protobuf
	uint16_t crc = packet::getCheckSum(&data[4], /*header->len*/len - 4);
	if (header->len == len &&
		header->crc == crc &&
		header->ver == 1 &&
		header->sign == HEADER_SIGN) {
		//锁定conn操作
		//刚开始还在想，会不会出现超时conn被异步关闭释放掉，而业务逻辑又被处理了，却发送不了的尴尬情况，
		//假如因为超时entry弹出bucket，引用计数减1，处理业务之前这里使用shared_ptr，持有entry引用计数(加1)，
		//如果持有失败，说明弹出bucket计数减为0，entry被析构释放，conn被关闭掉了，也就不会执行业务逻辑处理，
		//如果持有成功，即使超时entry弹出bucket，引用计数减1，但并没有减为0，entry也就不会被析构释放，conn也不会被关闭，
		//直到业务逻辑处理完并发送，entry引用计数减1变为0，析构被调用关闭conn(如果conn还存在的话，业务处理完也会主动关闭conn)
		muduo::net::TcpConnectionPtr peer(weakConn.lock());
		if (peer) {
 			//mainID
 			switch (header->mainID) {
 			case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PROXY: {
 				//网关服(Gateway)
 				switch (header->enctype) {
				case packet::PUBENC_PROTOBUF_NONE: {
					//NONE
					TraceMessageID(header->mainID, header->subID);
					int cmd = packet::enword(header->mainID, header->subID);
					CmdCallbacks::const_iterator it = handlers_.find(cmd);
					if (it != handlers_.end()) {
						CmdCallback const& handler = it->second;
						handler(peer, get_pointer(buf));
					}
					break;
				}
				case packet::PUBENC_PROTOBUF_RSA: {
					//RSA
					TraceMessageID(header->mainID, header->subID);
					break;
				}
 				case packet::PUBENC_PROTOBUF_AES: {
 					//AES
					TraceMessageID(header->mainID, header->subID);
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
				TraceMessageID(header->mainID, header->subID);
				{
					ContextPtr entryContext(boost::any_cast<ContextPtr>(peer->getContext()));
					assert(entryContext);
					//userid
					int64_t userid = entryContext->getUserID();
					//clientip
					uint32_t clientip = entryContext->getFromIp();
					//session
					std::string const& session = entryContext->getSession();
					//aeskey
					std::string const& aeskey = entryContext->getAesKey();
					ClientConn const& clientConn = entryContext->getClientConn(servTyE::kHallTy);
					muduo::net::TcpConnectionPtr hallConn(clientConn.second.lock());
					assert(header->len == len);
					assert(header->len >= packet::kHeaderLen);
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
					std::string const& servid = nodeValue_;
#endif
					BufferPtr buffer = packet::packMessage(
						userid,
						session,
						aeskey,
						clientip,
						0,
#if 0
						servid,
#endif
						buf->peek(),
						header->len);
					if (buffer) {
						//发送大厅消息
						sendHallMessage(entryContext, buffer, userid);
					}
				}
				break;
			}
			case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER:
			case Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC: {
				//游戏服(GameS)
				//逻辑服(LogicS，逻辑子游戏libGame_xxx.so)
				TraceMessageID(header->mainID, header->subID);
				{
					ContextPtr entryContext(boost::any_cast<ContextPtr>(peer->getContext()));
					assert(entryContext);
					//userid
					int64_t userid = entryContext->getUserID();
					//clientip
					uint32_t clientip = entryContext->getFromIp();
					//session
					std::string const& session = entryContext->getSession();
					//aeskey
					std::string const& aeskey = entryContext->getAesKey();
					ClientConn const& clientConn = entryContext->getClientConn(servTyE::kHallTy);
					muduo::net::TcpConnectionPtr hallConn(clientConn.second.lock());
					assert(header->len == len);
					assert(header->len >= packet::kHeaderLen);
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
					std::string const& servid = nodeValue_;
#endif
					BufferPtr buffer = packet::packMessage(
						userid,
						session,
						aeskey,
						clientip,
						0,
#if 0
						servid,
#endif
						buf->peek(),
						header->len);
					if (buffer) {
						//发送游戏消息
						sendGameMessage(entryContext, buffer, userid);
					}
				}
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
	//累计未处理请求数
	numTotalBadReq_.incrementAndGet();
}

void Gateway::asyncOfflineHandler(ContextPtr const& entryContext) {
	if (entryContext) {
		LOG_ERROR << __FUNCTION__;
		//session
		std::string const& session = entryContext->getSession();
		if (!session.empty()) {
			//remove
			entities_.remove(session);
		}
		//userid
		int64_t userid = entryContext->getUserID();
		if (userid > 0) {
			//check before remove
			sessions_.remove(userid, session);
		}
		//offline hall
		onUserOfflineHall(entryContext);
		//offline game
		onUserOfflineGame(entryContext);
	}
}

//网关服[S]端 <- 客户端[C]端，websocket
BufferPtr Gateway::packClientShutdownMsg(int64_t userid, int status) {

	::Game::Common::ProxyNotifyShutDownUserClientMessage msg;
	msg.mutable_header()->set_sign(PROTO_BUF_SIGN);
	msg.set_userid(userid);
	msg.set_status(status);
	
	BufferPtr buffer = packet::packMessage(
		::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::Game::Common::PROXY_NOTIFY_SHUTDOWN_USER_CLIENT_MESSAGE_NOTIFY, &msg);
	
	TraceMessageID(::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::Game::Common::PROXY_NOTIFY_SHUTDOWN_USER_CLIENT_MESSAGE_NOTIFY);
	
	return buffer;
}

BufferPtr Gateway::packNoticeMsg(
	int32_t agentid, std::string const& title,
	std::string const& content, int msgtype) {
	
	::ProxyServer::Message::NotifyNoticeMessageFromProxyServerMessage msg;
	msg.mutable_header()->set_sign(PROTO_BUF_SIGN);
	msg.add_agentid(agentid);
	msg.set_title(title.c_str());
	msg.set_message(content);
	msg.set_msgtype(msgtype);
	
	BufferPtr buffer = packet::packMessage(
		::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::Game::Common::PROXY_NOTIFY_PUBLIC_NOTICE_MESSAGE_NOTIFY, &msg);
	
	TraceMessageID(::Game::Common::MAIN_MESSAGE_CLIENT_TO_PROXY,
		::Game::Common::PROXY_NOTIFY_PUBLIC_NOTICE_MESSAGE_NOTIFY);

	return buffer;
}

void Gateway::broadcastNoticeMsg(
	std::string const& title,
	std::string const& msg,
	int32_t agentid, int msgType) {
	//packNoticeMsg
	BufferPtr buffer = packNoticeMsg(
		agentid,
		title,
		msg,
		msgType);
	if (buffer) {
		sessions_.broadcast(buffer);
	}
}

//网关服[S]端 <- 客户端[C]端，websocket
void Gateway::broadcastMessage(int mainID, int subID, ::google::protobuf::Message* msg) {
	BufferPtr buffer = packet::packMessage(mainID, subID, msg);
	if (buffer) {
		TraceMessageID(mainID, subID);
		entities_.broadcast(buffer);
	}
}

//网关服[S]端 <- 客户端[C]端，websocket
void Gateway::refreshBlackList() {
	if (blackListControl_ == IpVisitCtrlE::kOpenAccept) {
		//Accept时候判断，socket底层控制，否则开启异步检查
		server_.getLoop()->runInLoop(std::bind(&Gateway::refreshBlackListInLoop, this));
	}
	else if (blackListControl_ == IpVisitCtrlE::kOpen) {
		//同步刷新IP访问黑名单
		refreshBlackListSync();
	}
}

//网关服[S]端 <- 客户端[C]端，websocket
bool Gateway::refreshBlackListSync() {
	//Accept时候判断，socket底层控制，否则开启异步检查
	assert(blackListControl_ == IpVisitCtrlE::kOpen);
	{
		WRITE_LOCK(blackList_mutex_);
		blackList_.clear();
	}
	for (std::map<in_addr_t, IpVisitE>::const_iterator it = blackList_.begin();
		it != blackList_.end(); ++it) {
		LOG_DEBUG << "--- *** " << "IP访问黑名单\n"
			<< "--- *** ipaddr[" << Inet2Ipstr(it->first) << "] status[" << it->second << "]";
	}
	return false;
}

//网关服[S]端 <- 客户端[C]端，websocket
bool Gateway::refreshBlackListInLoop() {
	//Accept时候判断，socket底层控制，否则开启异步检查
	assert(blackListControl_ == IpVisitCtrlE::kOpenAccept);
	server_.getLoop()->assertInLoopThread();
	blackList_.clear();
	for (std::map<in_addr_t, IpVisitE>::const_iterator it = blackList_.begin();
		it != blackList_.end(); ++it) {
		LOG_DEBUG << "--- *** " << "IP访问黑名单\n"
			<< "--- *** ipaddr[" << Inet2Ipstr(it->first) << "] status[" << it->second << "]";
	}
	return false;
}

void Gateway::onGetAesKey(
	const muduo::net::TcpConnectionPtr& conn,
	muduo::net::Buffer* msg) {

}