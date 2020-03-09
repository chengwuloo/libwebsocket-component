#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>

#include <muduo/base/AsyncLogging.h>
#include <muduo/base/TimeZone.h>
#include <muduo/net/InetAddress.h>

#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>

#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <iostream>
#include <math.h>
#include <deque>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <utility>
#include <functional>
#include <chrono>
#include <random>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/timeb.h>
#include <fstream>
#include <ios>
#include <sys/wait.h>
#include <sys/resource.h>
#include "json/json.h"
#include "ApiServer.h"

int g_bisDebug = 0;
std::string g_gsIp;

int g_nConfigId = 0;

muduo::net::EventLoop *gloop = NULL;
using namespace std;
static int kRoolSize = 1024*1024*500;
int g_EastOfUtc = 60*60*8;
muduo::AsyncLogging* g_asyncLog = NULL;


int main(int argc, char* argv[])
{
    muduo::net::EventLoop loop;
    gloop = &loop;

    muduo::TimeZone beijing(g_EastOfUtc, "CST");
    muduo::Logger::setTimeZone(beijing);

    muduo::Logger::setLogLevel(muduo::Logger::DEBUG);
    muduo::Logger::setOutput(asyncOutput);

    if(!boost::filesystem::exists("./log/ApiServer/"))
    {
        boost::filesystem::create_directories("./log/ApiServer/");
    }
    muduo::AsyncLogging log(::basename("ApiServer"), kRoolSize);
    log.start();
    g_asyncLog = &log;
    if(!boost::filesystem::exists("./conf/game.conf")){
        LOG_INFO << "./conf/game.conf not exists";
        return 1;
    }
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/game.conf", pt);
    g_bisDebug = pt.get<int>("Global.isdebug", 0);
    int loglevel = pt.get<int>("Global.loglevel", 1);
    muduo::Logger::setLogLevel((muduo::Logger::LogLevel)loglevel);
	LOG_INFO << __FUNCTION__ << " --- *** " << "日志级别 = " << loglevel;
    //获取指定网卡ipaddr
    std::string netcardName = pt.get<std::string>("Global.netcardName","eth0");
	std::string strIpAddr;
	GlobalFunc::getIP(netcardName, strIpAddr);
	LOG_INFO << __FUNCTION__ << " --- *** " << "网卡名称 = " << netcardName;
    //HTTP监听相关
    std::string strHttpPort = "ApiServer.httpPort";
    std::string strHttpNumThreads = "ApiServer.httpNumThreads";
    std::string strHttpWorkerNumThreads = "ApiServer.httpWorkerNumThreads";
    //HTTP监听相关
	uint16_t httpPort = pt.get<int>(strHttpPort, 8120);
	int16_t httpNumThreads = pt.get<int>(strHttpNumThreads, 10);//网络IO线程数，IO收发
	int16_t httpWorkerNumThreads = pt.get<int>(strHttpWorkerNumThreads, 10);//业务逻辑处理线程数
    bool isdecrypt = pt.get<int>("ApiServer.isdecrypt", 0);              //0明文HTTP请求，1加密HTTP请求
    int whiteListControl = pt.get<int>("ApiServer.whiteListControl", 0);//白名单控制
    int kMaxConnections = pt.get<int>("ApiServer.kMaxConnections", 15000); //最大连接数
    int kTimeoutSeconds = pt.get<int>("ApiServer.kTimeoutSeconds", 3); //连接超时值(s)
    int kDeltaTime = pt.get<int>("ApiServer.kDeltaTime", 20);//间隔时间(s)打印一次
    int kMaxQueueSize = pt.get<int>("ApiServer.kMaxQueueSize", 1000);//Worker单任务队列大小
    std::string strAdminList = pt.get<std::string>("ApiServer.adminList", "192.168.2.93,");//管理员挂维护/恢复服务
    std::string cert_path = pt.get<std::string>("ApiServer.cert_path", "");//证书路径
    std::string private_key = pt.get<std::string>("ApiServer.private_key", "");//证书私钥
	int ttlUserLockSeconds = pt.get<int>("ApiServer.ttlUserLockSeconds", 1000);//上下分操作间隔时间(针对用户) ///
	int ttlAgentLockSeconds = pt.get<int>("ApiServer.ttlAgentLockSeconds", 1000);//上下分操作间隔时间(针对代理) ///
   
    muduo::net::InetAddress listenAddr(httpPort);
    ApiServer server(&loop, listenAddr, cert_path, private_key);
    server.strIpAddr_ = strIpAddr;
    server.isdecrypt_ = isdecrypt;
    server.whiteListControl_ = (eWhiteListCtrl)whiteListControl;
    server.kMaxConnections_ = kMaxConnections;
    server.kTimeoutSeconds_ = kTimeoutSeconds;
    server.ttlUserLockSeconds_ = ttlUserLockSeconds;
    server.ttlAgentLockSeconds_ = ttlAgentLockSeconds;
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
				server.admin_list_[addr.ipNetEndian()] = eApiVisit::Enable;
				LOG_INFO << __FUNCTION__ << " --- *** " << "管理员IP[" << ipaddr << "]";
			}
		}
	}
#ifdef _STAT_ORDER_QPS_
    server.deltaTime_ = kDeltaTime;
#endif
   
	//网络IO线程数，IO收发(recv/send) /////////
	//worker线程，处理游戏业务逻辑 //////////
    server.start(httpNumThreads, httpWorkerNumThreads, kMaxQueueSize);
    loop.loop();
    return 0;
}