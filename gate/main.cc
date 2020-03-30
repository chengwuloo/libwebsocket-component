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

//StopService ������ֹ
static void StopService(int signo) {
	if (gServer) {
		gServer->quit();
	}

	if (gMainEventLoop) {
		gMainEventLoop->quit();
	}
}

int main() {
	//��������
	if (!boost::filesystem::exists("./conf/game.conf")) {
		LOG_INFO << "./conf/game.conf not exists";
		return -1;
	}
	boost::property_tree::ptree pt;
	boost::property_tree::read_ini("./conf/game.conf", pt);

	//��־Ŀ¼ logdir/logname
	std::string logdir = pt.get<std::string>("Gateway.logdir", "./log/Gateway/");
	std::string logname = pt.get<std::string>("Gateway.logname", "Gateway");
	if (setEnv(logdir, logname) < 0) {
		return -1;
	}

	//��־����
	int loglevel = pt.get<int>("Gateway.loglevel", 1);
	muduo::Logger::setLogLevel((muduo::Logger::LogLevel)loglevel);
	LOG_INFO << __FUNCTION__ << " --- *** " << "��־���� = " << loglevel;

	//��ȡָ������ipaddr
	std::string strIpAddr;
	std::string netcardName = pt.get<std::string>("Global.netcardName", "eth0");
	IpByNetCardName(netcardName, strIpAddr);
	LOG_INFO << __FUNCTION__ << " --- *** " << "�������� = " << netcardName;

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

	//server_ tcp websocket
	int16_t tcpPort = pt.get<int>("Gateway.port", 8010);
	//innServer_ tcp
	int16_t innPort = pt.get<int>("Gateway.innPort", 9010);
	//httpServer_ tcp
	uint16_t httpPort = pt.get<int>("Gateway.httpPort", 8120);
	//����I/O�߳���
	int16_t numThreads = pt.get<int>("Gateway.numThreads", 10);
	//worker�߳���
	int16_t numWorkerThreads = pt.get<int>("Gateway.numWorkerThreads", 10);
	//���������
	int kMaxConnections = pt.get<int>("Gateway.kMaxConnections", 15000);
	//�ͻ������ӳ�ʱʱ��(s)��������ʱʱ��
	int kTimeoutSeconds = pt.get<int>("Gateway.kTimeoutSeconds", 3);
	//Worker�̵߳�������д�С
	int kMaxQueueSize = pt.get<int>("Gateway.kMaxQueueSize", 1000);
	//����Ա��ά��/�ָ�����
	std::string strAdminList = pt.get<std::string>("Gateway.adminList", "192.168.2.93,");
	//֤��·��
	std::string cert_path = pt.get<std::string>("Gateway.cert_path", "");
	//֤��˽Կ
	std::string private_key = pt.get<std::string>("Gateway.private_key", "");
	//�Ƿ����
	bool isdebug = pt.get<int>("Gateway.debug", 1);
	//////////////////////////////////////////////////////////////////////////
	//���߳�EventLoop��I/O����/���Ӷ�д accept(read)/connect(write)
	muduo::net::EventLoop loop;
	muduo::net::InetAddress listenAddr(tcpPort);
	muduo::net::InetAddress serverAddrInn(innPort);
	muduo::net::InetAddress listenAddrHttp(httpPort);
	//server
	Gateway server(
		&loop,
		listenAddr, serverAddrInn, listenAddrHttp,
		cert_path, private_key);
	server.isdebug_ = isdebug;
	server.strIpAddr_ = strIpAddr;
	server.kMaxConnections_ = kMaxConnections;
	server.kTimeoutSeconds_ = kTimeoutSeconds;
	//����Աip��ַ�б�
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
				LOG_INFO << __FUNCTION__ << " --- *** " << "����ԱIP[" << ipaddr << "]";
			}
		}
	}
	boost::algorithm::split(server.redlockVec_, strRedisLockIps, boost::is_any_of(","));
	if (
		server.initZookeeper(strZookeeperIps) &&
		server.initMongoDB(strMongoDBUrl) &&
		server.initRedisCluster(strRedisClusterIps, redisPasswd)) {
		registerSignalHandler(SIGTERM, StopService);
		registerSignalHandler(SIGINT, StopService);
		//////////////////////////////////////////////////////////////////////////
		//����I/O�̣߳�I/O�շ���д recv(read)/send(write)��worker�̣߳�������Ϸҵ���߼�
		server.start(numThreads, numWorkerThreads, kMaxQueueSize);
		loop.loop();
	}
}