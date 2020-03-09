
/* websocket ��׼Э��ͷ(RFC6455�淶)��ռ4�ֽ�(32bit)
*
  ��            �͸�             ��
  H             L H             L
 +---------------------------------------------------------------+
 |0                   1                   2                   3  |
 |0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|
 +-+-+-+-+-------+-+-------------+-------------------------------+
 |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 | |1|2|3|       |K|             |                               |
 +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 |     Extended payload length continued, if payload len == 127  |
 + - - - - - - - - - - - - - - - +-------------------------------+
 |                               |Masking-key, if MASK set to 1  |
 +-------------------------------+-------------------------------+
 | Masking-key (continued)       |          Payload Data         |
 +-------------------------------- - - - - - - - - - - - - - - - +
 :                     Payload Data continued ...                :
 + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 |                     Payload Data continued ...                |
 +---------------------------------------------------------------+

FIN                 1bit ֡������־λ(0/1)�������Ϊ1����ʶ��Ϣ�����һ֡(β֡/����֡)����һ֡(ͷ֡/��֡/��ʼ֡)Ҳ���������һ֡
RSV1/RSV2/RSV3      3bit ��������Ϊ0������Ԥ����Ĭ�϶�Ϊ0
opcode              4bit �����룬��ʶ��Ϣ֡���ͣ��������˵��
MASK                1bit �����־λ(0/1)���Ƿ�������ݣ������Ϊ1����ôMasking-Key�ֶδ��ڣ�C2S-1 S2C-0 DISCONN-1
Payload len         7bit,7+16bit,7+64bit �������ݳ���(��չ���ݳ��� + Ӧ�����ݳ���)��ȡֵ��Χ0~127(2^7-1)��
						 ���ֵΪ0~125����ô��ʾ�������ݳ��ȣ�
						 ���ֵΪ  126����ô����2�ֽ�(16bit)��ʾ�������ݳ��ȣ�
						 ���ֵΪ  127����ô����8�ֽ�(64bit)��ʾ�������ݳ��ȣ����λΪ0
Masking-key         0 or 4 bytes ���MASK��־λ��Ϊ1����ô���ֶδ��ڣ��������MASK��־λ��Ϊ0����ô���ֶ�ȱʧ
Payload data        (x + y) bytes �������� = ��չ���� + Ӧ������
						 ��MASK��־λΪ1ʱ��frame-payload-data = frame-masked-extension-data + frame-masked-application-data
						 ��MASK��־λΪ0ʱ��frame-payload-data = frame-unmasked-extension-data + frame-unmasked-application-data
						 frame-masked-extension-data 0x00-0xFF N*8(N>0) frame-masked-application-data 0x00-0xFF N*8(N>0)
						 frame-unmasked-extension-data 0x00-0xFF N*8(N>0) frame-unmasked-application-data 0x00-0xFF N*8(N>0)
Extension data      x bytes  ��չ���ݣ�����Э�̹���չ��������չ���ݳ���Ϊ0bytes
Application data    y bytes  Ӧ������

-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |Opcode  | Meaning                             | Reference |                                               |
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0x0   | Continuation Frame                  | RFC 6455  |  ��ʶһ��SEGMENT��Ƭ����֡                       |
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0x1   | Text Frame                          | RFC 6455  |  ��ʶһ��TEXT�����ı�֡
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0x2   | Binary Frame                        | RFC 6455  |  ��ʶһ��BINARY���Ͷ�����֡
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0x8   | Connection Close Frame              | RFC 6455  |  ��ʶһ��DISCONN���ӹر�֡
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0x9   | Ping Frame                          | RFC 6455  |  ��ʶһ��PING����״̬̽��֡keep-alive/heart-beats
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0xA   | Pong Frame                          | RFC 6455  |  ��ʶһ��PONG����״̬̽��֡keep-alive/heart-beats
-+--------+-------------------------------------+-----------|-----------------------------------------------+

һ��δ��Ƭ����Ϣ
	FIN = FrameFinished opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
һ����Ƭ����Ϣ
	FIN = FrameContinue opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
	FIN = FrameContinue opcode = SegmentMessage
	...
	...
	FIN = FrameFinished opcode = SegmentMessage (n >= 0)

���ģʽ(BigEndian) �͵�ַ���λ buf[4] = 0x12345678
---------------------------------------------------
buf[3] = 0x78  - �ߵ�ַ = ��λ
buf[2] = 0x56
buf[1] = 0x34
buf[0] = 0x12  - �͵�ַ = ��λ

С��ģʽ(LittleEndian) �͵�ַ���λ buf[4] = 0x12345678
---------------------------------------------------
buf[3] = 0x12  - �ߵ�ַ = ��λ
buf[2] = 0x34
buf[1] = 0x56
buf[0] = 0x78  - �͵�ַ = ��λ

*
*/

/************************************************************************/
/*    @author create by andy_ro@qq.com/Qwuloo@qq.com                    */
/*    @Date		   03.03.2020                                           */
/************************************************************************/
#ifndef _MUDUO_NET_WEBSOCKET_H_
#define _MUDUO_NET_WEBSOCKET_H_

#include <map>
#include <vector>
#include <utility>
#include <functional>
#include <memory>

#include <websocket/IBytesBuffer.h>
#include <websocket/ITimestamp.h>
#include <websocket/IContext.h>
//#include <ssl/ssl.h>

//websocketЭ�飬��ѭRFC6455�淶 ///
namespace muduo {
	namespace net {
        namespace websocket {
			
			enum MessageT {
				TyTextMessage = 0, //�ı���Ϣ
				TyBinaryMessage = 1, //��������Ϣ
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

			//context_free free websocket::IContextPtr
			//@param IContextPtr& "websocket context"
			void context_free(IContextPtr& context);

			//parse_message_frame
			//@param WeakIContextPtr const& "websocket context"
			/*extern*/ int parse_message_frame(
				WeakIContextPtr const& weakContext,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime);

			//pack_unmask_data_frame S2C
			/*extern*/ void pack_unmask_data_frame(
				IBytesBuffer* buf,
				char const* data, size_t len,
				MessageT messageType = MessageT::TyTextMessage, bool chunk = false);

			//pack_unmask_close_frame S2C
			/*extern*/ void pack_unmask_close_frame(
				IBytesBuffer* buf,
				char const* data, size_t len);

			//pack_unmask_ping_frame S2C
			/*extern*/ void pack_unmask_ping_frame(
				IBytesBuffer* buf,
				char const* data, size_t len);

			//pack_unmask_pong_frame S2C
			/*extern*/ void pack_unmask_pong_frame(
				IBytesBuffer* buf,
				char const* data, size_t len);

			void websocket_test_demo();

		}//namespace websocket
	}//namespace net
}//namespace muduo

#endif