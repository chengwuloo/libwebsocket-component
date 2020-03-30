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
#include <muduo/net/Reactor.h>

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

#include "packet.h"
#include "connector.h"

#include "public/zookeeperclient/zookeeperclient.h"
#include "public/zookeeperclient/zookeeperlocker.h"
#include "public/redLock/redlock.h"
#include "public/RedisClient/RedisClient.h"
#include "public/MongoDB/MongoDBClient.h"
#include "public/traceMsg/traceMsg.h"
#include "public/StdRandom.h"
//#define NDEBUG

//@@ Gateway
class Gateway : public muduo::noncopyable {
public:
	//@@
	typedef std::function<
		void(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer&)> CmdCallback;
	typedef std::map<uint32_t, CmdCallback> CmdCallbacks;
	
	//typedef std::shared_ptr<muduo::net::TcpClient> TcpClientPtr;
	//typedef std::weak_ptr<muduo::net::TcpClient> WeakTcpClientPtr;
	//typedef std::map<std::string, TcpClientPtr> MapTcpClientPtr;
	
	typedef std::shared_ptr<muduo::net::Buffer> BufferPtr;

	//@@ servTyE
	enum servTyE {
		kHallTy  = 0,
		kGameTy  = 1,
		kMaxServTy,
	};

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

	//typedef std::map<std::string, muduo::net::WeakTcpConnectionPtr> SessionInfosMap;
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
		const muduo::net::InetAddress& listenAddrInner,
		std::string const& cert_path, std::string const& private_key_path,
		std::string const& client_ca_cert_file_path = "",
		std::string const& client_ca_cert_dir_path = "");
	//Gateway dctor
	~Gateway();

	//MongoDB/redis与线程池关联
	void threadInit();

	//启动websocket(TCP)业务线程
	//启动websocket(TCP)监听，客户端交互
	void start(int numThreads, int workerNumThreads, int maxSize);

	//网关服[S]端 <- 客户端[C]端，websocket
private:
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

	//当前TCP连接数
	muduo::AtomicInt32 numConnected_;
	
	//网络I/O线程数，worker线程数
	int numThreads_, workerNumThreads_;
	
	//最大连接数限制
	int kMaxConnections_;
	
	//指定时间轮盘大小(bucket桶大小)
	//即环形数组大小(size) >=
	//心跳超时清理时间(timeout) >
	//心跳间隔时间(interval)
	int kTimeoutSeconds_;

	//Bucket池处理超时conn连接对象
	std::vector<ConnectionBucket> bucketsPool_;
	
	//累计接收请求数，累计未处理请求数
	muduo::AtomicInt64 numTotalReq_, numTotalBadReq_;
	
	//主线程EventLoop，处理网络I/O读写：监听/连接 accept(read)/connect(write)
	//muduo::net::EventLoop* loop_;

	//网络I/O线程池，处理网络I/O读写：数据收/发 recv(read)send(write)
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

	//绑定网卡IPPORT
	std::string strIpAddr_;

	STD::Random randomHall_;
private:
	void connectHallServer(std::string const& ipaddr);

	void connectHallServers();
	
	void ProcessHallIps(std::vector<std::string> const& newIps);

	//所有大厅服节点
	std::vector<std::string> hallIps_;
	muduo::MutexLock hallIps_mutex_;

	bool initZookeeper(std::string const& ipaddr);
	bool initRedisCluster();
	bool initRedisCluster(std::string const& ipaddr, std::string const& passwd);
	
	void ZookeeperConnectedHandler();
	void GetHallChildrenWatcherHandler(
		int type, int state,
		const std::shared_ptr<ZookeeperClient>& zkClientPtr,
		const std::string& path, void* context);

	//zk节点/服务注册/发现
	std::shared_ptr<ZookeeperClient> zkclient_;
	std::string nodePath_, nodeValue_;
	
	//redis订阅/发布
	std::shared_ptr<RedisClient>  redisClient_;
	std::string redisIpaddr_;
	std::string redisPasswd_;
};

#endif
