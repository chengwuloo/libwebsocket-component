#include "Gateway.h"
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <unistd.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/regex.hpp>

#include "public/Env.h"
#include "public/NetCardIP.h"

Gateway *gServer = NULL;
muduo::net::EventLoop *gMainEventLoop = NULL;

//StopService 服务终止
static void StopService(int signo) {
	if (gServer) {
		gServer->quit();
	}

	if (gMainEventLoop) {
		gMainEventLoop->quit();
	}
}

#if 1
int main() {
	//程序配置
	if (!boost::filesystem::exists("./conf/game.conf")) {
		LOG_INFO << "./conf/game.conf not exists";
		return -1;
	}
	boost::property_tree::ptree pt;
	boost::property_tree::read_ini("./conf/game.conf", pt);

	//日志目录 logdir/logname
	std::string logdir = pt.get<std::string>("Gateway.logdir", "./log/Gateway/");
	std::string logname = pt.get<std::string>("Gateway.logname", "Gateway");
	if (setEnv(logdir, logname) < 0) {
		return -1;
	}

	//日志级别
	int loglevel = pt.get<int>("Gateway.loglevel", 1);
	muduo::Logger::setLogLevel((muduo::Logger::LogLevel)loglevel);
	LOG_INFO << __FUNCTION__ << " --- *** " << "日志级别 = " << loglevel;

	//获取指定网卡ipaddr
	std::string strIpAddr;
	std::string netcardName = pt.get<std::string>("Global.netcardName", "eth0");
	IpByNetCardName(netcardName, strIpAddr);
	LOG_INFO << __FUNCTION__ << " --- *** " << "网卡名称 = " << netcardName << " - " << strIpAddr;

	//////////////////////////////////////////////////////////////////////////
	//zookeeper
	std::string strZookeeperIps = "";
	{
		auto const& childs = pt.get_child("Zookeeper");
		for (auto const& child : childs) {
			if (child.first.substr(0, 7) == "Server.") {
				if (!strZookeeperIps.empty()) {
					strZookeeperIps += ",";
				}
				strZookeeperIps += child.second.get_value<std::string>();
			}
		}
		LOG_INFO << __FUNCTION__ << " --- *** " << "ZookeeperIP = " << strZookeeperIps;
	}
	//////////////////////////////////////////////////////////////////////////
	//RedisCluster
	std::map<std::string, std::string> mapRedisClusterIps;
	std::string redisPasswd = pt.get<std::string>("RedisCluster.Password", "");
	std::string strRedisClusterIps = "";
	{
		auto const& childs = pt.get_child("RedisCluster");
		for (auto const& child : childs) {
			if (child.first.substr(0, 9) == "Sentinel.") {
				if (!strRedisClusterIps.empty()) {
					strRedisClusterIps += ",";
				}
				strRedisClusterIps += child.second.get_value<std::string>();
			}
			else if (child.first.substr(0, 12) == "SentinelMap.") {
				std::string const& ipport = child.second.get_value<std::string>();
				std::vector<std::string> vec;
				boost::algorithm::split(vec, ipport, boost::is_any_of(","));
				assert(vec.size() == 2);
				mapRedisClusterIps[vec[0]] = vec[1];
			}
		}
		LOG_INFO << __FUNCTION__ << " --- *** " << "RedisClusterIP = " << strRedisClusterIps;
	}
	//////////////////////////////////////////////////////////////////////////
	//redisLock
	std::string strRedisLockIps = "";
	{
		auto const& childs = pt.get_child("RedisLock");
		for (auto const& child : childs) {
			if (child.first.substr(0, 9) == "Sentinel.") {
				if (!strRedisLockIps.empty()) {
					strRedisLockIps += ",";
				}
				strRedisLockIps += child.second.get_value<std::string>();
			}
		}
		LOG_INFO << __FUNCTION__ << " --- *** " << "RedisLockIP = " << strRedisLockIps;
	}
	//////////////////////////////////////////////////////////////////////////
	//MongoDB
	std::string strMongoDBUrl = pt.get<std::string>("MongoDB.Url");

	//TCP监听客户端，websocket server_
	int16_t tcpPort = pt.get<int>("Gateway.port", 8010);
	//TCP监听客户端，内部推送通知服务 innServer_
	int16_t innPort = pt.get<int>("Gateway.innPort", 9010);
	//TCP监听客户端，HTTP httpServer_
	uint16_t httpPort = pt.get<int>("Gateway.httpPort", 8120);
	//网络I/O线程数
	int16_t numThreads = pt.get<int>("Gateway.numThreads", 10);
	//worker线程数
	int16_t numWorkerThreads = pt.get<int>("Gateway.numWorkerThreads", 10);
	//最大连接数
	int kMaxConnections = pt.get<int>("Gateway.kMaxConnections", 15000);
	//客户端连接超时时间(s)，心跳超时时间
	int kTimeoutSeconds = pt.get<int>("Gateway.kTimeoutSeconds", 3);
	//Worker线程单任务队列大小
	int kMaxQueueSize = pt.get<int>("Gateway.kMaxQueueSize", 1000);
	//管理员挂维护/恢复服务
	std::string strAdminList = pt.get<std::string>("Gateway.adminList", "192.168.2.93,");
	//证书路径
	std::string cert_path = pt.get<std::string>("Gateway.cert_path", "");
	//证书私钥
	std::string private_key = pt.get<std::string>("Gateway.private_key", "");
	//是否调试
	bool isdebug = pt.get<int>("Gateway.debug", 1);
	//主线程EventLoop，I/O监听/连接读写 accept(read)/connect(write)
	muduo::net::EventLoop loop;
	muduo::net::InetAddress listenAddr(strIpAddr, tcpPort);
	muduo::net::InetAddress listenAddrInn(strIpAddr, innPort);
	muduo::net::InetAddress listenAddrHttp(strIpAddr, httpPort);
	//server
	Gateway server(
		&loop,
		listenAddr, listenAddrInn, listenAddrHttp,
		cert_path, private_key);
	server.isdebug_ = isdebug;
	server.strIpAddr_ = strIpAddr;
	server.kMaxConnections_ = kMaxConnections;
	server.kTimeoutSeconds_ = kTimeoutSeconds;
	//管理员ip地址列表
	{
		std::vector<std::string> vec;
		boost::algorithm::split(vec, strAdminList, boost::is_any_of(","));
		for (std::vector<std::string>::const_iterator it = vec.begin();
			it != vec.end(); ++it) {
			std::string const& ipaddr = *it;
			if (!ipaddr.empty() &&
				boost::regex_match(ipaddr,
					boost::regex(
						"^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\." \
						"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
						"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
						"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$"))) {
				muduo::net::InetAddress addr(muduo::StringArg(ipaddr), 0, false);
				server.adminList_[addr.ipNetEndian()] = IpVisitE::kEnable;
				LOG_INFO << __FUNCTION__ << " --- *** " << "管理员IP[" << ipaddr << "]";
			}
		}
	}
	boost::algorithm::split(server.redlockVec_, strRedisLockIps, boost::is_any_of(","));
	if (
		server.initZookeeper(strZookeeperIps) &&
		server.initMongoDB(strMongoDBUrl) &&
		server.initRedisCluster(strRedisClusterIps, redisPasswd)) {
		//registerSignalHandler(SIGTERM, StopService);
		//registerSignalHandler(SIGINT, StopService);
		//网络I/O线程池，I/O收发读写 recv(read)/send(write)，worker线程池，处理游戏业务逻辑
		server.start(numThreads, numWorkerThreads, kMaxQueueSize);
		loop.loop();
	}
}
#endif