
/* websocket 基础协议头(RFC6455规范) 2bytes/uint16_t/16bit */
/*
  高            低高             低
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

FIN                 1bit 帧结束标志位(0/1)，如果设为1，标识消息的最后一帧(尾帧/结束帧)，第一帧(头帧/首帧/起始帧)也可能是最后一帧
RSV1/RSV2/RSV3      3bit 必须设置为0，备用预留，默认都为0
opcode              4bit 操作码，标识消息帧类型，详见如下说明
MASK                1bit 掩码标志位(0/1)，是否加密数据，如果设为1，那么Masking-Key字段存在，C2S-1 S2C-0 DISCONN-1
Payload len         7bit,7+16bit,7+64bit 负载数据长度(扩展数据长度 + 应用数据长度)，取值范围0~127(2^7-1)，
						 如果值为0~125，那么表示负载数据长度，
						 如果值为  126，那么后续2字节(16bit)表示负载数据长度，
						 如果值为  127，那么后续8字节(64bit)表示负载数据长度，最高位为0
Masking-key         0 or 4 bytes 如果MASK标志位设为1，那么该字段存在，否则如果MASK标志位设为0，那么该字段缺失
Payload data        (x + y) bytes 负载数据 = 扩展数据 + 应用数据
						 当MASK标志位为1时，frame-payload-data = frame-masked-extension-data + frame-masked-application-data
						 当MASK标志位为0时，frame-payload-data = frame-unmasked-extension-data + frame-unmasked-application-data
						 frame-masked-extension-data 0x00-0xFF N*8(N>0) frame-masked-application-data 0x00-0xFF N*8(N>0)
						 frame-unmasked-extension-data 0x00-0xFF N*8(N>0) frame-unmasked-application-data 0x00-0xFF N*8(N>0)
Extension data      x bytes  扩展数据，除非协商过扩展，否则扩展数据长度为0bytes
Application data    y bytes  应用数据

-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |Opcode  | Meaning                             | Reference |                                               |
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0x0   | Continuation Frame                  | RFC 6455  |  标识一个SEGMENT分片连续帧                       |
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0x1   | Text Frame                          | RFC 6455  |  标识一个TEXT类型文本帧
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0x2   | Binary Frame                        | RFC 6455  |  标识一个BINARY类型二进制帧
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0x8   | Connection Close Frame              | RFC 6455  |  标识一个DISCONN连接关闭帧
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0x9   | Ping Frame                          | RFC 6455  |  标识一个PING网络状态探测帧keep-alive/heart-beats
-+--------+-------------------------------------+-----------|-----------------------------------------------+
 |  0xA   | Pong Frame                          | RFC 6455  |  标识一个PONG网络状态探测帧keep-alive/heart-beats
-+--------+-------------------------------------+-----------|-----------------------------------------------+

一个未分片的消息
	FIN = FrameFinished opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
一个分片的消息
	FIN = FrameContinue opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
	FIN = FrameContinue opcode = SegmentMessage
	...
	...
	FIN = FrameFinished opcode = SegmentMessage (n >= 0)

大端模式(BigEndian) 低地址存高位 buf[4] = 0x12345678
---------------------------------------------------
buf[3] = 0x78  - 高地址 = 低位
buf[2] = 0x56
buf[1] = 0x34
buf[0] = 0x12  - 低地址 = 高位

小端模式(LittleEndian) 低地址存低位 buf[4] = 0x12345678
---------------------------------------------------
buf[3] = 0x12  - 高地址 = 高位
buf[2] = 0x34
buf[1] = 0x56
buf[0] = 0x78  - 低地址 = 低位

*
*/

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

#include <libwebsocket/IBytesBuffer.h>
#include <libwebsocket/ITimestamp.h>
#include <libwebsocket/ICallback.h>
#include <libwebsocket/IHttpContext.h>
#include <libwebsocket/IContext.h>

//websocket协议，遵循RFC6455规范 ///
namespace muduo {
	namespace net {
        namespace websocket {
			
			enum MessageT {
				TyTextMessage = 0, //文本消息
				TyBinaryMessage = 1, //二进制消息
			};

			IContext* create(
				ICallback* handler,
				IHttpContextPtr& context,
				IBytesBufferPtr& dataBuffer,
				IBytesBufferPtr& controlBuffer);

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

#if 0
			struct header_t;
			struct extended_header_t;
			struct frame_header_t;
			struct message_header_t;
			class Message;
			class Context;

			//validate_uncontrol_frame_header
			/*static*/ void validate_uncontrol_frame_header(websocket::header_t& header);

			//validate_control_frame_header
			/*static*/ void validate_control_frame_header(websocket::header_t& header);

			//parse_frame_header，int16_t
			/*static*/ void parse_frame_header(websocket::header_t& header, IBytesBuffer* buf);

			//pack_unmask_uncontrol_frame_header
			/*static*/ websocket::header_t pack_unmask_uncontrol_frame_header(
				IBytesBuffer* buf,
				websocket::OpcodeE opcode, websocket::FinE fin, size_t Payloadlen);

			//pack_unmask_control_frame_header
			/*static*/ websocket::header_t pack_unmask_control_frame_header(
				IBytesBuffer* buf,
				websocket::OpcodeE opcode, websocket::FinE fin, size_t Payloadlen);

			//pack_unmask_uncontrol_frame S2C
			/*static*/ websocket::header_t pack_unmask_uncontrol_frame(
				IBytesBuffer* buf,
				char const* data, size_t len,
				websocket::OpcodeE opcode, websocket::FinE fin);

			//pack_unmask_control_frame
			/*static*/ websocket::header_t pack_unmask_control_frame(
				IBytesBuffer* buf,
				char const* data, size_t len,
				websocket::OpcodeE opcode, websocket::FinE fin);

			//FrameControlE MessageFrameE use MAKEWORD best
			/*static*/ std::pair<FrameControlE, MessageFrameE>
				get_frame_control_message_type(websocket::header_t const& header);

			//pack_unmask_data_frame_chunk S2C
			/*static*/ void pack_unmask_data_frame_chunk(
				IBytesBuffer* buf,
				char const* data, size_t len,
				websocket::MessageE messageType = MessageE::TextMessage,
				size_t chunksz = 1024);

			/*static*/ uint8_t get_frame_FIN(websocket::header_t const& header);
			/*static*/ rsv123_t get_frame_RSV123(websocket::header_t const& header);
			/*static*/ uint8_t get_frame_RSV1(websocket::header_t const& header);
			/*static*/ uint8_t get_frame_RSV2(websocket::header_t const& header);
			/*static*/ uint8_t get_frame_RSV3(websocket::header_t const& header);
			/*static*/ uint8_t get_frame_opcode(websocket::header_t const& header);
			/*static*/ uint8_t get_frame_MASK(websocket::header_t const& header);
			/*static*/ uint8_t get_frame_Payloadlen(websocket::header_t const& header);

			//dump websocket协议头信息
			/*static*/ void dump_header_info(websocket::header_t const& header);
			
			//dump websocket带扩展协议头信息
			/*static*/ void dump_extended_header_info(websocket::extended_header_t const& header);

			//validate_message_frame 消息帧有效性安全检查
			//@param websocket::Context& 组件内部私有接口
			/*static*/ bool validate_message_frame(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//update_frame_body_parse_step 更新帧体(body)消息流解析步骤step
			//	非控制帧(数据帧) frame body
			//		Maybe include Extended payload length
			//		Maybe include Masking-key
			//		Must include Payload data
			//	控制帧 frame body
			//		Maybe include Extended payload length(<=126)
			//		Maybe include Masking-key
			//		Maybe include Payload data
			//@param websocket::Context& 组件内部私有接口
			/*static*/ bool update_frame_body_parse_step(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//parse_frame_ReadFrameHeader
			//	解析基础协议头/帧头(header)
			//	输出基础协议头信息
			//	消息帧有效性安全检查
			//	更新帧体(body)消息流解析步骤step
			//@param websocket::Context& 组件内部私有接口
			/*static*/ bool parse_frame_ReadFrameHeader(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//parse_frame_ReadExtendedPayloadlenU16
			//@param websocket::Context& 组件内部私有接口
			/*static*/ int parse_frame_ReadExtendedPayloadlenU16(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//parse_frame_ReadExtendedPayloadlenI64
			//@param websocket::Context& 组件内部私有接口
			/*static*/ int parse_frame_ReadExtendedPayloadlenI64(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//parse_frame_ReadMaskingkey
			//@param websocket::Context& 组件内部私有接口
			/*static*/ int parse_frame_ReadMaskingkey(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//parse_uncontrol_frame_ReadPayloadData
			//@param websocket::Context& 组件内部私有接口
			/*static*/ bool parse_uncontrol_frame_ReadPayloadData(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//parse_uncontrol_frame_ReadExtendedPayloadDataU16
			//@param websocket::Context& 组件内部私有接口
			/*static*/ bool parse_uncontrol_frame_ReadExtendedPayloadDataU16(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//parse_uncontrol_frame_ReadExtendedPayloadDataI64
			//@param websocket::Context& 组件内部私有接口
			/*static*/ bool parse_uncontrol_frame_ReadExtendedPayloadDataI64(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//parse_control_frame_ReadPayloadData
			//@param websocket::Context& 组件内部私有接口
			/*static*/ bool parse_control_frame_ReadPayloadData(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//parse_control_frame_ReadExtendedPayloadDataU16
			//@param websocket::Context& 组件内部私有接口
			/*static*/ bool parse_control_frame_ReadExtendedPayloadDataU16(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//parse_control_frame_ReadExtendedPayloadDataI64
			//@param websocket::Context& 组件内部私有接口
			/*static*/ bool parse_control_frame_ReadExtendedPayloadDataI64(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//parse_frame
			//@param websocket::Context& 组件内部私有接口
			/*static*/ int parse_frame(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//create_websocket_response 填充websocket握手成功响应头信息
			/*static*/ void create_websocket_response(
				http::IRequest const* req, std::string& rsp);

			//do_handshake
			/*static*/ bool do_handshake(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* saveErrno);
#endif

		}//namespace websocket
	}//namespace net
}//namespace muduo

#endif