/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#include <muduo/base/Logging.h>
#include <libwebsocket/websocket.h>
#include <libwebsocket/ssl.h>
#include <libwebsocket/server.h>

namespace muduo {
	namespace net {
		namespace websocket {

			Server::Server(muduo::net::EventLoop* loop,
				const muduo::net::InetAddress& listenAddr,
				const std::string& name,
				std::string const& cert_path, std::string const& private_key_path,
				std::string const& client_ca_cert_file_path,
				std::string const& client_ca_cert_dir_path)
				: server_(loop, listenAddr, name) {

				server_.setMessageCallback(
					std::bind(&Server::onMessage, this,
						std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
				server_.enableWebsocket(true);
				server_.setWsClosedCallback(
					std::bind(
						&Server::onWebSocketClosed, this,
						std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
				//添加OpenSSL认证支持
				muduo::net::ssl::SSL_CTX_Init(
					cert_path,
					private_key_path,
					client_ca_cert_file_path, client_ca_cert_dir_path);
				//指定SSL_CTX
				server_.set_SSL_CTX(muduo::net::ssl::SSL_CTX_Get());
			}
			Server::~Server() {
				//释放SSL_CTX
				muduo::net::ssl::SSL_CTX_free();
			}
			//setThreadNum EventLoop one polling one thread
			void Server::setThreadNum(int numThreads) {
				server_.setThreadNum(numThreads);
			}
			//start
			void Server::start(bool et) {
				server_.start(et);
			}
			//onMessage
			void Server::onMessage(
				const muduo::net::TcpConnectionPtr& conn,
				muduo::net::Buffer* buf,
				muduo::Timestamp receiveTime) {

				//////////////////////////////////////////////////////////////////////////
				//parse_message_frame
				//////////////////////////////////////////////////////////////////////////
				muduo::net::websocket::parse_message_frame(
					conn->getWebsocketContext(),
					buf,
					&receiveTime);
			}
			//sendData
			void Server::sendData(const muduo::net::TcpConnectionPtr& conn, char const* data, size_t len) {
				//////////////////////////////////////////////////////////////////////////
				//pack_unmask_data_frame
				//////////////////////////////////////////////////////////////////////////
				muduo::net::Buffer rspdata;
				muduo::net::websocket::pack_unmask_data_frame(
					&rspdata,
					data, len,
					muduo::net::websocket::MessageT::TyBinaryMessage, false);
				conn->send(&rspdata);
			}
			//sendData
			void Server::sendData(const muduo::net::TcpConnectionPtr& conn, uint8_t const* data, size_t len) {
				sendData(conn, (char const*)data, len);
			}
			//sendData
			void Server::sendData(const muduo::net::TcpConnectionPtr& conn, std::vector<uint8_t> const& data) {
				sendData(conn, (char const*)&data[0], data.size());
			}
			//onWebSocketClosed
			void Server::onWebSocketClosed(
				const muduo::net::TcpConnectionPtr& conn,
				muduo::net::Buffer* buf, muduo::Timestamp receiveTime) {

				LOG_INFO << "Server - onWebSocketClosed - ";// << conn->peerAddress().toIpPort() << " -> "
					//<< conn->localAddress().toIpPort() << " is "
					//<< (conn->connected() ? "UP" : "DOWN");

				//////////////////////////////////////////////////////////////////////////
				//pack_unmask_close_frame
				//////////////////////////////////////////////////////////////////////////
				muduo::net::Buffer rspdata;
				muduo::net::websocket::pack_unmask_close_frame(
					&rspdata,
					buf->peek(), buf->readableBytes());
				conn->send(&rspdata);
			}

		}//namespace websocket
	}//namespace net
}//namespace muduo