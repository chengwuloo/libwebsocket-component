/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#ifndef GATEWAY_SERVER_H
#define GATEWAY_SERVER_H

#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoopThreadPool.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpClient.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/ThreadLocal.h>
#include <muduo/base/ThreadPool.h>
#include <muduo/base/ThreadLocalSingleton.h>
#include <muduo/net/libwebsocket/server.h>

#include "muduo/net/http/HttpContext.h"
#include "muduo/net/http/HttpRequest.h"
#include "muduo/net/http/HttpResponse.h"

#include <boost/filesystem.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>  // memset
#include <string>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>
#include <map>
#include <list>
#include <vector>
#include <memory>
#include <iomanip>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "connector.h"

#include "public/packet.h"
#include "public/StdRandom.h"
#include "public/zookeeperclient/zookeeperclient.h"
#include "public/zookeeperclient/zookeeperlocker.h"
#include "public/redLock/redlock.h"
#include "public/RedisClient/RedisClient.h"
#include "public/MongoDB/MongoDBClient.h"
#include "public/traceMsg/traceMsg.h"

//#define NDEBUG

//@@ ServiceStateE 服务状态
enum ServiceStateE {
	kRepairing = 0,//维护中
	kRunning   = 1,//服务中
};

//@@ IpVisitCtrlE IP访问黑白名单控制
enum IpVisitCtrlE {
	kClose      = 0,
	kOpen       = 1,//应用层IP截断
	kOpenAccept = 2,//网络底层IP截断
};

//@@ IpVisitE
enum IpVisitE {
	kEnable  = 0,//IP允许访问
	kDisable = 1,//IP禁止访问
};

//@@ servTyE
enum servTyE {
	kHallTy    = 0,//大厅服
	kGameTy    = 1,//游戏服
	kMaxServTy,
};

/*
	HTTP/1.1 400 Bad Request\r\n\r\n
	HTTP/1.1 404 Not Found\r\n\r\n
	HTTP/1.1 405 服务维护中\r\n\r\n
	HTTP/1.1 500 IP访问限制\r\n\r\n
	HTTP/1.1 504 权限不够\r\n\r\n
	HTTP/1.1 505 timeout\r\n\r\n
	HTTP/1.1 600 访问量限制(1500)\r\n\r\n
*/

#define MY_TRY()	\
	try {

#define MY_CATCH() \
	} \
catch (const bsoncxx::exception & e) { \
	LOG_ERROR << __FUNCTION__ << " --- *** " << "exception caught " << e.what(); \
	abort(); \
} \
catch (const muduo::Exception & e) { \
	LOG_ERROR << __FUNCTION__ << " --- *** " << "exception caught " << e.what(); \
	abort(); \
	} \
catch (const std::exception & e) { \
	LOG_ERROR << __FUNCTION__ << " --- *** " << "exception caught " << e.what(); \
	abort(); \
} \
catch (...) { \
	LOG_ERROR << __FUNCTION__ << " --- *** " << "exception caught "; \
	throw; \
} \

static void setFailedResponse(muduo::net::HttpResponse& rsp,
	muduo::net::HttpResponse::HttpStatusCode code = muduo::net::HttpResponse::k200Ok,
	std::string const& msg = "") {
	rsp.setStatusCode(code);
	rsp.setStatusMessage("OK");
	rsp.addHeader("Server", "MUDUO");
#if 0
	rsp.setContentType("text/html;charset=utf-8");
	rsp.setBody("<html><body>" + msg + "</body></html>");
#elif 0
	rsp.setContentType("application/xml;charset=utf-8");
	rsp.setBody(msg);
#else
	rsp.setContentType("text/plain;charset=utf-8");
	rsp.setBody(msg);
#endif
}

//@@ Gateway
class Gateway : public muduo::noncopyable {
public:
	//@@ CmdCallback
	typedef std::function<
		void(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer&)> CmdCallback;
	//@@ CmdCallbacks
	typedef std::map<uint32_t, CmdCallback> CmdCallbacks;
	
	//typedef std::shared_ptr<muduo::net::TcpClient> TcpClientPtr;
	//typedef std::weak_ptr<muduo::net::TcpClient> WeakTcpClientPtr;
	//typedef std::map<std::string, TcpClientPtr> TcpClientMap;
	
	typedef std::shared_ptr<muduo::net::Buffer> BufferPtr;
	typedef std::map<std::string, std::string> HttpParams;

	//@@ Entry 避免恶意连接占用系统sockfd资源不请求处理也不关闭fd情况，超时强行关闭连接
	struct Entry : public muduo::noncopyable {
	public:
		//@@ Context
		struct Context : public muduo::copyable {
			explicit Context()
				: index_(0XFFFFFFFF) {
			}
			explicit Context(int index)
				: index_(index) {
				assert(index_ >= 0);
			}
			explicit Context(const boost::any& context)
				: index_(0XFFFFFFFF), context_(context) {
				assert(!context_.empty());
			}
			explicit Context(int index, const boost::any& context)
				: index_(index), context_(context) {
				assert(index_ >= 0);
				assert(!context_.empty());
			}
			~Context() {
			}
			inline void setWorkerIndex(int index) {
				index_ = index;
				assert(index_ >= 0);
			}
			inline int getWorkerIndex() const {
				return index_;
			}
			inline void setContext(const boost::any& context) {
				context_ = context;
			}
			inline const boost::any& getContext() const {
				return context_;
			}
			inline boost::any* getMutableContext() {
				return &context_;
			}
			//ipaddr
			void setIpaddr(in_addr_t inaddr) { ipaddr_ = inaddr; }
			in_addr_t getIpaddr() { return ipaddr_; }
			//session
			inline void setSession(std::string const& session) { session_ = session; }
			inline std::string const& getSession() const { return session_; }
			//userid
			inline void setUserId(int64_t userid) { userid_ = userid; }
			inline int64_t getUserId() const { return userid_; }
			//aeskey
			inline void setAesKey(std::string key) { aeskey_ = key; }
			inline std::string const& getAesKey() const { return aeskey_; }
			//setClientConn
			inline void setClientConn(servTyE ty,
				std::string const& name,
				muduo::net::WeakTcpConnectionPtr const& weakConn) {
				assert(!name.empty());
				client_[ty].first = name;
				client_[ty].second = weakConn; }
			//setClientConn
			inline void setClientConn(servTyE ty, ClientConn const& client) {
				client_[ty] = client;
			}
			//getClientConn
			inline ClientConn const& getClientConn(servTyE ty) { return client_[ty]; }
		public:
			//threadPool_下标
			int index_;
			uint32_t ipaddr_;
			int64_t userid_;
			std::string session_;
			std::string aeskey_;
			ClientConn client_[kMaxServTy];
			boost::any context_;
		};
		explicit Entry(const muduo::net::WeakTcpConnectionPtr& weakConn)
			: weakConn_(weakConn) {
		}
		explicit Entry(const muduo::net::WeakTcpConnectionPtr& weakConn, const boost::any& context)
			: weakConn_(weakConn), context_(context) {
			assert(!context_.empty());
		}
		inline muduo::net::WeakTcpConnectionPtr const& getWeakConnPtr() {
			return weakConn_;
		}
		inline void setContext(const boost::any& context) {
			context_ = context;
		}
		inline const boost::any& getContext() const {
			return context_;
		}
		inline boost::any* getMutableContext() {
			return &context_;
		}
		~Entry() {
			muduo::net::TcpConnectionPtr conn(weakConn_.lock());
			if (conn) {
#ifdef _DEBUG_BUCKETS_
				LOG_ERROR << __FUNCTION__ << " --- *** " << "客户端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
					<< conn->localAddress().toIpPort() << "] timeout closing";
#endif
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
			}
		}
		boost::any context_;
		muduo::net::WeakTcpConnectionPtr weakConn_;
	};
	typedef std::shared_ptr<Entry> EntryPtr;
	typedef std::weak_ptr<Entry> WeakEntryPtr;
	typedef boost::unordered_set<EntryPtr> Bucket;
	typedef boost::circular_buffer<Bucket> WeakConnList;

	typedef std::map<std::string, WeakEntryPtr> SessionInfosMap;
	//@@ EventLoopContext
	class EventLoopContext : public muduo::copyable {
	public:
		explicit EventLoopContext()
			: index_(0xFFFFFFFF) {
		}
		explicit EventLoopContext(int index)
			: index_(index) {
			assert(index_ >= 0);
		}
		explicit EventLoopContext(EventLoopContext const& ref) {
			index_ = ref.index_;
			pool_.clear();
#if 0
			std::copy(ref.pool_.begin(), ref.pool_.end(), pool_.begin());
#else
			std::copy(ref.pool_.begin(), ref.pool_.end(), std::back_inserter(pool_));
#endif
		}
		inline void setBucketIndex(int index) {
			index_ = index;
			assert(index_ >= 0);
		}
		inline int getBucketIndex() const {
			return index_;
		}
		inline void addWorkerIndex(int index) {
			pool_.emplace_back(index);
		}
		inline int allocWorkerIndex() {
			int index = nextPool_.getAndAdd(1) % pool_.size();
			if (index >= 0XFFFFFFFF) {
				nextPool_.getAndSet(-1);
				index = nextPool_.addAndGet(1);
			}
			assert(index >= 0 && index < pool_.size());
			return pool_[index];
		}
		~EventLoopContext() {
		}
	private:
		//bucketsPool_下标
		int index_;
		//threadPool_下标集合
		std::vector<int> pool_;
		//pool_游标
		muduo::AtomicInt32 nextPool_;
	};
	//@@ ConnectionBucket
	struct ConnectionBucket {
		explicit ConnectionBucket(muduo::net::EventLoop& loop, int index, size_t size)
			:loop_(loop), index_(index) {
			//指定时间轮盘大小(bucket桶大小)
			//即环形数组大小(size) >=
			//心跳超时清理时间(timeout) >
			//心跳间隔时间(interval)
			buckets_.resize(size);
#ifdef _DEBUG_BUCKETS_
			LOG_INFO << __FUNCTION__ << " --- *** [" << index << "] timeout = " << size << "s";
#endif
		}
		//tick检查，间隔1s，踢出超时conn
		void onTimer() {
			buckets_.push_back(Bucket());
#ifdef _DEBUG_BUCKETS_
			LOG_INFO << __FUNCTION__ << " --- *** [" << index_ << "]";
#endif
			//重启定时器超时检查
			loop_.runAfter(1.0f, std::bind(&ConnectionBucket::onTimer, this));
		}
		//连接成功，压入桶元素
		void pushBucket(EntryPtr const entry) {
			if (likely(entry)) {
				//muduo::net::TcpConnectionPtr conn(entry->weakConn_.lock());
				//if (likely(conn)) {
					//必须使用shared_ptr，持有entry引用计数(加1)
					buckets_.back().insert(entry);
#ifdef _DEBUG_BUCKETS_
					LOG_INFO << __FUNCTION__ << " --- *** [" << index_ << "]客户端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
						<< conn->localAddress().toIpPort() << "]";
#endif
				//}
			}
			else {
				//assert(false);
			}
		}
		//收到消息包，更新桶元素
		void updateBucket(EntryPtr const entry) {
			if (likely(entry)) {
				//muduo::net::TcpConnectionPtr conn(entry->weakConn_.lock());
				//if (likely(conn)) {
					//必须使用shared_ptr，持有entry引用计数(加1)
					buckets_.back().insert(entry);
#ifdef _DEBUG_BUCKETS_
					LOG_INFO << __FUNCTION__ << " --- *** [" << index_ << "]客户端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
						<< conn->localAddress().toIpPort() << "]";
#endif
				//}
			}
			else {
				//assert(false);
			}
		}
		//bucketsPool_下标
		int index_;
		WeakConnList buckets_;
		muduo::net::EventLoop& loop_;
	};
public:
	//Gateway ctor
	Gateway(muduo::net::EventLoop* loop,
		const muduo::net::InetAddress& listenAddr,
		const muduo::net::InetAddress& listenAddrInn,
		const muduo::net::InetAddress& listenAddrHttp,
		std::string const& cert_path, std::string const& private_key_path,
		std::string const& client_ca_cert_file_path = "",
		std::string const& client_ca_cert_dir_path = "");
	//Gateway dctor
	~Gateway();
	void quit();
	//zookeeper
	bool initZookeeper(std::string const& ipaddr);
	//RedisCluster
	bool initRedisCluster(std::string const& ipaddr, std::string const& passwd);
	bool initRedisCluster();
	//RedisLock
	bool initRedisLock();
	//MongoDB
	bool initMongoDB(std::string url);
	bool initMongoDB();
	//MongoDB/RedisCluster/RedisLock
	void threadInit();

	//启动worker线程
	//启动TCP监听客户端，websocket server_
	//启动TCP监听客户端，内部服务器之间交互 innServer_
	//启动TCP监听客户端，HTTP httpServer_
	void start(int numThreads, int numWorkerThreads, int maxSize);

	//网关服[S]端 <- 客户端[C]端，websocket
private:
	//客户端访问IP黑名单检查
	bool onCondition(const muduo::net::InetAddress& peerAddr);

	void onConnection(const muduo::net::TcpConnectionPtr& conn);

	void onConnected(
		const muduo::net::TcpConnectionPtr& conn,
		std::string const& ipaddr);

	void onMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, int msgType,
		muduo::Timestamp receiveTime);

	void asyncClientHandler(
		const WeakEntryPtr& weakEntry,
		BufferPtr& buf,
		muduo::Timestamp receiveTime);

	//刷新客户端访问IP黑名单信息
	//1.web后台更新黑名单通知刷新
	//2.游戏启动刷新一次
	//3.redis广播通知刷新一次
	void refreshBlackList();
	bool refreshBlackListSync();
	bool refreshBlackListInLoop();

	//网关服[S]端 <- 推送服[C]端，通知服
private:
	void onInnConnection(const muduo::net::TcpConnectionPtr& conn);

	void onInnMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

	void asyncInnHandler(
		const muduo::net::TcpConnectionPtr& conn,
		WeakEntryPtr const& weakEntry,
		BufferPtr& buf,
		muduo::Timestamp receiveTime);

	//网关服[S]端 <- HTTP客户端[C]端，WEB前端
private:
	//HTTP访问IP白名单检查
	bool onHttpCondition(const muduo::net::InetAddress& peerAddr);

	void onHttpConnection(const muduo::net::TcpConnectionPtr& conn);

	void onHttpMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

	void asyncHttpHandler(const WeakEntryPtr& weakEntry, muduo::Timestamp receiveTime);
	
	static std::string getRequestStr(muduo::net::HttpRequest const& req);
	
	static bool parseQuery(std::string const& queryStr, HttpParams& params, std::string& errmsg);

	void processHttpRequest(
		const muduo::net::HttpRequest& req, muduo::net::HttpResponse& rsp,
		muduo::net::InetAddress const& peerAddr,
		muduo::Timestamp receiveTime);

	//刷新HTTP访问IP白名单信息
	//1.web后台更新白名单通知刷新
	//2.游戏启动刷新一次
	//3.redis广播通知刷新一次
	void refreshWhiteList();
	bool refreshWhiteListSync();
	bool refreshWhiteListInLoop();

	//请求挂维护/恢复服务 status=0挂维护 status=1恢复服务
	bool repairServer(std::string const& queryStr);

	//请求挂维护/恢复服务 status=0挂维护 status=1恢复服务
	void repairServerNotify(std::string const& msg);

	//网关服[C]端 -> 大厅服[S]端
private:
	void onHallConnection(const muduo::net::TcpConnectionPtr& conn);

	void onHallMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

	void asyncHallHandler(
		const muduo::net::TcpConnectionPtr& conn,
		WeakEntryPtr const& weakEntry,
		BufferPtr& buf,
		muduo::Timestamp receiveTime);

	void sendHallMessage(
		WeakEntryPtr const& weakEntry,
		BufferPtr& buf, int64_t userId);
private:
	//监听客户端TCP请求(websocket)
	muduo::net::websocket::Server server_;
	
	//监听客户端TCP请求(内部服务器交互)
	muduo::net::TcpServer innServer_;

	//监听客户端TCP请求(HTTP，管理员维护)，WEB前端
	muduo::net::TcpServer httpServer_;

	//当前TCP连接数
	muduo::AtomicInt32 numConnected_;
	
	//网络I/O线程数，worker线程数
	int numThreads_, numWorkerThreads_;
	
	//Bucket池处理超时conn连接对象
	std::vector<ConnectionBucket> bucketsPool_;
	
	//累计接收请求数，累计未处理请求数
	muduo::AtomicInt64 numTotalReq_, numTotalBadReq_;
	
	//主线程EventLoop，I/O监听/连接读写 accept(read)/connect(write)
	//muduo::net::EventLoop* loop_;

	//网络I/O线程，I/O收发读写 recv(read)/send(write)
	//std::shared_ptr<muduo::net::EventLoopThreadPool> threadPool_io_;

	//worker线程池，内部任务消息队列
	std::vector<std::shared_ptr<muduo::ThreadPool>> threadPool_;

	//保存玩家会话信息[session] = conn，entry超时/session过期/玩家离线清理
	SessionInfosMap sessInfos_;
	mutable boost::shared_mutex sessInfos_mutex_;

	//命令消息回调处理函数
	CmdCallbacks handlers_;
	
	//连接到所有大厅服
	Connector hallConector_;
	
	//连接到所有游戏服
	Connector gameConnector_;

	//玩家session哈希散列
	std::hash<std::string> hash_session_;

	STD::Random randomHall_;
private:
	void connectHallServer(std::string const& ipaddr);

	void connectHallServers();
	
	void ProcessHallIps(std::vector<std::string> const& newIps);

	//所有大厅服节点
	std::vector<std::string> hallIps_;
	muduo::MutexLock hallIps_mutex_;

	void ZookeeperConnectedHandler();
	void GetHallChildrenWatcherHandler(
		int type, int state,
		const std::shared_ptr<ZookeeperClient>& zkClientPtr,
		const std::string& path, void* context);

	//zk节点/服务注册/发现
	std::shared_ptr<ZookeeperClient> zkclient_;
	std::string nodePath_, nodeValue_, invalidNodePath_;
	
	//redis订阅/发布
	std::shared_ptr<RedisClient>  redisClient_;
	std::string redisIpaddr_;
	std::string redisPasswd_;
	
	//MongoDB
	std::string mongoDBUrl_;

	//服务状态
	volatile long serverState_;

public:
	//是否调试
	bool isdebug_;

	//绑定网卡ipport
	std::string strIpAddr_;

	//最大连接数限制
	int kMaxConnections_;
	
	//指定时间轮盘大小(bucket桶大小)
	//即环形数组大小(size) >=
	//心跳超时清理时间(timeout) >
	//心跳间隔时间(interval)
	int kTimeoutSeconds_, kHttpTimeoutSeconds_;
	
	//redisLock
	std::vector<std::string> redlockVec_;
	
	//管理员挂维护/恢复服务
	std::map<in_addr_t, IpVisitE> adminList_;

	//HTTP访问IP白名单控制
	IpVisitCtrlE whiteListControl_;

	//HTTP访问IP白名单信息
	std::map<in_addr_t, IpVisitE> whiteList_;
	mutable boost::shared_mutex whiteList_mutex_;

	//客户端访问IP黑名单控制，websocket
	IpVisitCtrlE blackListControl_;

	//客户端访问IP黑名单信息，websocket
	std::map<in_addr_t, IpVisitE> blackList_;
	mutable boost::shared_mutex blackList_mutex_;
};

#endif
