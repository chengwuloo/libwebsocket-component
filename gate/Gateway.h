/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#ifndef GATEWAY_SERVER_H
#define GATEWAY_SERVER_H

#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Exception.h>
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

#include "Clients.h"
#include "EntryPtr.h"
#include "Entities.h"
#include "Container.h"

#include "public/packet.h"
#include "public/StdRandom.h"
#include "public/zookeeperclient/zookeeperclient.h"
#include "public/zookeeperclient/zookeeperlocker.h"
#include "public/redLock/redlock.h"
#include "public/RedisClient/RedisClient.h"
#include "public/MongoDB/MongoDBClient.h"
#include "public/traceMsg/traceMsg.h"

//#define NDEBUG
#define KICK_GS                 (0x01)
#define KICK_HALL               (0x02)
#define KICK_CLOSEONLY          (0x100)
#define KICK_LEAVEGS            (0x200)

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

//@@
typedef std::shared_ptr<muduo::net::Buffer> BufferPtr;
typedef std::map<std::string, std::string> HttpParams;

//@@ Gateway
class Gateway : public muduo::noncopyable {
public:
	//@@ CmdCallback
	typedef std::function<
		void(const muduo::net::TcpConnectionPtr&,
			muduo::net::Buffer*)> CmdCallback;
	//@@ CmdCallbacks
	typedef std::map<uint32_t, CmdCallback> CmdCallbacks;
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
	void init();
	void quit();
private:
	void onGetAesKey(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* msg);
public:
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
	//启动TCP监听客户端，内部推送通知服务 innServer_
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
		muduo::net::WeakTcpConnectionPtr const& weakConn,
		BufferPtr& buf,
		muduo::Timestamp receiveTime);

	void asyncOfflineHandler(ContextPtr const& entryContext);

	static BufferPtr packClientShutdownMsg(int64_t userid, int status = 0);

	static BufferPtr packNoticeMsg(
		int32_t agentid, std::string const& title,
		std::string const& content, int msgtype);

	void broadcastNoticeMsg(
		std::string const& title,
		std::string const& msg,
		int32_t agentid, int msgType);

	void broadcastMessage(int mainID, int subID, ::google::protobuf::Message* msg);

	//刷新客户端访问IP黑名单信息
	//1.web后台更新黑名单通知刷新
	//2.游戏启动刷新一次
	//3.redis广播通知刷新一次
	void refreshBlackList();
	bool refreshBlackListSync();
	bool refreshBlackListInLoop();

	//网关服[S]端 <- 推送服[C]端，推送通知服务
private:
	void onInnConnection(const muduo::net::TcpConnectionPtr& conn);

	void onInnMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

	void asyncInnHandler(
		muduo::net::WeakTcpConnectionPtr const& weakConn,
		BufferPtr& buf,
		muduo::Timestamp receiveTime);

	void onMarqueeNotify(std::string const& msg);

	void onLuckPushNotify(std::string const& msg);

	//网关服[S]端 <- HTTP客户端[C]端，WEB前端
private:
	//HTTP访问IP白名单检查
	bool onHttpCondition(const muduo::net::InetAddress& peerAddr);

	void onHttpConnection(const muduo::net::TcpConnectionPtr& conn);

	void onHttpMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

	void asyncHttpHandler(muduo::net::WeakTcpConnectionPtr const& weakConn, muduo::Timestamp receiveTime);
	
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
	bool repairServer(servTyE servTy, std::string const& servname, std::string const& name, int status, std::string& rspdata);

	bool repairServer(std::string const& queryStr, std::string& rspdata);

	void repairServerNotify(std::string const& msg, std::string& rspdata);

	//网关服[C]端 -> 大厅服[S]端
private:
	void onHallConnection(const muduo::net::TcpConnectionPtr& conn);

	void onHallMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

	void asyncHallHandler(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::WeakTcpConnectionPtr const& weakConn,
		BufferPtr& buf,
		muduo::Timestamp receiveTime);

	void sendHallMessage(
		ContextPtr const& entryContext,
		BufferPtr& buf, int64_t userid);

	//跨网关顶号处理(异地登陆)
	void onUserReLoginNotify(std::string const& msg);

	void onUserOfflineHall(ContextPtr const& entryContext);

	//网关服[C]端 -> 游戏服[S]端
private:
	void onGameConnection(const muduo::net::TcpConnectionPtr& conn);

	void onGameMessage(
		const muduo::net::TcpConnectionPtr& conn,
		muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

	void asyncGameHandler(
		muduo::net::WeakTcpConnectionPtr const& weakConn,
		BufferPtr& buf,
		muduo::Timestamp receiveTime);

	void sendGameMessage(
		ContextPtr const& entryContext,
		BufferPtr& buf, int64_t userid);

	void onUserOfflineGame(ContextPtr const& entryContext, bool leave = 0);
private:
	//监听客户端TCP请求(websocket)
	muduo::net::TcpServer server_;
	
	//监听客户端TCP请求，内部推送通知服务
	muduo::net::TcpServer innServer_;

	//监听客户端TCP请求(HTTP，管理员维护)，WEB前端
	muduo::net::TcpServer httpServer_;

	//当前TCP连接数
	muduo::AtomicInt32 numConnected_;
	
	//网络I/O线程数，worker线程数
	int numThreads_, numWorkerThreads_;
	
	//Bucket池处理连接超时conn对象
	std::vector<ConnBucketPtr> bucketsPool_;
	
	//累计接收请求数，累计未处理请求数
	muduo::AtomicInt64 numTotalReq_, numTotalBadReq_;
	
	//主线程EventLoop，I/O监听/连接读写 accept(read)/connect(write)
	//muduo::net::EventLoop* loop_;

	//网络I/O线程池，I/O收发读写 recv(read)/send(write)
	//std::shared_ptr<muduo::net::EventLoopThreadPool> threadPool_io_;

	//worker线程池，内部任务消息队列
	std::vector<std::shared_ptr<muduo::ThreadPool>> threadPool_;
	
	//公共定时器
	std::shared_ptr<muduo::net::EventLoopThread> threadTimer_;

	//map[session] = weakConn
	STR::Entities entities_;
	
#ifdef MAP_USERID_SESSION
	//map[userid] = session
	INT::Sessions sessions_;
#else
	//map[userid] = weakConn
	INT::Entities sessions_;
#endif

	//命令消息回调处理函数
	CmdCallbacks handlers_;
	
	//连接到所有大厅服
	Connector hallClients_;
	
	//连接到所有游戏服
	Connector gameClients_;

	//所有大厅服/游戏服节点
	Container clients_[kMaxServTy];

	//一致性哈希散列
	//对玩家session进行一致性hash运算，使得玩家worker线程处理均匀分配(worker线程负载均衡)
	std::hash<std::string> hash_session_;

	STD::Random randomHall_;
private:
	void ZookeeperConnectedHandler();
	
	void onHallWatcherHandler(
		int type, int state,
		const std::shared_ptr<ZookeeperClient>& zkClientPtr,
		const std::string& path, void* context);
	
	void onGameWatcherHandler(
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
