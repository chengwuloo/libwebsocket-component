/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#ifndef MUDUO_NET_WEBSOCKET_SERVER_H
#define MUDUO_NET_WEBSOCKET_SERVER_H

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
                
                //getLoop
                muduo::net::EventLoop* getLoop() const { return server_.getLoop(); }
                
                //setConnectionCallback TCP连接/关闭回调
                void setConnectionCallback(muduo::net::ConnectionCallback const& cb) {
                    server_.setConnectionCallback(cb);
                }
                
                //setWsConnectedCallback websocket握手成功回调
                void setWsConnectedCallback(muduo::net::WsConnectedCallback const& cb) {
                    server_.setWsConnectedCallback(cb);
                }
                
                //setWsMessageCallback websocket消息回调
                void setWsMessageCallback(muduo::net::WsMessageCallback const& cb) {
                    server_.setWsMessageCallback(cb);
                }
                
                //setThreadNum EventLoop one polling one thread
                void setThreadNum(int numThreads);
                
                //start
                void start(bool et = false);
                
                //send 发送消息
                static void send(const muduo::net::TcpConnectionPtr& conn, char const* data, size_t len);
                static void send(const muduo::net::TcpConnectionPtr& conn, uint8_t const* data, size_t len);
                static void send(const muduo::net::TcpConnectionPtr& conn, std::vector<uint8_t> const& data);
            
            private:
                //onMessage
                void onMessage(
                    const muduo::net::TcpConnectionPtr& conn,
                    muduo::net::Buffer* buf, muduo::Timestamp receiveTime);

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
