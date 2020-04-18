#include "demo/echo.h"
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <unistd.h>

#if 0
int main()
{
	std::string const cert_path = "./certificate/CA/private/cacert.pem";
	std::string const private_key = "./certificate/CA/private/cakey.pem";
	
	muduo::net::EventLoop loop;
	muduo::net::InetAddress listenAddr("192.168.2.93", 10000);
	EchoServer server(&loop, listenAddr, cert_path, private_key);
	server.setThreadNum(2);
	server.start();
	loop.loop();
}
#endif