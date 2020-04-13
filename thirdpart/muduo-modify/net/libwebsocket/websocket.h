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
#include <muduo/net/libwebsocket/IContext.h>

//websocket协议，遵循RFC6455规范 ///
namespace muduo {
	namespace net {
        namespace websocket {
			
			enum MessageT {
				TyTextMessage = 0, //文本消息
				TyBinaryMessage = 1, //二进制消息
			};
			
			//
			// for example:
			//		TcpConnectionPtr conn(shared_from_this());
			//		websocket::IContextPtr websocket_ctx_ = websocket::context_new(
			//                  WeakTcpConnectionPtr(conn),
			//					http::IContextPtr(new HttpContext()),
			//					IBytesBufferPtr(new Buffer()),
			//					IBytesBufferPtr(new Buffer()));
			//
			//context_new create websocket::IContextPtr
			//@return IContextPtr "websocket context"
			IContextPtr context_new(
				WeakICallbackPtr handler,        //callback handler
				http::IContextPtr context,       //"http Context"
				IBytesBufferPtr dataBuffer,
				IBytesBufferPtr controlBuffer);

			//parse_message_frame
			//@param WeakIContextPtr const& "websocket context"
			int parse_message_frame(
				WeakIContextPtr const/*&*/ weakContext,
				IBytesBuffer /*const*/* buf,
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

			void websocket_test_demo();

		}//namespace websocket
	}//namespace net
}//namespace muduo

#endif