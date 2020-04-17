/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.03.2020                                           */
/************************************************************************/
#ifndef _MUDUO_NET_WEBSOCKET_ICALLBACK_H_
#define _MUDUO_NET_WEBSOCKET_ICALLBACK_H_

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
#include <memory>

#include <muduo/net/libwebsocket/base.h>
#include <muduo/net/libwebsocket/ITimestamp.h>
#include <muduo/net/libwebsocket/IBytesBuffer.h>

namespace muduo {
	namespace net {
		namespace websocket {

			//@@ ICallback
			class ICallback {
			public:
				//@overide
				virtual void send(const void* message, int len) = 0;
				virtual void sendMessage(std::string const& message) = 0;
				virtual void sendMessage(IBytesBuffer* message) = 0;
				//@overide
				virtual void shutdown() = 0;
				virtual void forceClose() = 0;
				virtual void forceCloseWithDelay(double seconds) = 0;
				//@overide
				virtual std::string peerIpAddrToString() const = 0;
				//@overide
				virtual void onConnectedCallback(std::string const& ipaddr) = 0;
				virtual void onMessageCallback(IBytesBuffer* buf, int msgType, ITimestamp* receiveTime) = 0;
				virtual void onClosedCallback(IBytesBuffer* buf, ITimestamp* receiveTime) = 0;
			};

			//@@
			typedef ICallback IHandler, ICallbackHandler;

		}//namespace websocket
	}//namespace net
}//namespace muduo

#endif