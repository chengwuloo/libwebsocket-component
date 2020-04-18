/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.03.2020                                           */
/************************************************************************/
#ifndef _MUDUO_NET_WEBSOCKET_H_
#define _MUDUO_NET_WEBSOCKET_H_

#include <map>
#include <vector>
#include <utility>
#include <functional>
#include <memory>

#include <muduo/net/libwebsocket/IBytesBuffer.h>
#include <muduo/net/libwebsocket/ITimestamp.h>
#include <muduo/net/libwebsocket/ICallback.h>
#include <muduo/net/libwebsocket/IHttpContext.h>
#include <muduo/net/libwebsocket/IContext.h>

//websocket协议，遵循RFC6455规范 ///
namespace muduo {
	namespace net {
        namespace websocket {
			
			enum MessageT {
				TyTextMessage = 0, //文本消息
				TyBinaryMessage = 1, //二进制消息
			};

			//create
			IContext* create(
				ICallback* handler,
				IHttpContextPtr& context, //"http Context"
				IBytesBufferPtr& dataBuffer,
				IBytesBufferPtr& controlBuffer);
			
			//free
			void free(IContext* context);

			//parse_message_frame
			int parse_message_frame(
				IContext* context,
				IBytesBuffer* buf,
				ITimestamp* receiveTime);

			//pack_unmask_data_frame S2C
			void pack_unmask_data_frame(
				IBytesBuffer* buf,
				char const* data, size_t len,
				MessageT messageType = MessageT::TyTextMessage, bool chunk = false);

			//pack_unmask_close_frame S2C
			void pack_unmask_close_frame(
				IBytesBuffer* buf,
				char const* data, size_t len);

			//pack_unmask_ping_frame S2C
			void pack_unmask_ping_frame(
				IBytesBuffer* buf,
				char const* data, size_t len);

			//pack_unmask_pong_frame S2C
			void pack_unmask_pong_frame(
				IBytesBuffer* buf,
				char const* data, size_t len);

		}//namespace websocket
	}//namespace net
}//namespace muduo

#endif