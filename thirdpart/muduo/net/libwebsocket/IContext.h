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

#include <muduo/net/libwebsocket/base.h>
#include <muduo/net/libwebsocket/IBytesBuffer.h>
#include <muduo/net/libwebsocket/ICallback.h>
#include <muduo/net/libwebsocket/IHttpContext.h>
#include <memory>

namespace muduo {
	namespace net {
		namespace websocket {
			
			//@@ IContext
			class IContext {
			public:
				virtual void resetAll() = 0;
				virtual ~IContext() {}
			};

		}//namespace websocket
	}//namespace net
}//namespace muduo

#endif