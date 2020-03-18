/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#ifndef MUDUO_LIBWEBSOCKET_SERVER_H
#define MUDUO_LIBWEBSOCKET_SERVER_H

#include <muduo/base/noncopyable.h>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Timestamp.h>

namespace muduo {
    namespace net {
        namespace websocket {

            //@@ Server
            class Server : muduo::noncopyable {
            public:
                Server(muduo::net::EventLoop* loop,
                    const muduo::net::InetAddress& listenAddr,
                    const std::string& name,
                    std::string const& cert_path, std::string const& private_key_path,
                    std::string const& client_ca_cert_file_path = "",
                    std::string const& client_ca_cert_dir_path = "");
                ~Server();
                muduo::net::EventLoop* getLoop() const { return server_.getLoop(); }
                //setConnectionCallback
                void setConnectionCallback(muduo::net::ConnectionCallback const& cb) {
                    server_.setConnectionCallback(cb);
                }
                //setWsConnectedCallback
                void setWsConnectedCallback(muduo::net::WsConnectedCallback const& cb) {
                    server_.setWsConnectedCallback(cb);
                }
                //setWsMessageCallback
                void setWsMessageCallback(muduo::net::WsMessageCallback const& cb) {
                    server_.setWsMessageCallback(cb);
                }
                //setThreadNum EventLoop one polling one thread
                void setThreadNum(int numThreads);
                //start
                void start(bool et = false);
                //sendData
                void sendData(const muduo::net::TcpConnectionPtr& conn, char const* data, size_t len);
                void sendData(const muduo::net::TcpConnectionPtr& conn, uint8_t const* data, size_t len);
                void sendData(const muduo::net::TcpConnectionPtr& conn, std::vector<uint8_t> const& data);
            private:
                //onMessage
                void onMessage(const muduo::net::TcpConnectionPtr& conn,
                    muduo::net::Buffer* buf,
                    muduo::Timestamp receiveTime);
                //onWebSocketClosed
                void onWebSocketClosed(
                    const muduo::net::TcpConnectionPtr& conn,
                    muduo::net::Buffer* buf, muduo::Timestamp receiveTime);
            public:
                //TcpServer
                muduo::net::TcpServer server_;
            };

        }//namespace websocket
    }//namespace net
}//namespace muduo

#endif
