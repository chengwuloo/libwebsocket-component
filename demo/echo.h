#ifndef MUDUO_EXAMPLES_SIMPLE_ECHO_ECHO_H
#define MUDUO_EXAMPLES_SIMPLE_ECHO_ECHO_H

#include "muduo/net/TcpServer.h"

#include <libwebsocket/ssl.h>

// RFC 862
class EchoServer
{
 public:
  EchoServer(muduo::net::EventLoop* loop,
             const muduo::net::InetAddress& listenAddr,
	  std::string const& cert_path, std::string const& private_key_path,
	  std::string const& client_ca_cert_file_path = "",
	  std::string const& client_ca_cert_dir_path = "");
  
  //EventLoop one polling one thread
  void setThreadNum(int numThreads);

  void start();  // calls server_.start();

 private:
  void onConnection(const muduo::net::TcpConnectionPtr& conn);
  void onMessage(const muduo::net::TcpConnectionPtr& conn,
                 muduo::net::Buffer* buf,
                 muduo::Timestamp receiveTime);

  //onWebSocketConnected websocket
  void onWebSocketConnected(
      const muduo::net::TcpConnectionPtr& conn,
      std::string const& ipaddr);
  
  //onWebSocketMessage websocket
  void onWebSocketMessage(
	  const muduo::net::TcpConnectionPtr& conn,
	  muduo::net::Buffer* buf, int msgType,
	  muduo::Timestamp receiveTime);
  
  //onWebSocketClosed websocket
  void onWebSocketClosed(
      const muduo::net::TcpConnectionPtr& conn,
      muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

  //processRequest
  void processRequest(const muduo::net::TcpConnectionPtr& conn,
	  std::vector<uint8_t> &buf,
	  muduo::Timestamp receiveTime);

private:
    //监听客户端TCP请求
	muduo::net::TcpServer server_;
	//添加OpenSSL认证支持 ///
	muduo::net::ssl::SSL_CTX_Init ssl_ctx_init_;
};

#endif  // MUDUO_EXAMPLES_SIMPLE_ECHO_ECHO_H
