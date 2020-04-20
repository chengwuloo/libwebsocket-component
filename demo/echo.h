#ifndef MUDUO_EXAMPLES_SIMPLE_ECHO_ECHO_H
#define MUDUO_EXAMPLES_SIMPLE_ECHO_ECHO_H

#include "muduo/net/TcpServer.h"

// RFC 862
class EchoServer
{
 public:
  EchoServer(muduo::net::EventLoop* loop,
             const muduo::net::InetAddress& listenAddr,
	  std::string const& cert_path, std::string const& private_key_path,
	  std::string const& client_ca_cert_file_path = "",
	  std::string const& client_ca_cert_dir_path = "");
  
  ~EchoServer();

  //EventLoop one polling one thread
  void setThreadNum(int numThreads);

  void start();  // calls server_.start();

 private:
  void onConnection(const muduo::net::TcpConnectionPtr& conn);

  void onConnected(
      const muduo::net::TcpConnectionPtr& conn,
      std::string const& ipaddr);
  
  void onMessage(
	  const muduo::net::TcpConnectionPtr& conn,
	  muduo::net::Buffer* buf, int msgType,
	  muduo::Timestamp receiveTime);
  
private:
	//监听客户端TCP请求
	muduo::net::TcpServer server_;
};

#endif  // MUDUO_EXAMPLES_SIMPLE_ECHO_ECHO_H
