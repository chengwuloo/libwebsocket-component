/************************************************************************/
/*    @author create by Yangzhi                                         */
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

#include <websocket/base.h>
#include <websocket/IBytesBuffer.h>
#include <websocket/ICallback.h>
#include <websocket/IHttpContext.h>
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

			//@@ IContext ��Ϊ TcpSession/TcpConnection ���ӻỰ���Ա����
			//@bref           TcpSession/TcpConnection ���ӻỰ�๹��� context_new() ����
			//@bref           TcpSession/TcpConnection ���ӻỰ������ʱ context_free() ����
			class IContext : public std::enable_shared_from_this<IContext> {
			public:
				friend IContextPtr context_new(
					WeakICallbackPtr handler,        //callback handler
					http::IContextPtr context,       //"http Context"
					IBytesBufferPtr dataBuffer,
					IBytesBufferPtr controlBuffer);
			protected:
				//setDataBuffer ��������֡��Ϣ��(body)
				//@return IBytesBufferPtr
				virtual void setDataBuffer(IBytesBufferPtr buf) = 0;

				//setControlBuffer ��������֡��Ϣ��(body)
				//@return IBytesBufferPtr
				virtual void setControlBuffer(IBytesBufferPtr buf) = 0;

				//setCallbackHandler ����ص��ӿ�
				//@param WeakICallbackPtr
				virtual void setCallbackHandler(WeakICallbackPtr handler) = 0;

				//getCallbackHandler ����ص��ӿ�
				//@return WeakICallbackPtr
				//virtual WeakICallbackPtr getCallbackHandler() = 0;

				//setHttpContext HTTP Context������
				//@param http::IContextPtr
				virtual void setHttpContext(http::IContextPtr context) = 0;

				//getHttpContext HTTP Context������
				//@return http::WeakIContextPtr
				virtual http::WeakIContextPtr getHttpContext() = 0;
			};

		}//namespace websocket
	}//namespace net
}//namespace muduo

#endif