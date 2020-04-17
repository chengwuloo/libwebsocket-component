/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#include <muduo/base/Logging.h>
#include <muduo/net/libwebsocket/websocket.h>
//#include <muduo/net/libwebsocket/ssl.h>
#include <muduo/net/libwebsocket/context.h>
#include <muduo/net/libwebsocket/server.h>
#include <assert.h>

namespace muduo {
	namespace net {
		namespace websocket {

			void send(const muduo::net::TcpConnectionPtr& conn, char const* data, size_t len) {
				//////////////////////////////////////////////////////////////////////////
				//pack_unmask_data_frame
				//////////////////////////////////////////////////////////////////////////
				muduo::net::Buffer rspdata;
				websocket::pack_unmask_data_frame(
					&rspdata, data, len,
					muduo::net::websocket::MessageT::TyBinaryMessage, false);
				conn->send(&rspdata);
			}

			void send(const muduo::net::TcpConnectionPtr& conn, uint8_t const* data, size_t len) {
				websocket::send(conn, (char const*)data, len);
			}

			void send(const muduo::net::TcpConnectionPtr& conn, std::vector<uint8_t> const& data) {
				websocket::send(conn, (char const*)&data[0], data.size());
			}

			void onMessage(
				const muduo::net::TcpConnectionPtr& conn,
				muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {
				assert(conn);
				conn->getLoop()->assertInLoopThread();

				muduo::net::WsContextPtr& wsContext = conn->getWsContext();
				assert(wsContext);
				//////////////////////////////////////////////////////////////////////////
				//parse_message_frame
				//////////////////////////////////////////////////////////////////////////
				wsContext->parse_message_frame(buf, &receiveTime);
			}

			void onClosed(
				const muduo::net::TcpConnectionPtr& conn,
				muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {
				assert(conn);
				conn->getLoop()->assertInLoopThread();

				LOG_WARN << "websocket::onClosed - " << conn->peerAddress().toIpPort();
				//////////////////////////////////////////////////////////////////////////
				//pack_unmask_close_frame
				//////////////////////////////////////////////////////////////////////////
				muduo::net::Buffer rspdata;
				websocket::pack_unmask_close_frame(
					&rspdata, buf->peek(), buf->readableBytes());
				conn->send(&rspdata);
			}

		}//namespace websocket
	}//namespace net
}//namespace muduo