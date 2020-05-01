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

			void onMessage(
				const muduo::net::TcpConnectionPtr& conn,
				muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {
				assert(conn);
				conn->getLoop()->assertInLoopThread();

				websocket::ContextPtr& context = conn->getWsContext();
				assert(context);
				//////////////////////////////////////////////////////////////////////////
				//parse_message_frame
				//////////////////////////////////////////////////////////////////////////
				context->parse_message_frame(buf, &receiveTime);
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
			
			//hook
			void hook(
				const WsConnectedCallback& ccb,
				const WsMessageCallback& mcb,
				const muduo::net::TcpConnectionPtr& conn) {
				conn->getLoop()->assertInLoopThread();
				//////////////////////////////////////////////////////////////////////////
				//websocket::Context::ctor
				//////////////////////////////////////////////////////////////////////////
				websocket::ContextPtr context(new muduo::net::websocket::Context(muduo::net::WeakTcpConnectionPtr(conn)));
				{
					//WsConnectedCallback
					context->setWsConnectedCallback(ccb);
					//WsClosedCallback
					context->setWsClosedCallback(
						std::bind(
							&muduo::net::websocket::onClosed,
							std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
					//WsMessageCallback
					context->setWsMessageCallback(mcb);
				}
				conn->setWsContext(context);
			}

			//reset
			void reset(const muduo::net::TcpConnectionPtr& conn) {
				conn->getLoop()->assertInLoopThread();
				//////////////////////////////////////////////////////////////////////////
				//websocket::Context::dtor
				//////////////////////////////////////////////////////////////////////////
				conn->getWsContext().reset();
			}

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
			
			//@@ Server ctor
			Server::Server(muduo::net::EventLoop* loop,
				const muduo::net::InetAddress& listenAddr,
				const std::string& name)
				: server_(loop, listenAddr, name) {
				//MessageCallback
				server_.setMessageCallback(
					std::bind(&muduo::net::websocket::onMessage,
						std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
#if 0
				//添加OpenSSL认证支持
				muduo::net::ssl::SSL_CTX_Init(
					cert_path,
					private_key_path,
					client_ca_cert_file_path, client_ca_cert_dir_path);
				//指定SSL_CTX
				server_.set_SSL_CTX(muduo::net::ssl::SSL_CTX_Get());
#endif
			}

			//@@ Server dtor
			Server::~Server() {
				//释放SSL_CTX
				muduo::net::ssl::SSL_CTX_free();
			}

			//setThreadNum
			void Server::setThreadNum(int numThreads) {
				server_.setThreadNum(numThreads);
			}

			//start
			void Server::start(bool et) {
				server_.start(et);
			}

			//hook
			void Server::hook(
				const WsConnectedCallback& ccb,
				const WsMessageCallback& mcb,
				const muduo::net::TcpConnectionPtr& conn) {

				muduo::net::websocket::hook(ccb, mcb, conn);
			}

			//reset
			void Server::reset(const muduo::net::TcpConnectionPtr& conn) {
				muduo::net::websocket::reset(conn);
			}

			void Server::send(const muduo::net::TcpConnectionPtr& conn, char const* data, size_t len) {
				muduo::net::websocket::send(conn, data, len);
			}

			void Server::send(const muduo::net::TcpConnectionPtr& conn, uint8_t const* data, size_t len) {
				send(conn, (char const*)data, len);
			}
			
			void Server::send(const muduo::net::TcpConnectionPtr& conn, std::vector<uint8_t> const& data) {
				send(conn, (char const*)&data[0], data.size());
			}

		}//namespace websocket
	}//namespace net
}//namespace muduo