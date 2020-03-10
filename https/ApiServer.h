#ifndef APISERVER_H
#define APISERVER_H

#include <map>
#include <list>

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

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>
#include <boost/random.hpp>
#include <boost/thread.hpp>
#include <boost/regex.hpp>

#include "muduo/base/Logging.h"
#include "muduo/net/http/HttpContext.h"
#include "muduo/net/http/HttpRequest.h"
#include "muduo/net/http/HttpResponse.h"

#include <map>
#include <string>
#include "BackCodeSet.h"

#include <ctime>
#include <iostream>
#include <iomanip>

//#define _STAT_ORDER_QPS_DETAIL_	//上下分QPS细节，单个函数处理性能指标
//#define _NO_LOGIC_PROCESS_		//不带逻辑空请求
//#define _EVENTLOOP_CONTEXT_       //最高效的cpu利用率，没有线程切换开销，没有共享锁竞态 ///

//IP访问白名单控制 ///
enum eWhiteListCtrl {
	Close      = 0,
	Open       = 1,//应用层IP截断
	OpenAccept = 2,//网络底层IP截断
};

//
//GET 请求 ///
//GET /GameHandle?agentid=10000&timestamp=1578915580677&type=2&paraValue=V8UZBagKU%2bZbi0kZ7JTObRHE2caSgh%2bxYBDvJDteQrx5PS%2fR%2bnDzTGLZTLJG4wjhqDuvpfujZOsoYoU04IzWesqGT%2fwhWZ1uNTmMHdNIGZmYSmY1%2fM%2bHAfl%2fI6%2bgKhzntB8JrspAaUmHTVwdoVcu7n22EJrD2%2f%2baldjiFkay2nScgCQKQFgFTMQ3ICH64UDU&key=5EF05FD510E7CE1A66842CF92510F474 HTTP/1.1\r\n
//User-Agent: curl/7.29.0\r\n
//Host: 192.168.2.93:8080\r\n
//Accept: */*\r\n\r\n
//
//POST 请求 ///
//POST /GameHandle HTTP/1.1\r\n
//User-Agent: curl/7.29.0\r\n
//Host: 192.168.2.93:8080\r\n
//Accept: */*\r\n
//Content-Length: 306\r\n
//Content-Type: application/x-www-form-urlencoded\r\n\r\n
//agentid=10000&timestamp=1578915580677&type=2&paraValue=V8UZBagKU%2bZbi0kZ7JTObRHE2caSgh%2bxYBDvJDteQrx5PS%2fR%2bnDzTGLZTLJG4wjhqDuvpfujZOsoYoU04IzWesqGT%2fwhWZ1uNTmMHdNIGZmYSmY1%2fM%2bHAfl%2fI6%2bgKhzntB8JrspAaUmHTVwdoVcu7n22EJrD2%2f%2baldjiFkay2nScgCQKQFgFTMQ3ICH64UDU&key=5EF05FD510E7CE1A66842CF92510F474
//

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

/// A simple embeddable HTTP server designed for report status of a program.
/// It is not a fully HTTP 1.1 compliant server, but provides minimum features
/// that can communicate with HttpClient and Web browser.
/// It is synchronous, just like Java Servlet.
class ApiServer : public muduo::noncopyable
{
public:
	typedef std::map<std::string, muduo::net::WeakTcpConnectionPtr> WeakConnMap;
	typedef std::shared_ptr<muduo::net::TcpClient> TcpClientPtr;
	typedef std::map<std::string, TcpClientPtr> TcpClientMap;

	typedef std::shared_ptr<muduo::net::Buffer> BufferPtr;

	typedef std::map<std::string, std::string> HttpParams;

    typedef std::function<void(
		muduo::net::WeakTcpConnectionPtr&,
		uint8_t*, int, internal_prev_header*)> AccessCommandFunctor;

	typedef std::function<void(
		const muduo::net::HttpRequest&,
		muduo::net::HttpResponse&, muduo::Timestamp receiveTime)> HttpCallback;

	muduo::net::EventLoop* getLoop() const { return server_.getLoop(); }

public:
	ApiServer(muduo::net::EventLoop* loop,
		const muduo::net::InetAddress& listenAddr,
		std::string const& cert_path, std::string const& private_key_path,
		std::string const& client_ca_cert_file_path = "",
		std::string const& client_ca_cert_dir_path = "");
    ~ApiServer();
	//避免恶意连接占用系统sockfd资源不请求处理也不关闭fd情况，超时强行关闭连接 ///
	//typedef std::weak_ptr<muduo::net::TcpConnection> WeakTcpConnectionPtr;
	//Entry ///
	struct Entry : public muduo::copyable {
		//Context ///
		struct Context : public muduo::copyable {
			explicit Context()
				: index_(0XFFFFFFFF) {
			}
			explicit Context(int index)
				: index_(index) {
				assert(index_ >= 0);
			}
			explicit Context(const boost::any& context)
				: index_(0XFFFFFFFF) {
				setContext(context);
			}
			explicit Context(int index, const boost::any& context)
				: index_(index) {
				assert(index_ >= 0);
				setContext(context);
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
			~Context() {
			}
			//threadPool_下标 ///
			int index_;
			boost::any context_;
		};
		explicit Entry(const muduo::net::WeakTcpConnectionPtr& weakConn)
			: weakConn_(weakConn) {
		}
		explicit Entry(const muduo::net::WeakTcpConnectionPtr& weakConn, int index)
			: weakConn_(weakConn), ctx_(index) {
		}
		explicit Entry(const muduo::net::WeakTcpConnectionPtr& weakConn, const boost::any& context)
			: weakConn_(weakConn), ctx_(context) {
		}
		explicit Entry(const muduo::net::WeakTcpConnectionPtr& weakConn, int index, const boost::any& context)
			: weakConn_(weakConn), ctx_(index, context) {
		}
		inline void setWorkerIndex(int index) {
			ctx_.setWorkerIndex(index);
		}
		inline int getWorkerIndex() const {
			return ctx_.getWorkerIndex();
		}
		inline void setContext(const boost::any& context) {
			ctx_.setContext(context);
		}
		inline const boost::any& getContext() const {
			return ctx_.getContext();
		}
		inline boost::any* getMutableContext() {
			return ctx_.getMutableContext();
		}
		inline WeakTcpConnectionPtr const& getWeakConnPtr() {
			return weakConn_;
		}
		~Entry() {
			muduo::net::TcpConnectionPtr conn(weakConn_.lock());
			if (conn) {
#ifdef _DEBUG_BUCKETS_
				LOG_ERROR << __FUNCTION__ << " --- *** " << "WEB端[" << conn->peerAddress().toIpPort() << "] -> HTTP服["
					<< conn->localAddress().toIpPort() << "] 超时强制关闭";
#endif
#if 0
				//不再接收数据
				conn->shutdown();
#elif 0
				//直接强制关闭连接
				conn->forceClose();
#else
				//HTTP应答包(header/body)
				muduo::net::HttpResponse rsp(false);
				setFailedResponse(rsp,
					muduo::net::HttpResponse::k404NotFound,
					"HTTP/1.1 505 timeout\r\n\r\n");
				muduo::net::Buffer buf;
				rsp.appendToBuffer(&buf);
				conn->send(&buf);

				//延迟0.2s强制关闭连接
				conn->forceCloseWithDelay(0.2f);
#endif
			}
		}
		//上下文信息 ///
		Entry::Context ctx_;
		//为了安全使用weak_ptr弱指针 ///
		WeakTcpConnectionPtr weakConn_;
	};
	typedef std::shared_ptr<Entry> EntryPtr;
	typedef std::weak_ptr<Entry> WeakEntryPtr;
	typedef std::unordered_set<EntryPtr> Bucket;
	typedef boost::circular_buffer<Bucket> WeakConnectionList;
	//EventLoopContext ///
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
			//为conn指定一个逻辑处理线程 ///
			int index = nextPool_.getAndAdd(1) % pool_.size();

			//防止nextPool_类型溢出 ///
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
		//bucketsPool_下标 ///
		int index_;
		//threadPool_下标集合 ///
		std::vector<int> pool_;
		//pool_游标 ///
		muduo::AtomicInt32 nextPool_;
	};
	//HashConnectionBucket，通过哈希散列(hash)无锁化(lockfree)处理 ///
	struct HashConnectionBucket {
		explicit HashConnectionBucket(muduo::net::EventLoop& loop, int index, size_t size)
			:loop_(loop), index_(index) {
			//指定时间轮盘大小(bucket桶大小) ///
			//即环形数组大小(size) >=
			//心跳超时清理时间(timeout) >
			//心跳间隔时间(interval)
			buckets_.resize(size);
#ifdef _DEBUG_BUCKETS_
			LOG_INFO << __FUNCTION__ << " --- *** [" << index << "]WEB端超时清理时间(timeout) = " << size << "s";
#endif
		}
		//tick检查，间隔1s，踢出超时conn ///
		void onTimer() {
			//Entry析构时延迟0.2s强制关闭连接 ///
			buckets_.push_back(Bucket());
#ifdef _DEBUG_BUCKETS_
			LOG_INFO << __FUNCTION__ << " --- *** [" << index_ << "]Tick清理超时连接...";
#endif
			//重启超时检查定时器 ///
			loop_.runAfter(1.0f, std::bind(&HashConnectionBucket::onTimer, this));
		}
		//连接成功，压入桶元素 ///
		void pushBucket(EntryPtr const entry) {
			if (likely(entry)) {
				//muduo::net::TcpConnectionPtr conn(entry->weakConn_.lock());
				//if (likely(conn)) {
					//必须使用shared_ptr，持有entry引用计数(加1) ///
					buckets_.back().insert(entry);
#ifdef _DEBUG_BUCKETS_
					LOG_INFO << __FUNCTION__ << " --- *** [" << index_ << "]WEB端[" << conn->peerAddress().toIpPort() << "] -> HTTP服["
						<< conn->localAddress().toIpPort() << "] 压入Bucket桶!";
#endif
				//}
			}
			else {
				//assert(false);
			}
		}
		//收到消息包，更新桶元素 ///
		void updateBucket(EntryPtr const entry) {
			if (likely(entry)) {
				//muduo::net::TcpConnectionPtr conn(entry->weakConn_.lock());
				//if (likely(conn)) {
					//必须使用shared_ptr，持有entry引用计数(加1) ///
					buckets_.back().insert(entry);
#ifdef _DEBUG_BUCKETS_
					LOG_INFO << __FUNCTION__ << " --- *** [" << index_ << "]WEB端[" << conn->peerAddress().toIpPort() << "] -> HTTP服["
						<< conn->localAddress().toIpPort() << "] 更新Bucket桶!";
#endif
				//}
			}
			else {
				//assert(false);
			}
		}
		//bucketsPool_下标 ///
		int index_;
		WeakConnectionList buckets_;
		muduo::net::EventLoop& loop_;
	};
	std::vector<HashConnectionBucket> bucketsPool_;
	
	/// Not thread safe, callback be registered before calling start().
	void setHttpCallback(const HttpCallback& cb)
	{
		httpCallback_ = cb;
	}
public:
	//启动HTTP业务线程 ///
	//启动HTTP监听 ///
	void start(int numThreads, int workerNumThreads, int maxSize);

	//白名单检查 ///
	bool onConnection(const InetAddress& peerAddr);

	//Connected/closed事件 ///
	void onHttpConnection(const muduo::net::TcpConnectionPtr& conn);

	//接收HTTP网络消息回调 ///
	void onHttpMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

	//异步回调 ///
	void asyncHttpHandler(const WeakEntryPtr& weakEntry/*, const muduo::net::HttpRequest& req*/, muduo::Timestamp receiveTime);
	
	//处理HTTP回调 ///
	void processHttpRequest(const muduo::net::HttpRequest& req, muduo::net::HttpResponse& rsp, muduo::net::InetAddress const& peerAddr, muduo::Timestamp receiveTime);

	//请求字符串 ///
	static std::string getRequestStr(muduo::net::HttpRequest const& req);

	//解析请求 ///
	//strQuery string req.query()
	static bool parseQuery(std::string const& queryStr, HttpParams& params, std::string& errmsg);
public:
	//I/O线程数，worker线程数
	int numThreads_, workerNumThreads_;
	//指定网卡ipaddr
	std::string strIpAddr_;
	//监听HTTP请求
	muduo::net::TcpServer server_;
	//添加OpenSSL认证支持 ///
	muduo::net::ssl::SSL_CTX_Init ssl_ctx_init_;

	//threadPool_游标 ///
	muduo::AtomicInt32 nextThreadPool_;
	//业务线程池，内部自实现任务消息队列
	std::vector<std::shared_ptr<muduo::ThreadPool>> threadPool_;

	//请求处理回调，但非线程安全的
	HttpCallback httpCallback_;

	//统计TCP/HTTP连接数量，"Connection:keep-alive"
	muduo::AtomicInt32 numConnected_;
	//累计接收请求数，累计未处理请求数 ///
	muduo::AtomicInt64 numTotalReq_, numTotalBadReq_;

	//最大连接数限制 ///
	int kMaxConnections_;

	//指定时间轮盘大小(bucket桶大小) ///
	//即环形数组大小(size) >=
	//心跳超时清理时间(timeout) >
	//心跳间隔时间(interval)
	int kTimeoutSeconds_;
private:
	/* 返回格式 ///
	{
		"maintype": "/GameHandle",
			"type": 2,
			"data":
			{
				"orderid":"",
				"agentid": 10000,
				"account": "999",
				"score": 10000,
				"code": 0,
				"errmsg":"",
			}
	}
	*/
	// 构造返回结果 ///
	std::string createResponse(
		int32_t opType,
		std::string const& orderId,
		uint32_t agentId,
		std::string account, int64_t score,
		int errcode, std::string const& errmsg, bool debug);

	//最近一次请求(上分或下分操作的elapsed detail)
	static void createLatestElapsed(
		boost::property_tree::ptree& latest,
		std::string const& op, std::string const& key, double elapsed);
	
	//监控数据
	static std::string createMonitorData(
		boost::property_tree::ptree const& latest, double totalTime, int timeout,
		int64_t requestNum, int64_t requestNumSucc, int64_t requestNumFailed, double ratio,
		int64_t requestNumTotal, int64_t requestNumTotalSucc, int64_t requestNumTotalFailed, double ratioTotal, int testTPS);

	//判断是否数字组成的字符串 ///
	static inline bool IsDigitStr(std::string const& str) {
#if 0
		boost::regex reg("^[1-9]\d*\,\d*|[1-9]\d*$");
#elif 1
		boost::regex reg("^[0-9]+([.]{1}[0-9]+){0,1}$");
#elif 0
		boost::regex reg("^[+-]?(0|([1-9]\d*))(\.\d+)?$");
#elif 0
		boost::regex reg("[1-9]\d*\.?\d*)|(0\.\d*[1-9]");
#endif
		return boost::regex_match(str, reg);
	}
	
public:
	// 代理信息 ///
	struct agent_info_t {
		int64_t		score;              //代理分数 
		int32_t		status;             //是否被禁用 0正常 1停用
		int32_t		agentId;            //agentId
		int32_t		cooperationtype;    //合作模式  1 买分 2 信用
		std::string descode;            //descode 
		std::string md5code;            //MD5 
	};

	//刷新所有agent_info信息 ///
	//1.web后台更新代理通知刷新
	//2.游戏启动刷新一次
	//3.redis广播通知刷新一次
	bool refreshAgentInfo();
	
	//刷新所有IP访问白名单信息 ///
	//1.web后台更新白名单通知刷新
	//2.游戏启动刷新一次
	//3.redis广播通知刷新一次 ///
	void refreshWhiteList();
	//同步刷新IP访问白名单
	bool refreshWhiteListSync();
	bool refreshWhiteListInLoop();
	
	//订单处理函数 ///
	std::string OrderProcess(std::string const& reqStr, muduo::Timestamp receiveTime, int& errcode, std::string& errmsg, boost::property_tree::ptree& latest, int& testTPS);
	
	//上下分操作 ///
	int doOrderExecute(int32_t opType, std::string const& account, int64_t score, agent_info_t& _agent_info, std::string const& orderId, std::string& errmsg, boost::property_tree::ptree& latest, int& testTPS);
	
	//上分写db操作 ///
	int AddOrderScore(std::string const& account, int64_t score, agent_info_t& _agent_info, std::string const& orderId, std::string& errmsg, boost::property_tree::ptree& latest, int& testTPS);
	
	//下分写db操作 ///
	int SubOrderScore(std::string const& account, int64_t score, agent_info_t& _agent_info, std::string const& orderId, std::string& errmsg, boost::property_tree::ptree& latest, int& testTPS);

	//代理信息map[agentId]=agent_info_
	std::map<int32_t, agent_info_t> agent_info_;
	mutable boost::shared_mutex agent_info_mutex_;
	bool isdecrypt_;

	//IP访问白名单信息 ///
	std::map<in_addr_t, eApiVisit> white_list_;
	mutable boost::shared_mutex white_list_mutex_;
	//Accept时候判断，socket底层控制，否则开启异步检查 ///
	eWhiteListCtrl whiteListControl_;
	//管理员挂维护/恢复服务 ///
	std::map<in_addr_t, eApiVisit> admin_list_;

	//redis分布式锁 ///
	std::vector<std::string> redlockVec_;
	//上下分操作间隔时间(针对用户/代理) ///
	int ttlUserLockSeconds_, ttlAgentLockSeconds_;
private:
    const static size_t kHeaderLen = sizeof(int16_t);
    std::shared_ptr<muduo::net::EventLoopThread> threadTimer_;
public:
#ifdef _STAT_ORDER_QPS_
	//性能测试指标 间隔输出时间(s) ///
	int deltaTime_;
#endif
};

#endif // ApiServer_H
