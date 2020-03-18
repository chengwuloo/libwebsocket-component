/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.03.2020                                           */
/************************************************************************/
#ifndef _MUDUO_NET_WEBSOCKET_ICONTEXT_H_
#define _MUDUO_NET_WEBSOCKET_ICONTEXT_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>  // memset
#include <string>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>
#include <map>

#include <libwebsocket/base.h>
#include <libwebsocket/IBytesBuffer.h>
#include <libwebsocket/ICallback.h>
#include <libwebsocket/IHttpContext.h>
#include <memory>

namespace muduo {
	namespace net {
		namespace websocket {
			
			class IContext;
			//@@
			typedef websocket::IContext IWebsocketContext;
			//@@
			typedef std::shared_ptr<IWebsocketContext> IContextPtr;
			typedef std::weak_ptr<IWebsocketContext> WeakIContextPtr;
			//@@
			typedef IContextPtr IWebsocketContextPtr;
			typedef WeakIContextPtr WeakIWebsocketContextPtr;

			//@@ IContext 作为 TcpSession/TcpConnection 连接会话类成员变量
			//@bref           TcpSession/TcpConnection 连接会话类构造后 context_new() 创建
			//@bref           TcpSession/TcpConnection 连接会话类析构时 context_free() 销毁
			class IContext : public std::enable_shared_from_this<IContext> {
			public:
				friend IContextPtr context_new(
					WeakICallbackPtr handler,        //callback handler
					http::IContextPtr context,       //"http Context"
					IBytesBufferPtr dataBuffer,
					IBytesBufferPtr controlBuffer);
			protected:
				//setDataBuffer 完整数据帧消息体(body)
				//@return IBytesBufferPtr
				virtual void setDataBuffer(IBytesBufferPtr buf) = 0;

				//setControlBuffer 完整控制帧消息体(body)
				//@return IBytesBufferPtr
				virtual void setControlBuffer(IBytesBufferPtr buf) = 0;

				//setCallbackHandler 代理回调接口
				//@param WeakICallbackPtr
				virtual void setCallbackHandler(WeakICallbackPtr handler) = 0;

				//getCallbackHandler 代理回调接口
				//@return WeakICallbackPtr
				//virtual WeakICallbackPtr getCallbackHandler() = 0;

				//setHttpContext HTTP Context上下文
				//@param http::IContextPtr
				virtual void setHttpContext(http::IContextPtr context) = 0;

				//getHttpContext HTTP Context上下文
				//@return http::WeakIContextPtr
				virtual http::WeakIContextPtr getHttpContext() = 0;
			};

		}//namespace websocket
	}//namespace net
}//namespace muduo

#endif