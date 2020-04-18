/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#ifndef MUDUO_NET_WEBSOCKET_SERVER_H
#define MUDUO_NET_WEBSOCKET_SERVER_H

#include <muduo/base/noncopyable.h>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/libwebsocket/websocket.h>

namespace muduo {
    namespace net {
        namespace websocket {

			//hook 协议钩子
			void hook(
				const WsConnectedCallback& ccb,
				const WsMessageCallback& mcb,
				const muduo::net::TcpConnectionPtr& conn);

			//reset
			void reset(const muduo::net::TcpConnectionPtr& conn);

			void onMessage(
				const muduo::net::TcpConnectionPtr& conn,
				muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

			void onClosed(
				const muduo::net::TcpConnectionPtr& conn,
				muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

			//send 发送消息
			void send(const muduo::net::TcpConnectionPtr& conn, char const* data, size_t len);
			void send(const muduo::net::TcpConnectionPtr& conn, uint8_t const* data, size_t len);
			void send(const muduo::net::TcpConnectionPtr& conn, std::vector<uint8_t> const& data);

			//@@ Server
			class Server : muduo::noncopyable {
			public:
				Server(muduo::net::EventLoop* loop,
					const muduo::net::InetAddress& listenAddr,
					const std::string& name);
				~Server();

				//getLoop
				muduo::net::EventLoop* getLoop() const { return server_.getLoop(); }

				//setThreadNum
				void setThreadNum(int numThreads);

				//start
				void start(bool et = false);

				//hook 协议钩子
				static void hook(
					const WsConnectedCallback& ccb,
					const WsMessageCallback& mcb,
					const muduo::net::TcpConnectionPtr& conn);

				//reset
				static void reset(const muduo::net::TcpConnectionPtr& conn);

				//send 发送消息
				static void send(const muduo::net::TcpConnectionPtr& conn, char const* data, size_t len);
				static void send(const muduo::net::TcpConnectionPtr& conn, uint8_t const* data, size_t len);
				static void send(const muduo::net::TcpConnectionPtr& conn, std::vector<uint8_t> const& data);

			public:
				//TcpServer
				muduo::net::TcpServer server_;
			};

        }//namespace websocket
    }//namespace net
}//namespace muduo

#endif
