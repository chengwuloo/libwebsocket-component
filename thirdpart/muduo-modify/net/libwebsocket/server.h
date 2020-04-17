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

			void onMessage(
				const muduo::net::TcpConnectionPtr& conn,
				muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

			void onClosed(
				const muduo::net::TcpConnectionPtr& conn,
				muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

			//send 发送消息
			void send(const muduo::net::TcpConnectionPtr& conn, char const* data, size_t len);

			//send 发送消息
			void send(const muduo::net::TcpConnectionPtr& conn, uint8_t const* data, size_t len);

			//send 发送消息
			void send(const muduo::net::TcpConnectionPtr& conn, std::vector<uint8_t> const& data);

        }//namespace websocket
    }//namespace net
}//namespace muduo

#endif
