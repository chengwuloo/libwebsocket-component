
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
/*    @author create by andy_ro@qq.com/Qwuloo@qq.com                    */
/*    @Date		   03.03.2020                                           */
/************************************************************************/

//websocket协议
//https://blog.csdn.net/sguniver_22/article/details/74273839
//https://www.cnblogs.com/hustskyking/p/websocket-with-node.html
//https://tools.ietf.org/html/rfc6455
//WebSocket 协议 RFC6455 文档
//https://blog.csdn.net/weixin_34388207/article/details/88581729
//http://www.rfc-editor.org/info/rfc6455
//wss测试
//http://www.websocket.org/echo.html
//http://tool.hibbba.com/websocket/
//https://www.toolfk.com/tool-online-runwebsocket

//1.websocket 基础协议头，占2字节(16bit) ///
//websocket::header_t* header_;

//2.Extended payload length ///
//header_.Payloadlen 0~125，Payloadlen表示负载数据长度
//header_.Payloadlen = 126 后续2字节(16bit)表示负载数据长度
//header_.Payloadlen = 127 后续8字节(64bit)表示负载数据长度，最高位为0
//uint8_t Extended_Payloadlen_[8];

//3.Masking-key 0 or 4 bytes ///
//如果MASK标志位设为1，那么该字段存在，否则，如果MASK标志位设为0，那么该字段缺失
//uint8_t Masking_key_[4];

//4.Payload Data (x + y) bytes 负载数据 = 扩展数据 + 应用数据 ///
//uint8_t Payload_data_[4096];

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>
#include <boost/random.hpp>
#include <boost/thread.hpp>
#include <boost/regex.hpp>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/proto/detail/ignore_unused.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time.hpp>
#include <boost/thread.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <libwebsocket/private/base64.h>
#include <libwebsocket/IHttpContext.h>
#include <libwebsocket/websocket.h>
#include <libwebsocket/private/Endian.h>

#include <memory>
#include <assert.h>
#include <functional>
#include <netinet/in.h>
#include <openssl/sha.h>

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

#define MY_DESC(n, s) { n, ""#n, s },

#define MY_ENUM0(n)		n,
#define MY_ENUM1(n, s)	n,
#define MY_ENUM2(n, s)	k##n,
#define MY_ENUM3(n)		k##n,

#define MY_ENUM_DECLARE(name, MY_ENUM_) \
	enum name##E {	\
		MY_MAP_##name(MY_ENUM_) \
	}; \

#define MY_DESC_TABLE_DECLARE(var, MY_MAP_) \
	static struct { \
		int id_; \
		char const *name_; \
		char const* desc_; \
	}var[] = { \
		MY_MAP_(MY_DESC) \
	}

#define MY_DESC_FETCH(id, var, name, desc) \
for (int i = 0; i < ARRAYSIZE(var); ++i) { \
	if (var[i].id_ == id) { \
		name = var[i].name_; \
		desc = var[i].desc_; \
		break; \
	}\
}

#define EXTERN_FUNCTON_DECLARE_TO_STRING(varname) \
extern std::string varname##_to_string(int varname);

#if 0
#define STATIC_FUNCTON_DECLARE_TO_STRING(varname) \
static std::string varname##_to_string(int varname);
#else
#define STATIC_FUNCTON_DECLARE_TO_STRING(varname) \
/*static*/ std::string varname##_to_string(int varname) { \
			std::string str##varname, desc; \
			MY_DESC_FETCH(varname, table_##varname##s_, str##varname, desc); \
			return str##varname; \
			}
#endif

#define FUNCTON_DECLARE_TO_STRING(varname) \
/*static*/ std::string varname##_to_string(int varname) { \
			std::string str##varname, desc; \
			MY_DESC_FETCH(varname, table_##varname##s_, str##varname, desc); \
			return str##varname; \
			}

//websocket握手 ///
#define MY_MAP_HANDSHAKE(XX) \
		XX(WS_ERROR_WANT_MORE, "握手需要读取") \
		XX(WS_ERROR_PARSE, "握手解析失败") \
		XX(WS_ERROR_PACKSZ, "握手包非法") \
		XX(WS_ERROR_CRLFCRLF, "握手查找CRLFCRLF") \
		XX(WS_ERROR_CONTEXT, "无效websocket::context")

#define FUNCTON_DECLARE_HANDSHAKE_TO_STRING(varname) \
			MY_DESC_TABLE_DECLARE(table_##varname##s_, MY_MAP_HANDSHAKE); \
			STATIC_FUNCTON_DECLARE_TO_STRING(varname);

//websocket连接状态 ///
#define MY_MAP_STATE(XX) \
		XX(StateE::kConnected, "") \
		XX(StateE::kClosed, "")

#define FUNCTON_DECLARE_STATE_TO_STRING(varname) \
			MY_DESC_TABLE_DECLARE(table_##varname##s_, MY_MAP_STATE); \
			STATIC_FUNCTON_DECLARE_TO_STRING(varname);

//大小端模式 ///
#define MY_MAP_ENDIANMODE(XX) \
		XX(LittleEndian, "小端:低地址存低位") \
		XX(BigEndian, "大端:低地址存高位")

#define FUNCTON_DECLARE_ENDIANMODE_TO_STRING(varname) \
			MY_DESC_TABLE_DECLARE(table_##varname##s_, MY_MAP_ENDIANMODE); \
			STATIC_FUNCTON_DECLARE_TO_STRING(varname);

//消息流解包步骤 ///
#define MY_MAP_STEP(XX) \
			XX(StepE::ReadFrameHeader, "") \
			XX(StepE::ReadExtendedPayloadlen2Byte, "") \
			XX(StepE::ReadExtendedPayloadlen8Byte, "") \
			XX(StepE::ReadMaskingkey, "") \
			XX(StepE::ReadPayloadData, "") \
			XX(StepE::ReadExtendedPayloadDataU16, "") \
			XX(StepE::ReadExtendedPayloadDataI64, "")

#define FUNCTON_DECLARE_STEP_TO_STRING(varname) \
			MY_DESC_TABLE_DECLARE(table_##varname##s_, MY_MAP_STEP); \
			STATIC_FUNCTON_DECLARE_TO_STRING(varname);

//FIN 帧结束标志位 ///
#define MY_MAP_FIN(XX) \
			XX(FrameContinue, "分片连续帧") \
			XX(FrameFinished, "消息结束帧")

#define FUNCTON_DECLARE_FIN_TO_STRING(varname) \
			MY_DESC_TABLE_DECLARE(table_##varname##s_, MY_MAP_FIN); \
			STATIC_FUNCTON_DECLARE_TO_STRING(varname);

//opcode 操作码，标识消息帧类型 ///
#define MY_MAP_OPCODE(XX) \
			XX(SegmentMessage, "消息片段") \
			XX(TextMessage,    "文本消息") \
			XX(BinaryMessage, "二进制消息") \
			XX(CloseMessage, "连接关闭消息") \
			XX(PingMessage, "心跳PING消息") \
			XX(PongMessage, "心跳PONG消息")

#define FUNCTON_DECLARE_OPCODE_TO_STRING(varname) \
			MY_DESC_TABLE_DECLARE(table_##varname##s_, MY_MAP_OPCODE); \
			STATIC_FUNCTON_DECLARE_TO_STRING(varname);

//帧控制类型 控制帧/非控制帧 ///
#define MY_MAP_FRAMECONTROL(XX) \
			XX(FrameInvalid, "无效帧") \
			XX(ControlFrame, "控制帧") \
			XX(UnControlFrame, "非控制帧")

#define FUNCTON_DECLARE_FRAMECONTROL_TO_STRING(varname) \
			MY_DESC_TABLE_DECLARE(table_##varname##s_, MY_MAP_FRAMECONTROL); \
			STATIC_FUNCTON_DECLARE_TO_STRING(varname);

//消息帧类型 ///
#define MY_MAP_MESSAGEFRAME(XX) \
			XX(UnknownFrame,"未知") \
			XX(HeadFrame, "头帧") \
			XX(MiddleFrame, "连续帧") \
			XX(TailFrame, "尾帧") \
			XX(HeadTailFrame, "头尾帧")

#define FUNCTON_DECLARE_MESSAGEFRAME_TO_STRING(varname) \
			MY_DESC_TABLE_DECLARE(table_##varname##s_, MY_MAP_MESSAGEFRAME); \
			STATIC_FUNCTON_DECLARE_TO_STRING(varname);

typedef uint8_t rsv123_t;

//websocket协议，遵循RFC6455规范 ///
namespace muduo {
	namespace net {
		namespace websocket {

			//typedef std::function<void(const TcpConnectionPtr&, std::string const&)> WsConnectedCallback;
			//typedef std::function<void(const TcpConnectionPtr&, Buffer*, int msgType, Timestamp receiveTime)> WsMessageCallback;
			//typedef std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp receiveTime)> WsClosedCallback;

			//握手错误码 ///
			enum HandShakeE {
				WS_ERROR_WANT_MORE = 0x66, //握手读取
				WS_ERROR_PARSE     = 0x77, //握手解析失败
				WS_ERROR_PACKSZ    = 0x88, //握手包非法
				WS_ERROR_CRLFCRLF  = 0x99, //握手查找CRLFCRLF
				WS_ERROR_CONTEXT   = 0x55, //无效websocket::Context
			};

			//连接状态 ///
			enum StateE {
				kConnected,
				kClosed,
			};

			//大小端模式 ///
			enum EndianModeE {
				LittleEndian = 0,
				BigEndian    = 1,
			};

			//消息流解包步骤 ///
			enum StepE {
				ReadFrameHeader,
				ReadExtendedPayloadlen2Byte,
				ReadExtendedPayloadlen8Byte,
				ReadMaskingkey,
				ReadPayloadData,
				ReadExtendedPayloadDataU16,
				ReadExtendedPayloadDataI64,
			};

			//帧结束标志位 ///
			enum FinE {
				FrameContinue, //帧未结束
				FrameFinished, //帧已结束(最后一个消息帧)
			};

			//操作码，标识消息帧类型 ///
			enum OpcodeE {
				//非控制帧，携带应用/扩展数据
				SegmentMessage = 0x0, //分片消息(片段)，连续帧/中间帧/分片帧
				TextMessage    = 0x1, //文本消息，头帧/首帧/起始帧
				BinaryMessage  = 0x2, //二进制消息，头帧/首帧/起始帧

				//非控制帧，作为扩展预留
				Reserved1 = 0x3,
				Reserved2 = 0x4,
				Reserved3 = 0x5,
				Reserved4 = 0x6,
				Reserved5 = 0x7,

				//控制帧，既是头帧/首帧/起始帧，也是尾帧/结束帧
				//可以被插入到消息片段中，如果非控制帧分片传输的话 ///
				//必须满足Payload len<=126字节，且不能被分片 ///

				//发送了连接关闭帧后禁止再[发送]任何数据帧
				//收到了连接关闭帧但之前没有发送连接关闭帧，则必须回应连接关闭帧
				CloseMessage = 0x8, //连接关闭帧，如果应用数据(body)有的话，表明连接关闭的reason

				//可能包含应用数据(body)
				PingMessage  = 0x9, //心跳PING探测帧

				//作为回应发送PONG帧必须包含PING帧传过来的应用数据(body)
				PongMessage  = 0xA, //心跳PONG探测帧，可以被主动发送(单向心跳)

				//控制帧，作为扩展预留
				Reserved6  = 0xB,
				Reserved7  = 0xC,
				Reserved8  = 0xD,
				Reserved9  = 0xE,
				Reserved10 = 0xF,
			};
			typedef OpcodeE MessageE;

			//帧控制类型 控制帧/非控制帧
			enum FrameControlE {
				FrameInvalid   = 0,
				ControlFrame   = 1,
				UnControlFrame = 2,
			};

			//消息帧类型
			enum MessageFrameE {
				UnknownFrame                   = 0x00,
				HeadFrame                      = 0x01, //头帧/首帧/起始帧
				MiddleFrame                    = 0x02, //连续帧/中间帧/分片帧
				TailFrame                      = 0x04, //尾帧/结束帧
				HeadTailFrame = HeadFrame | TailFrame, //未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
			};

			//格式化字符串 websocket握手
			FUNCTON_DECLARE_HANDSHAKE_TO_STRING(Handshake);

			//格式化字符串 websocket连接状态
			FUNCTON_DECLARE_STATE_TO_STRING(State);

			//格式化字符串 大小端模式
			FUNCTON_DECLARE_ENDIANMODE_TO_STRING(EndianMode);

			//格式化字符串 消息流解包步骤
			FUNCTON_DECLARE_STEP_TO_STRING(Step);

			//格式化字符串 Fin 帧结束标志位
			FUNCTON_DECLARE_FIN_TO_STRING(Fin);

			//格式化字符串 opcode 操作码，标识消息帧类型
			FUNCTON_DECLARE_OPCODE_TO_STRING(Opcode);

			//格式化字符串 帧控制类型 控制帧/非控制帧
			FUNCTON_DECLARE_FRAMECONTROL_TO_STRING(FrameControl);

			//格式化字符串 消息帧类型
			FUNCTON_DECLARE_MESSAGEFRAME_TO_STRING(MessageFrame);

#ifdef _NETTOHOST_BIGENDIAN_
			//BigEndian
			//header_t websocket
			//         L                       H
			//buf[0] = FIN|RSV1|RSV2|RSV3|opcode
			//         L              H
			//buf[1] = MASK|Payload len
			//
			//@@ header_t 基础协议头(RFC6455规范) uint16_t(16bit) ///
			struct header_t {
				uint16_t FIN        : 1; //1bit 帧结束标志位(0/1)
				uint16_t RSV1       : 1; //1bit RSV1/RSV2/RSV3 必须设置为0，备用预留，默认都为0
				uint16_t RSV2       : 1; //1bit RSV1/RSV2/RSV3 必须设置为0，备用预留，默认都为0
				uint16_t RSV3       : 1; //1bit RSV1/RSV2/RSV3 必须设置为0，备用预留，默认都为0
				uint16_t opcode     : 4; //4bit 操作码，标识消息帧类型
				uint16_t MASK       : 1; //1bit 掩码标志位(0/1)
				uint16_t Payloadlen : 7; //7bit 负载数据长度(扩展数据长度 + 应用数据长度)，取值范围0~127(2^7-1)
			};
#else		//LittleEndian
			//header_t websocket
			//         L                       H
			//buf[0] = opcode|RSV3|RSV2|RSV1|FIN
			//         L              H
			//buf[1] = Payload len|MASK
			//
			//@@ header_t 基础协议头(RFC6455规范) uint16_t(16bit) ///
			struct header_t {
				uint16_t opcode     : 4; //4bit 操作码，标识消息帧类型
				uint16_t RSV3       : 1; //1bit RSV1/RSV2/RSV3 必须设置为0，备用预留，默认都为0
				uint16_t RSV2       : 1; //1bit RSV1/RSV2/RSV3 必须设置为0，备用预留，默认都为0
				uint16_t RSV1       : 1; //1bit RSV1/RSV2/RSV3 必须设置为0，备用预留，默认都为0
				uint16_t FIN        : 1; //1bit 帧结束标志位(0/1)
				uint16_t Payloadlen : 7; //7bit 负载数据长度(扩展数据长度 + 应用数据长度)，取值范围0~127(2^7-1)
				uint16_t MASK       : 1; //1bit 掩码标志位(0/1)
			};
#endif
			//基础协议头大小
			static const size_t kHeaderLen = sizeof(header_t);
			static const size_t kMaskingkeyLen = 4;
			static const size_t kExtendedPayloadlen2Byte = sizeof(uint16_t);
			static const size_t kExtendedPayloadlen8Byte = sizeof(int64_t);

			//@@ extended_header_t 带扩展协议头
			struct extended_header_t {
			public:
				//set_header header_t，uint16_t
				inline void set_header(header_t const& header) {
#if 0
					memcpy(&this->header, &header, kHeaderLen);
#else
					this->header = header;
#endif
				}
				//get_header header_t，uint16_t
				inline header_t& get_header() {
					return header;
				}
				//get_header header_t，uint16_t
				inline header_t const& get_header() const {
					return header;
				}
				//reset_header header_t，uint16_t
				inline void reset_header() {
#if 0
					memset(&header, 0, kHeaderLen);
#else
					header = { 0 };
#endif
				}
			public:
				//getPayloadlen
				inline size_t getPayloadlen() const {
					return header.Payloadlen;
				}
				//setExtendedPayloadlen
				inline void setExtendedPayloadlen(uint16_t ExtendedPayloadlen) {
					this->ExtendedPayloadlen.u16 = ExtendedPayloadlen;
				}
				//setExtendedPayloadlen
				inline void setExtendedPayloadlen(int64_t ExtendedPayloadlen) {
					this->ExtendedPayloadlen.i64 = ExtendedPayloadlen;
				}
				//getExtendedPayloadlenU16
				inline uint16_t getExtendedPayloadlenU16() const {
					switch (header.Payloadlen) {
					case 126:
						//assert(ExtendedPayloadlen.u16 > 0);
						return ExtendedPayloadlen.u16;
					case 127:
						//assert(false);
						//assert(ExtendedPayloadlen.i64 > 0);
						return ExtendedPayloadlen.i64;
					default:
						//assert(false);
						//assert(
						//	ExtendedPayloadlen.u16 == 0 ||
						//	ExtendedPayloadlen.i64 == 0);
						return 0;
					}
				}
				//getExtendedPayloadlenI64
				inline int64_t getExtendedPayloadlenI64() const {
					switch (header.Payloadlen) {
					case 126:
						//assert(false);
						//assert(ExtendedPayloadlen.u16 > 0);
						return ExtendedPayloadlen.u16;
					case 127:
						//assert(ExtendedPayloadlen.i64 > 0);
						return ExtendedPayloadlen.i64;
					default:
						//assert(false);
						//assert(
						//	ExtendedPayloadlen.u16 == 0 ||
						//	ExtendedPayloadlen.i64 == 0);
						return 0;
					}
				}
				//getExtendedPayloadlen
				inline size_t getExtendedPayloadlen() const {
					switch (header.Payloadlen) {
					case 126:
						//assert(ExtendedPayloadlen.u16 > 0);
						return ExtendedPayloadlen.u16;
					case 127:
						//BUG ???
						//assert(ExtendedPayloadlen.i64 > 0);
						return ExtendedPayloadlen.i64;
					default:
						//assert(
						//	ExtendedPayloadlen.u16 == 0 ||
						//	ExtendedPayloadlen.i64 == 0);
						return 0;
					}
				}
				//existExtendedPayloadlen
				inline bool existExtendedPayloadlen() const {
					switch (header.Payloadlen) {
					case 126:
						//assert(ExtendedPayloadlen.u16 > 0);
						return true;
					case 127:
						//assert(ExtendedPayloadlen.i64 > 0);
						return true;
					default:
						//assert(
						//	ExtendedPayloadlen.u16 == 0 ||
						//	ExtendedPayloadlen.i64 == 0);
						return false;
					}
				}
				//getRealDatalen
				inline size_t getRealDatalen() const {
					switch (header.Payloadlen) {
					case 126:
						//assert(ExtendedPayloadlen.u16 > 0);
						return ExtendedPayloadlen.u16;
					case 127:
						//assert(ExtendedPayloadlen.i64 > 0);
						return ExtendedPayloadlen.i64;
					default:
						//assert(
						//	ExtendedPayloadlen.u16 == 0 ||
						//	ExtendedPayloadlen.i64 == 0);
						return header.Payloadlen;
					}
				}
			public:
				//setMaskingkey Masking-key
				inline void setMaskingkey(uint8_t const Masking_key[kMaskingkeyLen], size_t size) {
					assert(size == websocket::kMaskingkeyLen);
					uint8_t const Masking_key_[kMaskingkeyLen] = { 0 };
					assert(memcmp(
						this->Masking_key,
						Masking_key_, kMaskingkeyLen) == 0);
					memcpy(this->Masking_key, Masking_key, websocket::kMaskingkeyLen);
				}
				//getMaskingkey Masking-key
				inline uint8_t const* getMaskingkey() const {
					return Masking_key;
				}
				//existMaskingkey Masking-key
				inline bool existMaskingkey() const {
					switch (header.MASK)
					{
					case 1:
						//BUG???
						//assert(Masking_key[0] != '\0');
						return true;
					case 0:
						assert(Masking_key[0] == '\0');
						return false;
					default:
						assert(false);
						return false;
					}
				}
				//reset extended_header_t
				inline void reset() {
					reset_header();
					static const size_t kExtendedPayloadlenUnion = sizeof ExtendedPayloadlen;
					assert(kExtendedPayloadlenUnion == kExtendedPayloadlen8Byte);
					memset(&ExtendedPayloadlen, 0, kExtendedPayloadlen8Byte);
					memset(Masking_key, 0, kMaskingkeyLen);
				}
			public:
				//@ header_t，uint16_t 基础协议头
				header_t header;

				//@ Masking-key 字节对齐关系，在ExtendedPayloadlen之前定义占用空间更小
				uint8_t Masking_key[kMaskingkeyLen];

				//@ Extended payload length
				union {
					uint16_t u16;
					int64_t i64;
				}ExtendedPayloadlen;
				
				//@ Payload Data
				//uint8_t data[0];
			};
			
			//带扩展协议头大小
			static const size_t kExtendedHeaderLen = sizeof(extended_header_t);

			//@@ frame_header_t 分片/未分片每帧头
			struct frame_header_t {
			public:
				//set_header header_t，uint16_t
				inline void set_header(header_t const& header) {
					this->header.set_header(header);
				}
				//get_header header_t，uint16_t
				inline header_t& get_header() {
					return header.get_header();
				}
				//get_header header_t，uint16_t
				inline header_t const& get_header() const {
					return header.get_header();
				}
				//getPayloadlen
				inline size_t getPayloadlen() const {
					return header.getPayloadlen();
				}
				//setExtendedPayloadlen
				inline void setExtendedPayloadlen(uint16_t ExtendedPayloadlen) {
					header.setExtendedPayloadlen(ExtendedPayloadlen);
				}
				//setExtendedPayloadlen
				inline void setExtendedPayloadlen(int64_t ExtendedPayloadlen) {
					header.setExtendedPayloadlen(ExtendedPayloadlen);
				}
				//getExtendedPayloadlenU16
				inline uint16_t getExtendedPayloadlenU16() const {
					return header.getExtendedPayloadlenU16();
				}
				//getExtendedPayloadlenI64
				inline int64_t getExtendedPayloadlenI64() const {
					return header.getExtendedPayloadlenI64();
				}
				//getExtendedPayloadlen
				inline size_t getExtendedPayloadlen() const {
					return header.getExtendedPayloadlen();
				}
				//existExtendedPayloadlen
				inline bool existExtendedPayloadlen() const {
					return header.existExtendedPayloadlen();
				}
				//getRealDatalen
				inline size_t getRealDatalen() const {
					return header.getRealDatalen();
				}
				//setMaskingkey Masking-key
				inline void setMaskingkey(uint8_t const Masking_key[kMaskingkeyLen], size_t size) {
					header.setMaskingkey(Masking_key, size);
				}
				//getMaskingkey Masking-key
				inline uint8_t const* getMaskingkey() const {
					return header.getMaskingkey();
				}
				//existMaskingkey Masking-key
				inline bool existMaskingkey() const {
					return header.existMaskingkey();
				}
				//resetExtendedHeader extended_header_t
				inline void resetExtendedHeader() {
#if 0
					memset(&header, 0, kExtendedHeaderLen);
#elif 1
					header = { 0 };
#else
					header.reset();
#endif
				}
			public:
				//setFrameControlType 帧控制类型 控制帧/非控制帧
				inline void setFrameControlType(FrameControlE frameControlType) {
					this->frameControlType = frameControlType;
				}
				//getFrameControlType
				inline FrameControlE getFrameControlType() const {
					return frameControlType;
				}
				//setMessageFrameType 消息帧类型
				//头帧/首帧/起始帧
				//连续帧/中间帧/分片帧
				//尾帧/结束帧
				//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
				inline void setMessageFrameType(MessageFrameE messageFrameType) {
					this->messageFrameType = messageFrameType;
				}
				//getMessageFrameType
				inline MessageFrameE getMessageFrameType() const {
					return messageFrameType;
				}
			public:
				//testValidate 测试帧头合法性
				inline void testValidate();

			public:
				//带扩展协议头
				extended_header_t header;
				//帧控制类型 控制帧/非控制帧
				FrameControlE frameControlType;
				//消息帧类型
				MessageFrameE messageFrameType;
			};

			//@@ message_header_t 完整websocket消息头(header)
			struct message_header_t {
			public:
				message_header_t(MessageE messageType)
					: messageType_(messageType) {
				}
				~message_header_t() {
					reset();
				}
			public:
				//setMessageType 指定消息类型
				inline void setMessageType(MessageE messageType) {
					messageType_ = messageType;
				}
				//getMessageType 返回消息类型
				inline MessageE getMessageType() const {
					return messageType_;
				}
				//reset
				inline void reset() {
					frameHeaders_.clear();
				}

				//getFrameHeaderSize 返回帧头数
				inline size_t getFrameHeaderSize() const {
					return frameHeaders_.size();
				}

				//getFrameHeader 返回第i个帧头
				//@param i 0-返回首帧 否则返回中间帧或尾帧
				inline frame_header_t const* getFrameHeader(size_t i) const {
					int size = frameHeaders_.size();
					if (size > 0) {
						return i < size ?
							&frameHeaders_[i] :
							&frameHeaders_[size - 1];
					}
					return NULL;
				}
				//getFirstFrameHeader 返回首个帧头
				inline frame_header_t* getFirstFrameHeader() {
					int size = frameHeaders_.size();
					if (size > 0) {
						return &frameHeaders_[0];
					}
					return NULL;
				}
				//getFirstFrameHeader 返回首个帧头
				inline frame_header_t const* getFirstFrameHeader() const {
					int size = frameHeaders_.size();
					if (size > 0) {
						return &frameHeaders_[0];
					}
					return NULL;
				}
				//getLastFrameHeader 返回最新帧头
				inline frame_header_t* getLastFrameHeader() {
					int size = frameHeaders_.size();
					if (size > 0) {
						return &frameHeaders_[size - 1];
					}
					return NULL;
				}
				//getLastFrameHeader 返回最新帧头
				inline frame_header_t const* getLastFrameHeader() const {
					int size = frameHeaders_.size();
					if (size > 0) {
						return &frameHeaders_[size - 1];
					}
					return NULL;
				}
				//addFrameHeader 添加单个帧头
				inline MessageFrameE addFrameHeader(websocket::extended_header_t& header);

				//dumpFrameHeaders 打印全部帧头信息
				inline void dumpFrameHeaders(std::string const& name) {
					int i = 0;
					printf("\n--- *** %s\n", name.c_str());
					printf("+-----------------------------------------------------------------------------------------------------------------+\n");
					for (std::vector<frame_header_t>::const_iterator it = frameHeaders_.begin();
						it != frameHeaders_.end(); ++it) {
#if 0
						if (it->existExtendedPayloadlen()) {
							if (it->existMaskingkey()) {
								//Extended payload length & Masking-key
							}
							else {
								//Extended payload length
							}
						}
						else {
							if (it->existMaskingkey()) {
								//Masking-key
							}
							else {
							}
						}
#endif
						printf("Frame[%d][%s][%s] FIN = [%s] opcode = [%s] MASK[%d][%d][%d] {%s}\n",
							i++,
							FrameControl_to_string(it->frameControlType).c_str(),
							Opcode_to_string(messageType_).c_str(),
							Fin_to_string(it->get_header().FIN).c_str(),
							Opcode_to_string(it->get_header().opcode).c_str(),
							it->get_header().MASK,
							it->getPayloadlen(), it->getExtendedPayloadlen(),
							MessageFrame_to_string(it->messageFrameType).c_str());
					}
					printf("+-----------------------------------------------------------------------------------------------------------------+\n\n");
				}
			private:
				//消息类型 TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
				MessageE messageType_;
				//消息全部帧头
				std::vector<frame_header_t> frameHeaders_;
			};

			//@@ Message
			class Message : public base::noncopyable {
			public:
				explicit Message(MessageE messageType)
					: messageHeader_(messageType)
					//, messageBuffer_(NULL)
					/*, header_({ 0 })*/
					, segmentOffset_(0) {
				}
				explicit Message(MessageE messageType,
					IBytesBuffer* messageBuffer)
					: messageHeader_(messageType)
					, messageBuffer_(messageBuffer)
					/*, header_({ 0 })*/
					, unMask_c_(0)
					, segmentOffset_(0) {
					//assert must not be NULL
					assert(messageBuffer_);
				}
				~Message() {
					messageHeader_.reset();
					if (messageBuffer_) {
						messageBuffer_->retrieveAll();
						segmentOffset_ = 0;
						unMask_c_ = 0;
						messageBuffer_.reset();
					}
				}
				//getMessageHeader 完整websocket消息头(header)
				//@return message_header_t&
				inline message_header_t& getMessageHeader() {
					return messageHeader_;
				}
				//setMessageBuffer 完整websocket消息体(body)
				//@return IBytesBufferPtr
				inline void setMessageBuffer(IBytesBufferPtr buf) {
					//assert must be NULL
					assert(!messageBuffer_);
					messageBuffer_ = buf;
					//assert must not be NULL
					assert(messageBuffer_);
				}
				//getMessageBuffer 完整websocket消息体(body)
				//@return IBytesBufferPtr
				inline IBytesBufferPtr getMessageBuffer() {
					//assert must not be NULL
					//@@@ Fixed BUG Crash ///
					//assert(messageBuffer_);
					return messageBuffer_;
				}
				//setMessageType 指定消息类型
				inline void setMessageType(MessageE messageType) {
					messageHeader_.setMessageType(messageType);
				}
				//getMessageType 返回消息类型
				inline MessageE getMessageType() const {
					return messageHeader_.getMessageType();
				}

				//getFrameHeaderSize 返回帧头数
				inline size_t getFrameHeaderSize() const {
					return messageHeader_.getFrameHeaderSize();
				}
				//getFrameHeader 返回第i个帧头
				//@param i 0-返回首帧 否则返回中间帧或尾帧
				inline frame_header_t const* getFrameHeader(size_t i) const {
					return messageHeader_.getFrameHeader(i);
				}
				//getFirstFrameHeader 返回首个帧头
				inline frame_header_t* getFirstFrameHeader() {
					return messageHeader_.getFirstFrameHeader();
				}
				//getFirstFrameHeader 返回首个帧头
				inline frame_header_t const* getFirstFrameHeader() const {
					return messageHeader_.getFirstFrameHeader();
				}
				//getLastFrameHeader 返回最新帧头
				inline frame_header_t* getLastFrameHeader() {
					return messageHeader_.getLastFrameHeader();
				}
				//getLastFrameHeader 返回最新帧头
				inline frame_header_t const* getLastFrameHeader() const {
					return messageHeader_.getLastFrameHeader();
				}
				//addFrameHeader 添加单个帧头
				inline MessageFrameE addFrameHeader(websocket::extended_header_t& header) {
					return messageHeader_.addFrameHeader(header);
				}
				//resetMessage 重置消息
				inline void resetMessage() {
					//重置正处于解析当中的帧头(当前帧头)
					//memset(&header_, 0, kHeaderLen);
					//重置消息头(header)/消息体(body)
					messageHeader_.reset();
					//@@@ Fixed BUG Crash ///
					//assert must not be NULL
					//assert(messageBuffer_);
					if (messageBuffer_) {
						messageBuffer_->retrieveAll();
						segmentOffset_ = 0;
						unMask_c_ = 0;
						//断开连接析构时候调用释放 ///
						//messageBuffer_.reset();
					}
				}
				//dumpFrameHeaders 打印全部帧头信息
				inline void dumpFrameHeaders(std::string const& name) {
					messageHeader_.dumpFrameHeaders(name);
				}
				//unMaskPayloadData Payload Data做UnMask计算
				//@param unMask bool
				//@param Masking_key uint8_t const*
				inline bool unMaskPayloadData(bool unMask, IBytesBuffer* buf, uint8_t const Masking_key[kMaskingkeyLen]) {
					if (unMask &&
						buf->readableBytes() > 0 && buf->readableBytes() > segmentOffset_) {
						for (ssize_t i = segmentOffset_; i < buf->readableBytes(); ++i) {
							*((char*)buf->peek() + i) = *(buf->peek() + i) ^ *(Masking_key + i % websocket::kMaskingkeyLen);
						}
						if (unMask_c_++ == 0) {
							assert(segmentOffset_ == 0);
						}
						segmentOffset_ = buf->readableBytes();
						return true;
					}
					return false;
				}
			private:
				//完整websocket消息头(header)
				message_header_t messageHeader_;
				//完整websocket消息体(body)，存放接收数据解析之后的buffer
				IBytesBufferPtr messageBuffer_;
				//消息分片数据UnMask Masking-key操作段偏移，UnMask操作次数
				size_t segmentOffset_, unMask_c_;
			};

			//@@ Context
			class Context : public base::noncopyable, public IContext {
			public:
				//parse_message_frame
				//@param websocket::IContext* 暴露给外部接口
				//friend int parse_message_frame(
				//	websocket::IContext* context,
				//	IBytesBuffer /*const*/* buf,
				//	ITimestamp* receiveTime);

				//do_handshake
				//@param websocket::Context& 组件内部私有接口
				//friend bool do_handshake(
				//	websocket::Context& context,
				//	IBytesBuffer /*const*/* buf,
				//	ITimestamp* receiveTime, int* saveErrno);

				//parse_frame
				//@param websocket::Context& 组件内部私有接口
				//friend int parse_frame(
				//	websocket::Context& context,
				//	IBytesBuffer /*const*/* buf,
				//	ITimestamp* receiveTime, int* savedErrno);

				//parse_uncontrol_frame_body 非控制帧(数据帧) frame body
				//Maybe include Extended payload length
				//Maybe include Masking-key
				//Must include Payload data
				//@param websocket::Context& 组件内部私有接口
				//friend bool parse_uncontrol_frame_body(
				//	websocket::Context& context,
				//	IBytesBuffer /*const*/* buf,
				//	ITimestamp* receiveTime, int* savedErrno);

				//parse_control_frame_body 控制帧 frame body
				//Maybe include Extended payload length(<=126)
				//Maybe include Masking-key
				//Maybe include Payload data
				//@param websocket::Context& 组件内部私有接口
				//friend bool parse_control_frame_body(
				//	websocket::Context& context,
				//	IBytesBuffer /*const*/* buf,
				//	ITimestamp* receiveTime, int* savedErrno);
			public:
				explicit Context(EndianModeE endian = EndianModeE::LittleEndian)
					: step_(StepE::ReadFrameHeader)
					, state_(StateE::kClosed)
					, header_({ 0 })
					, endian_(endian)
					, dataMessage_(OpcodeE::TextMessage)
					, controlMessage_(OpcodeE::CloseMessage)
					//, callbackHandler_(NULL)
					/*, httpContext_(NULL)*/ {
				}
				explicit Context(
					IBytesBuffer* dataBuffer,
					IBytesBuffer* controlBuffer,
					EndianModeE endian = EndianModeE::LittleEndian)
					: step_(StepE::ReadFrameHeader)
					, state_(StateE::kClosed)
					, header_({ 0 })
					, endian_(endian)
					, dataMessage_(OpcodeE::TextMessage, dataBuffer)
					, controlMessage_(OpcodeE::CloseMessage, controlBuffer)
					//, callbackHandler_(NULL)
					/*, httpContext_(NULL)*/ {
				}
				~Context() {
					//printf("--- *** websocket::Context::dtor ...\n");
					resetAll();
					step_ = StepE::ReadFrameHeader;
					state_ = StateE::kClosed;
				}

				//setDataBuffer 完整数据帧消息体(body)
				//@return IBytesBufferPtr
				inline void setDataBuffer(IBytesBufferPtr buf) {
					dataMessage_.setMessageBuffer(buf);
				}

				//setControlBuffer 完整控制帧消息体(body)
				//@return IBytesBufferPtr
				inline void setControlBuffer(IBytesBufferPtr buf) {
					controlMessage_.setMessageBuffer(buf);
				}

				//setCallbackHandler 代理回调接口
				//@param WeakICallbackPtr
				inline void setCallbackHandler(WeakICallbackPtr handler) {
					callbackHandler_ = handler;
					//assert(callbackHandler_);
				}

				//getCallbackHandler 代理回调接口
				//@return WeakICallbackPtr
				inline WeakICallbackPtr getCallbackHandler() {
					//assert(callbackHandler_);
					return callbackHandler_;
				}

				//setHttpContext HTTP Context上下文
				//@param http::IContextPtr
				inline void setHttpContext(http::IContextPtr context) {
					httpContext_ = context;
					assert(httpContext_);
				}

				//getHttpContext HTTP Context上下文
				//@return http::WeakIContextPtr
				inline http::WeakIContextPtr getHttpContext() {
					assert(httpContext_);
					return httpContext_;
				}

				//getExtendedHeader 正处于解析当中的帧头(当前帧头)
				//@return websocket::extended_header_t&
				inline websocket::extended_header_t& getExtendedHeader() {
					return header_;
				}

				//getExtendedHeader 正处于解析当中的帧头(当前帧头)
				//@return websocket::extended_header_t const&
				inline websocket::extended_header_t const& getExtendedHeader() const {
					return header_;
				}

				//getDataMessage 数据帧(非控制帧)，携带应用/扩展数据
				//@return Message&
				inline Message& getDataMessage() {
					return dataMessage_;
				}

				//getControlMessage 控制帧，Payload len<=126字节，且不能被分片
				//@return Message&
				inline Message& getControlMessage() {
					return controlMessage_;
				}

				//resetExtendedHeader extended_header_t
				inline void resetExtendedHeader() {
#if 0
					//重置正处于解析当中的帧头(当前帧头)
					memset(&header_, 0, kExtendedHeaderLen);
#elif 1
					header_ = { 0 };
#else
					header_.reset();
#endif
				}

				//resetDataMessage
				inline void resetDataMessage() {
					dataMessage_.resetMessage();
				}

				//resetControlMessage
				inline void resetControlMessage() {
					controlMessage_.resetMessage();
				}

				//resetAll
				inline void resetAll() {
					resetExtendedHeader();
					dataMessage_.resetMessage();
					controlMessage_.resetMessage();
					//需要握手成功前释放httpContext资源 ///
					if (httpContext_) {
						httpContext_.reset();
					}
				}

				//StateE
				inline void setWebsocketState(StateE state) {
					state_ = state;
				}
				inline StateE getWebsocketState() const {
					return state_;
				}

				//StepE
				inline void setWebsocketStep(StepE step) {
					step_ = step;
				}
				inline StepE getWebsocketStep() const {
					return step_;
				}
			private:
				//解析步骤
				StepE step_;
				//连接状态
				StateE state_;
				//大小端模式
				EndianModeE endian_;
				//正处于解析当中的帧头(当前帧头)，uint16_t
				websocket::extended_header_t header_;
				//数据帧(非控制帧)，携带应用/扩展数据
				websocket::Message dataMessage_;
				//控制帧，既是头帧/首帧/起始帧，也是尾帧/结束帧
				//可以被插入到消息片段中，如果非控制帧分片传输的话 ///
				//必须满足Payload len<=126字节，且不能被分片 ///
				websocket::Message controlMessage_;
				//IContextPtr HTTP handshake for websocket
				http::IContextPtr httpContext_;
				//WeakICallbackPtr for callback
				websocket::WeakICallbackPtr callbackHandler_;
			};

			//
			// for example:
			//		TcpConnectionPtr conn(shared_from_this());
			//		websocket::IContextPtr websocket_ctx_ = context_new(
			//                  WeakTcpConnectionPtr(conn),
			//					http::IContextPtr(new HttpContext()),
			//					IBytesBufferPtr(new Buffer()),
			//					IBytesBufferPtr(new Buffer()));
			//
			//context_new create websocket::IContextPtr
			//@return IContextPtr "websocket context"
			/*extern*/ /*"C"*/ IContextPtr context_new(
				WeakICallbackPtr handler,      //callback handler
				http::IContextPtr context,     //"http Context"
				IBytesBufferPtr dataBuffer,
				IBytesBufferPtr controlBuffer) {
				IContextPtr newobj(new websocket::Context());
				//assert(std::dynamic_pointer_cast<IContext>(newobj));
				if (newobj) {
					newobj->setHttpContext(context);
					newobj->setDataBuffer(dataBuffer);
					newobj->setControlBuffer(controlBuffer);
					newobj->setCallbackHandler(handler);
				}
				else {
					printf("websocket::context_new() error\n");
				}
				return newobj;
			}

			//context_free free websocket::IContextPtr
			//@param IContextPtr& "websocket context"
			/*extern*/ /*"C"*/ void context_free(IContextPtr& context) {
				if (context) {
					context.reset();
				}
			}

			//testValidate 测试帧头合法性
			void frame_header_t::testValidate() {
				header_t const& header = this->header.get_header();
				switch (header.opcode)
				{
					//非控制帧，携带应用/扩展数据
				case OpcodeE::SegmentMessage:
					//分片消息帧(消息片段)，中间数据帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//连续帧/中间帧/分片帧
						assert(messageFrameType == MessageFrameE::MiddleFrame);
						break;
					}
					case FinE::FrameFinished: {
						//尾帧/结束帧
						assert(messageFrameType == MessageFrameE::TailFrame);
						break;
					}
					}
					//控制类型安全性检查
					assert(frameControlType == FrameControlE::UnControlFrame);
					break;

					//非控制帧，携带应用/扩展数据
				case OpcodeE::TextMessage:
					//文本消息帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//头帧/首帧/起始帧
						assert(messageFrameType == MessageFrameE::HeadFrame);
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						assert(messageFrameType == MessageFrameE::HeadTailFrame);
						break;
					}
					}
					//控制类型安全性检查
					assert(frameControlType == FrameControlE::UnControlFrame);
					break;

					//非控制帧，携带应用/扩展数据
				case OpcodeE::BinaryMessage:
					//二进制消息帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//头帧/首帧/起始帧
						assert(messageFrameType == MessageFrameE::HeadFrame);
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						assert(messageFrameType == MessageFrameE::HeadTailFrame);
						break;
					}
					}
					//控制类型安全性检查
					assert(frameControlType == FrameControlE::UnControlFrame);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::CloseMessage:
					//连接关闭帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查 ///
						assert(header.FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						assert(messageFrameType == MessageFrameE::HeadTailFrame);
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧 bug??????
						//assert(messageFrameType == MessageFrameE::HeadTailFrame);
						break;
					}
					}
					//控制类型安全性检查
					assert(frameControlType == FrameControlE::ControlFrame);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::PingMessage:
					//心跳PING探测帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查 ///
						assert(header.FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						assert(messageFrameType == MessageFrameE::HeadTailFrame);
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						assert(messageFrameType == MessageFrameE::HeadTailFrame);
						break;
					}
					}
					//控制类型安全性检查
					assert(frameControlType == FrameControlE::ControlFrame);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::PongMessage:
					//心跳PONG探测帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查 ///
						assert(header.FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						assert(messageFrameType == MessageFrameE::HeadTailFrame);
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						assert(messageFrameType == MessageFrameE::HeadTailFrame);
						break;
					}
					}
					//控制类型安全性检查
					assert(frameControlType == FrameControlE::ControlFrame);
					break;
				default:
					assert(false);
					break;
				}
			}

			//addFrameHeader 添加单个帧头
			MessageFrameE message_header_t::addFrameHeader(websocket::extended_header_t& header) {
				//帧控制类型 控制帧/非控制帧
				FrameControlE frameControlType = FrameControlE::FrameInvalid;
				//消息帧类型
				MessageFrameE messageFrameType = MessageFrameE::UnknownFrame;
				switch (header.get_header().opcode)
				{
					//非控制帧，携带应用/扩展数据
				case OpcodeE::SegmentMessage://分片消息
					switch (header.get_header().FIN) {
					case FinE::FrameContinue: {
						//连续帧/中间帧/分片帧
						messageFrameType = MessageFrameE::MiddleFrame;
						break;
					}
					case FinE::FrameFinished: {
						//尾帧/结束帧
						messageFrameType = MessageFrameE::TailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::UnControlFrame;
					//消息类型安全性检查
					assert(
						messageType_ == MessageE::TextMessage ||
						messageType_ == MessageE::BinaryMessage);
					break;

					//非控制帧，携带应用/扩展数据
				case OpcodeE::TextMessage:
					//文本消息帧
					switch (header.get_header().FIN) {
					case FinE::FrameContinue: {
						//头帧/首帧/起始帧
						messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::UnControlFrame;
					//消息类型安全性检查
					assert(messageType_ == MessageE::TextMessage);
					break;

					//非控制帧，携带应用/扩展数据
				case OpcodeE::BinaryMessage:
					//二进制消息帧
					switch (header.get_header().FIN) {
					case FinE::FrameContinue: {
						//头帧/首帧/起始帧
						messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::UnControlFrame;
					//消息类型安全性检查
					assert(messageType_ == MessageE::BinaryMessage);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::CloseMessage:
					//连接关闭帧
					switch (header.get_header().FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查 ///
						assert(header.get_header().FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						//messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::ControlFrame;
					//消息类型安全性检查
					assert(messageType_ == MessageE::CloseMessage);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::PingMessage:
					//心跳PING探测帧
					switch (header.get_header().FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查 ///
						assert(header.get_header().FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						//messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::ControlFrame;
					//消息类型安全性检查
					assert(messageType_ == MessageE::PingMessage);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::PongMessage:
					//心跳PONG探测帧
					switch (header.get_header().FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查 ///
						assert(header.get_header().FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						//messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::ControlFrame;
					//消息类型安全性检查
					assert(messageType_ == MessageE::PongMessage);
					break;
				default:
					assert(false);
					break;
				}
				//只有数据帧(非控制帧)可以分片，控制帧不能被分片 ///
				if (frameHeaders_.size() > 0) {
					for (std::vector<frame_header_t>::const_iterator it = frameHeaders_.begin();
						it != frameHeaders_.end(); ++it) {
						assert(it->getFrameControlType() == FrameControlE::UnControlFrame);
					}
				}
#if 0
				//frame_header_t
				frame_header_t frameHeader = {
								frameControlType:frameControlType,
								messageFrameType:messageFrameType,
				};
#else
				frame_header_t frameHeader = { 0 };
#endif
				frameHeader.setFrameControlType(frameControlType);
				frameHeader.setMessageFrameType(messageFrameType);
				frameHeader.set_header(header.get_header());
				frameHeader.setExtendedPayloadlen(header.getExtendedPayloadlenI64());
				frameHeader.setMaskingkey(header.getMaskingkey(), websocket::kMaskingkeyLen);
				//测试帧头合法性 ///
				frameHeader.testValidate();
				//添加帧头
				frameHeaders_.emplace_back(frameHeader);
 				return messageFrameType;
			}

			//getAcceptKey
			static std::string getAcceptKey(const std::string& key) {
				//sha-1
				std::string KEY = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
				unsigned char sha1_md[20] = { 0 };
				SHA1((const unsigned char*)KEY.c_str(), KEY.length(), sha1_md);
				//base64 encode
				return Base64::Encode(sha1_md, 20);
			}

			//webtime
			static std::string webtime(time_t time) {
				struct tm timeValue;
				gmtime_r(&time, &timeValue);
				char buf[1024];
				// Wed, 20 Apr 2011 17:31:28 GMT
				strftime(buf, sizeof(buf) - 1, "%a, %d %b %Y %H:%M:%S %Z", &timeValue);
				return buf;
			}

			//webtime_now
			static std::string webtime_now() {
				return webtime(time(NULL));
			}

			//validate_uncontrol_frame_header
			static void validate_uncontrol_frame_header(websocket::header_t& header) {
				assert(header.opcode >= OpcodeE::SegmentMessage &&
					header.opcode <= OpcodeE::BinaryMessage);
			}

			//validate_control_frame_header
			static void validate_control_frame_header(websocket::header_t& header) {
				//控制帧，Payload len<=126字节，且不能被分片
				assert(header.Payloadlen <= 126);
				assert(header.FIN == FinE::FrameFinished);
				assert(header.opcode >= OpcodeE::CloseMessage &&
					header.opcode <= OpcodeE::PongMessage);
			}

			//parse_frame_header，int16_t
			static void parse_frame_header(websocket::header_t& header, IBytesBuffer* buf) {
#if 1
				//websocket::header_t header = { 0 };
				memcpy(&header, buf->peek(), websocket::kHeaderLen);
				buf->retrieve(websocket::kHeaderLen);
#else
				//         L                       H
				//buf[0] = opcode|RSV3|RSV2|RSV1|FIN
				uint8_t c1 = *buf->peek();
				header.opcode = c1 & 0x0F;
				header.FIN = (c1 >> 7) & 0xFF;
				//         L              H
				//buf[1] = Payload len|MASK
				uint8_t c2 = *(buf->peek() + 1);
				header.Payloadlen = c2 & 0x7F;
				header.MASK = (c2 >> 7) & 0xFF;
				buf->retrieve(websocket::kHeaderLen);
#endif
			}

			//pack_unmask_uncontrol_frame_header
			static websocket::header_t pack_unmask_uncontrol_frame_header(
				IBytesBuffer* buf,
				websocket::OpcodeE opcode, websocket::FinE fin, size_t Payloadlen) {
				//0~127(2^7-1) 0x‭7F‬ ///
				assert(Payloadlen <= 127);
				//websocket::header_t int16_t ///
				websocket::header_t header = { 0 };
#if 1
				header.opcode = opcode;
				header.FIN = fin;
				header.Payloadlen = Payloadlen;
				header.MASK = 0;//S2C-0
#else
				//         L                       H
				//buf[0] = opcode|RSV3|RSV2|RSV1|FIN
				header.opcode = opcode & 0x0F;
				header.FIN = 0x0 | (fin << 7);
				//         L              H
				//buf[1] = Payload len|MASK
				header.Payloadlen = Payloadlen & 0x7F;
				header.MASK = 0;//S2C-0
#endif
			//验证非控制帧头有效性 ///
				validate_uncontrol_frame_header(header);
				buf->append(&header, websocket::kHeaderLen);
				return header;
			}

			//pack_unmask_control_frame_header
			static websocket::header_t pack_unmask_control_frame_header(
				IBytesBuffer* buf,
				websocket::OpcodeE opcode, websocket::FinE fin, size_t Payloadlen) {
				//0~127(2^7-1) 0x‭7F‬ ///
				assert(Payloadlen <= 127);
				//websocket::header_t int16_t ///
				websocket::header_t header = { 0 };
#if 1
				header.opcode = opcode;
				header.FIN = fin;
				header.Payloadlen = Payloadlen;
				header.MASK = 0;//S2C-0
#else
				//         L                       H
				//buf[0] = opcode|RSV3|RSV2|RSV1|FIN
				header.opcode = opcode & 0x0F;
				header.FIN = 0x0 | (fin << 7);
				//         L              H
				//buf[1] = Payload len|MASK
				header.Payloadlen = Payloadlen & 0x7F;
				header.MASK = 0;//S2C-0
#endif
			//验证控制帧头有效性 ///
				validate_control_frame_header(header);
				buf->append(&header, websocket::kHeaderLen);
				return header;
			}

			//pack_unmask_uncontrol_frame S2C
			static websocket::header_t pack_unmask_uncontrol_frame(
				IBytesBuffer* buf,
				char const* data, size_t len,
				websocket::OpcodeE opcode, websocket::FinE fin) {
				//websocket::header_t int16_t ///
				websocket::header_t header = { 0 };
				//Masking_key ///
				uint8_t Masking_key[kMaskingkeyLen] = { 0 };
				size_t Payloadlen = 0;
				uint16_t ExtendedPayloadlenU16 = 0;
				int64_t ExtendedPayloadlenI64 = 0;
				//如果值为0~125，那么表示负载数据长度
				if (len < 126) {
					Payloadlen = len;
				}
				//若能表示short不溢出0~65535(2^16-1)
				else if (len <= (uint16_t)65535/*0xFFFF*/) {
					Payloadlen = 126;
					ExtendedPayloadlenU16 = Endian::hostToNetwork16(len);
				}
				else /*if (len <= (int64_t)0x7FFFFFFFFFFFFFFF)*/ {
					Payloadlen = 127;
					ExtendedPayloadlenI64 = (int16_t)Endian::hostToNetwork64(len);
				}
				header = pack_unmask_uncontrol_frame_header(buf, opcode, fin, Payloadlen);
				if (Payloadlen == 126) {
					buf->append(&ExtendedPayloadlenU16, websocket::kExtendedPayloadlen2Byte);
				}
				else if (Payloadlen == 127) {
					buf->append(&ExtendedPayloadlenI64, websocket::kExtendedPayloadlen8Byte);
				}
				if (header.MASK == 1) {
					buf->append(Masking_key, websocket::kMaskingkeyLen);
				}
				//带帧头的空包 ///
				if (data && len > 0) {
					buf->append(data, len);
				}
				return header;
			}

			//pack_unmask_control_frame
			websocket::header_t pack_unmask_control_frame(
				IBytesBuffer* buf,
				char const* data, size_t len,
				websocket::OpcodeE opcode, websocket::FinE fin) {
				//websocket::header_t int16_t ///
				websocket::header_t header = { 0 };
				//Masking_key ///
				uint8_t Masking_key[kMaskingkeyLen] = { 0 };
				size_t Payloadlen = 0;
				uint16_t ExtendedPayloadlenU16 = 0;
				int64_t ExtendedPayloadlenI64 = 0;
				//如果值为0~125，那么表示负载数据长度
				if (len < 126) {
					Payloadlen = len;
				}
				//若能表示short不溢出0~65535(2^16-1)
				else if (len <= (uint16_t)65535/*0xFFFF*/) {
					Payloadlen = 126;
					ExtendedPayloadlenU16 = Endian::hostToNetwork16(len);
				}
				else /*if (len <= (int64_t)0x7FFFFFFFFFFFFFFF)*/ {
					Payloadlen = 127;
					ExtendedPayloadlenI64 = (int16_t)Endian::hostToNetwork64(len);
				}
				header = pack_unmask_control_frame_header(buf, opcode, fin, Payloadlen);
				if (Payloadlen == 126) {
					buf->append(&ExtendedPayloadlenU16, websocket::kExtendedPayloadlen2Byte);
				}
				else if (Payloadlen == 127) {
					//控制帧，Payload len<=126字节，且不能被分片
					printf("pack_unmask_control_frame Payloadlen =%d ExtendedPayloadlenI64 = %lld\n",
						Payloadlen, ExtendedPayloadlenI64);
					assert(false);
					buf->append(&ExtendedPayloadlenI64, websocket::kExtendedPayloadlen8Byte);
				}
				if (header.MASK == 1) {
					buf->append(Masking_key, websocket::kMaskingkeyLen);
				}
				//带帧头的空包 ///
				if (data && len > 0) {
					buf->append(data, len);
				}
				return header;
			}
			
			//FrameControlE MessageFrameE use MAKEWORD best
			static std::pair<FrameControlE, MessageFrameE>
				get_frame_control_message_type(websocket::header_t const& header) {
				//帧控制类型 控制帧/非控制帧
				websocket::FrameControlE frameControlType = FrameControlE::FrameInvalid;
				//消息帧类型
				websocket::MessageFrameE messageFrameType = MessageFrameE::UnknownFrame;
				switch (header.opcode)
				{
					//非控制帧，携带应用/扩展数据
				case OpcodeE::SegmentMessage://分片消息
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//连续帧/中间帧/分片帧
						messageFrameType = MessageFrameE::MiddleFrame;
						break;
					}
					case FinE::FrameFinished: {
						//尾帧/结束帧
						messageFrameType = MessageFrameE::TailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::UnControlFrame;
					//消息类型安全性检查
					//assert(
					//	messageType_ == MessageE::TextMessage ||
					//	messageType_ == MessageE::BinaryMessage);
					break;

					//非控制帧，携带应用/扩展数据
				case OpcodeE::TextMessage:
					//文本消息帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//头帧/首帧/起始帧
						messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::UnControlFrame;
					//消息类型安全性检查
					//assert(messageType_ == MessageE::TextMessage);
					break;

					//非控制帧，携带应用/扩展数据
				case OpcodeE::BinaryMessage:
					//二进制消息帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//头帧/首帧/起始帧
						messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::UnControlFrame;
					//消息类型安全性检查
					//assert(messageType_ == MessageE::BinaryMessage);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::CloseMessage:
					//连接关闭帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查 ///
						assert(header.FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						//messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::ControlFrame;
					//消息类型安全性检查
					//assert(messageType_ == MessageE::CloseMessage);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::PingMessage:
					//心跳PING探测帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查 ///
						assert(header.FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						//messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::ControlFrame;
					//消息类型安全性检查
					//assert(messageType_ == MessageE::PingMessage);
					break;

					//控制帧，Payload len<=126字节，且不能被分片
				case OpcodeE::PongMessage:
					//心跳PONG探测帧
					switch (header.FIN) {
					case FinE::FrameContinue: {
						//未分片安全性检查 ///
						assert(header.FIN == FinE::FrameFinished);
						//头帧/首帧/起始帧
						//messageFrameType = MessageFrameE::HeadFrame;
						break;
					}
					case FinE::FrameFinished: {
						//未分片的消息，既是头帧/首帧/起始帧，也是尾帧/结束帧
						messageFrameType = MessageFrameE::HeadTailFrame;
						break;
					}
					}
					frameControlType = FrameControlE::ControlFrame;
					//消息类型安全性检查
					//assert(messageType_ == MessageE::PongMessage);
					break;
				default:
					assert(false);
					break;
				}
				return std::pair<FrameControlE, MessageFrameE>(frameControlType, messageFrameType);
			}

			//pack_unmask_data_frame_chunk S2C
			static void pack_unmask_data_frame_chunk(
				IBytesBuffer* buf,
				char const* data, size_t len,
				websocket::MessageE messageType/* = MessageE::TextMessage*/, size_t chunksz/* = 1024*/) {
				assert(len > chunksz);
				assert(messageType == OpcodeE::TextMessage ||
					messageType == OpcodeE::BinaryMessage);
				ssize_t left = (ssize_t)len;
				ssize_t n = 0;
				websocket::header_t header = pack_unmask_uncontrol_frame(
					buf, data + n, chunksz,
					messageType, websocket::FinE::FrameContinue);
				n += chunksz;
				left -= chunksz;
				int i = 0;
				std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(header);
				printf("\n\n--- *** websocket::pack_unmask_data_frame_chunk:\n");
				printf("+-----------------------------------------------------------------------------------------------------------------+\n");
				printf("Frame[%d][%s][%s][%s] opcode[%s] FIN[%s] Payloadlen[%d] MASK[%d]\n",
					i++,
					websocket::FrameControl_to_string(ty.first).c_str(),
					websocket::Opcode_to_string(messageType).c_str(),
					websocket::MessageFrame_to_string(ty.second).c_str(),
					websocket::Opcode_to_string(header.opcode).c_str(),
					websocket::Fin_to_string(header.FIN).c_str(), header.Payloadlen, header.MASK);
				while (left > 0) {
					size_t size = left > chunksz ? chunksz : left;
					if (left > chunksz) {
						websocket::header_t header = pack_unmask_uncontrol_frame(
							buf, data + n, chunksz,
							websocket::OpcodeE::SegmentMessage,
							websocket::FinE::FrameContinue);
						n += chunksz;
						left -= chunksz;
						std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(header);
						printf("Frame[%d][%s][%s][%s] opcode[%s] FIN[%s] Payloadlen[%d] MASK[%d]\n",
							i++,
							websocket::FrameControl_to_string(ty.first).c_str(),
							websocket::Opcode_to_string(messageType).c_str(),
							websocket::MessageFrame_to_string(ty.second).c_str(),
							websocket::Opcode_to_string(header.opcode).c_str(),
							websocket::Fin_to_string(header.FIN).c_str(), header.Payloadlen, header.MASK);
					}
					else {
						websocket::header_t header = pack_unmask_uncontrol_frame(
							buf, data + n, left,
							websocket::OpcodeE::SegmentMessage,
							websocket::FinE::FrameFinished);
						n += left;
						left -= left;
						std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(header);
						printf("Frame[%d][%s][%s][%s] opcode[%s] FIN[%s] Payloadlen[%d] MASK[%d]\n",
							i++,
							websocket::FrameControl_to_string(ty.first).c_str(),
							websocket::Opcode_to_string(messageType).c_str(),
							websocket::MessageFrame_to_string(ty.second).c_str(),
							websocket::Opcode_to_string(header.opcode).c_str(),
							websocket::Fin_to_string(header.FIN).c_str(), header.Payloadlen, header.MASK);
						printf("+-----------------------------------------------------------------------------------------------------------------+\n\n");
						assert(left == 0);
						break;
					}
				}
			}

			//pack_unmask_data_frame S2C
			void pack_unmask_data_frame(
				IBytesBuffer* buf,
				char const* data, size_t len,
				MessageT msgType /*= MessageT::TyTextMessage*/, bool chunk/* = false*/) {
				websocket::MessageE messageType = 
					msgType == MessageT::TyTextMessage ?
					websocket::MessageE::TextMessage :
					websocket::MessageE::BinaryMessage;

				static const size_t chunksz = 1024;
				if (chunk && len > chunksz) {
					pack_unmask_data_frame_chunk(
						buf, data, len, messageType, chunksz);
				}
				else {
					websocket::header_t header = pack_unmask_uncontrol_frame(
						buf, data, len,
						messageType, websocket::FinE::FrameFinished);
					int i = 0;
					std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(header);
					printf("\n\n--- *** websocket::pack_unmask_data_frame:\n" \
						"+-----------------------------------------------------------------------------------------------------------------+\n");
					printf("Frame[%d][%s][%s][%s] opcode[%s] FIN[%s] Payloadlen[%d] MASK[%d]\n",
						i++,
						websocket::FrameControl_to_string(ty.first).c_str(),
						websocket::Opcode_to_string(messageType).c_str(),
						websocket::MessageFrame_to_string(ty.second).c_str(),
						websocket::Opcode_to_string(header.opcode).c_str(),
						websocket::Fin_to_string(header.FIN).c_str(), header.Payloadlen, header.MASK);
					printf("+-----------------------------------------------------------------------------------------------------------------+\n\n");
				}
			}

			
			//pack_unmask_close_frame S2C
			void pack_unmask_close_frame(
				IBytesBuffer* buf,
				char const* data, size_t len) {
				websocket::header_t header = pack_unmask_control_frame(
					buf, data, len,
					websocket::OpcodeE::CloseMessage,
					websocket::FinE::FrameFinished);
				int i = 0;
				std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(header);
				printf("\n\n--- *** websocket::pack_unmask_close_frame:\n" \
					"+-----------------------------------------------------------------------------------------------------------------+\n");
				printf("Frame[%d][%s][%s][%s] opcode[%s] FIN[%s] Payloadlen[%d] MASK[%d]\n",
					i++,
					websocket::FrameControl_to_string(ty.first).c_str(),
					websocket::Opcode_to_string(websocket::OpcodeE::CloseMessage).c_str(),
					websocket::MessageFrame_to_string(ty.second).c_str(),
					websocket::Opcode_to_string(header.opcode).c_str(),
					websocket::Fin_to_string(header.FIN).c_str(), header.Payloadlen, header.MASK);
				printf("+-----------------------------------------------------------------------------------------------------------------+\n\n");
			}

			//pack_unmask_ping_frame S2C
			void pack_unmask_ping_frame(
				IBytesBuffer* buf,
				char const* data, size_t len) {
				websocket::header_t header = pack_unmask_control_frame(
					buf, data, len,
					websocket::OpcodeE::PingMessage,
					websocket::FinE::FrameFinished);
				int i = 0;
				std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(header);
				printf("\n\n--- *** websocket::pack_unmask_ping_frame:\n" \
					"+-----------------------------------------------------------------------------------------------------------------+\n");
				printf("Frame[%d][%s][%s][%s] opcode[%s] FIN[%s] Payloadlen[%d] MASK[%d]\n",
					i++,
					websocket::FrameControl_to_string(ty.first).c_str(),
					websocket::Opcode_to_string(websocket::OpcodeE::PingMessage).c_str(),
					websocket::MessageFrame_to_string(ty.second).c_str(),
					websocket::Opcode_to_string(header.opcode).c_str(),
					websocket::Fin_to_string(header.FIN).c_str(), header.Payloadlen, header.MASK);
				printf("+-----------------------------------------------------------------------------------------------------------------+\n\n");
			}

			//pack_unmask_pong_frame S2C
			void pack_unmask_pong_frame(
				IBytesBuffer* buf,
				char const* data, size_t len) {
				websocket::header_t header = pack_unmask_control_frame(
					buf, data, len,
					websocket::OpcodeE::PongMessage,
					websocket::FinE::FrameFinished);
				int i = 0;
				std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(header);
				printf("\n\n--- *** websocket::pack_unmask_pong_frame:\n" \
					"+-----------------------------------------------------------------------------------------------------------------+\n");
				printf("Frame[%d][%s][%s][%s] opcode[%s] FIN[%s] Payloadlen[%d] MASK[%d]\n",
					i++,
					websocket::FrameControl_to_string(ty.first).c_str(),
					websocket::Opcode_to_string(websocket::OpcodeE::PongMessage).c_str(),
					websocket::MessageFrame_to_string(ty.second).c_str(),
					websocket::Opcode_to_string(header.opcode).c_str(),
					websocket::Fin_to_string(header.FIN).c_str(), header.Payloadlen, header.MASK);
				printf("+-----------------------------------------------------------------------------------------------------------------+\n\n");
			}

			static uint8_t get_frame_FIN(websocket::header_t const& header) {
				return header.FIN;
			}
			static rsv123_t get_frame_RSV123(websocket::header_t const& header) {
				return (header.RSV1 << 2) | (header.RSV2 << 1) | (header.RSV3);
			}
			static uint8_t get_frame_RSV1(websocket::header_t const& header) {
				return header.RSV1;
			}
			static uint8_t get_frame_RSV2(websocket::header_t const& header) {
				return header.RSV2;
			}
			static uint8_t get_frame_RSV3(websocket::header_t const& header) {
				return header.RSV3;
			}
			static uint8_t get_frame_opcode(websocket::header_t const& header) {
				return header.opcode;
			}
			static uint8_t get_frame_MASK(websocket::header_t const& header) {
				return header.MASK;
			}
			static uint8_t get_frame_Payloadlen(websocket::header_t const& header) {
				return header.Payloadlen;
			}
			
			//dump websocket协议头信息
			static void dump_header_info(websocket::header_t const& header) {
				rsv123_t RSV123 = get_frame_RSV123(header);
				uint8_t RSV1 = get_frame_RSV1(header);
				uint8_t RSV2 = get_frame_RSV2(header);
				uint8_t RSV3 = get_frame_RSV3(header);
				//show hex to the left
				printf("--- *** dump_header_info\n" \
					"+---------------------------------------------------------------+\n" \
					"|0                   1                   2                   3  |\n" \
					"|0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|\n" \
					"+-+-+-+-+-------+-+-------------+-------------------------------+\n" \
					"+-+-+-+-+-------+-+-------------+-------------------------------+\n" \
					"|F|R|R|R| opcode|M| Payload len |    Extended payload length    |\n" \
					"|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |\n" \
					"|N|V|V|V|       |S|             |   (if payload len==126/127)   |\n" \
					"| |1|2|3|       |K|             |                               |\n" \
					"|%d|%01d|%01d|%01d|%07x|%01x|%013d|\n\n\n",
					header.FIN,
					RSV1, RSV2, RSV3,
					header.opcode,
					header.MASK,
					header.Payloadlen);
			}
			
			//dump websocket带扩展协议头信息
			static void dump_extended_header_info(websocket::extended_header_t const& header) {
				rsv123_t RSV123 = get_frame_RSV123(header.get_header());
				uint8_t RSV1 = get_frame_RSV1(header.get_header());
				uint8_t RSV2 = get_frame_RSV2(header.get_header());
				uint8_t RSV3 = get_frame_RSV3(header.get_header());
				if (header.existExtendedPayloadlen()) {
					if (header.existMaskingkey()) {
						//Extended payload length & Masking-key
						printf("--- *** dump_extended_header_info\n" \
							"+---------------------------------------------------------------+\n" \
							"|0                   1                   2                   3  |\n" \
							"|0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|\n" \
							"+-+-+-+-+-------+-+-------------+-------------------------------+\n" \
							"|F|R|R|R| opcode|M| Payload len |    Extended payload length    |\n" \
							"|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |\n" \
							"|N|V|V|V|       |S|             |   (if payload len==126/127)   |\n" \
							"| |1|2|3|       |K|             |                               |\n" \
							"|%d|%01d|%01d|%01d|%07x|%01x|%013d|%031d|\n" \
							"+-------------------------------+-------------------------------+\n" \
							"| Masking-key (continued)       |          Payload Data         |\n" \
							"+-------------------------------- - - - - - - - - - - - - - - - +\n" \
							"|%063d|\n\n\n",
							header.get_header().FIN,
							RSV1, RSV2, RSV3,
							header.get_header().opcode,
							header.get_header().MASK,
							header.get_header().Payloadlen,
							header.getExtendedPayloadlen(),
							Endian::be32BigEndian(header.getMaskingkey()));
					}
					else {
						//Extended payload length
						printf(
							"+---------------------------------------------------------------+\n" \
							"|0                   1                   2                   3  |\n" \
							"|0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|\n" \
							"+-+-+-+-+-------+-+-------------+-------------------------------+\n" \
							"|F|R|R|R| opcode|M| Payload len |    Extended payload length    |\n" \
							"|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |\n" \
							"|N|V|V|V|       |S|             |   (if payload len==126/127)   |\n" \
							"| |1|2|3|       |K|             |                               |\n" \
							"|%d|%01d|%01d|%01d|%07x|%01x|%013d|%031d|\n\n\n",
							header.get_header().FIN,
							RSV1, RSV2, RSV3,
							header.get_header().opcode,
							header.get_header().MASK,
							header.get_header().Payloadlen,
							header.getExtendedPayloadlen());
					}
				}
				else {
					if (header.existMaskingkey()) {
						//Masking-key
						printf(
							"+---------------------------------------------------------------+\n" \
							"|0                   1                   2                   3  |\n" \
							"|0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|\n" \
							"+-+-+-+-+-------+-+-------------+-------------------------------+\n" \
							"|F|R|R|R| opcode|M| Payload len |    Extended payload length    |\n" \
							"|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |\n" \
							"|N|V|V|V|       |S|             |   (if payload len==126/127)   |\n" \
							"| |1|2|3|       |K|             |                               |\n" \
							"|%d|%01d|%01d|%01d|%07x|%01x|%013d|\n" \
							"+-------------------------------+-------------------------------+\n" \
							"| Masking-key (continued)       |          Payload Data         |\n" \
							"+-------------------------------- - - - - - - - - - - - - - - - +\n" \
							"|%063d|\n\n\n",
							header.get_header().FIN,
							RSV1, RSV2, RSV3,
							header.get_header().opcode,
							header.get_header().MASK,
							header.get_header().Payloadlen,
							Endian::be32BigEndian(header.getMaskingkey()));
					}
					else {
						//show hex to the left
						printf(
							"+---------------------------------------------------------------+\n" \
							"|0                   1                   2                   3  |\n" \
							"|0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|\n" \
							"+-+-+-+-+-------+-+-------------+-------------------------------+\n" \
							"|F|R|R|R| opcode|M| Payload len |    Extended payload length    |\n" \
							"|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |\n" \
							"|N|V|V|V|       |S|             |   (if payload len==126/127)   |\n" \
							"| |1|2|3|       |K|             |                               |\n" \
							"|%d|%01d|%01d|%01d|%07x|%01x|%013d|\n\n\n",
							header.get_header().FIN,
							RSV1, RSV2, RSV3,
							header.get_header().opcode,
							header.get_header().MASK,
							header.get_header().Payloadlen);
					}
				}
			}

			//parse_control_frame_body_payload_data Payload data 非控制帧(数据帧)
			//@param websocket::Context& 组件内部私有接口
// 			static bool parse_control_frame_body_payload_data(
// 				websocket::Context& context,
// 				IBytesBuffer /*const*/* buf,
// 				ITimestamp* receiveTime, int* savedErrno) {
// 				
// 				
// 			}

			//validate_message_frame 消息帧有效性安全检查
			//@param websocket::Context& 组件内部私有接口
			static bool validate_message_frame(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno) {

				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();

				//数据帧(非控制帧)，携带应用/扩展数据
				websocket::Message& dataMessage = context.getDataMessage();
				//数据帧(非控制帧)，完整websocket消息头(header)
				//websocket::message_header_t& messageHeader = dataMessage.getMessageHeader();
				//数据帧(非控制帧)，完整websocket消息体(body)
				//IBytesBufferPtr messageBuffer = dataMessage.getMessageBuffer();

				//控制帧，Payload len<=126字节，且不能被分片
				websocket::Message& controlMessage = context.getControlMessage();
				//控制帧，完整websocket消息头(header)
				//websocket::message_header_t& controlHeader = controlMessage.getMessageHeader();
				//控制帧，完整websocket消息体(body)
				//IBytesBufferPtr controlBuffer = controlMessage.getMessageBuffer();

				bool bok = true;
				do {
					//消息帧类型
					switch (header.opcode)
					{
					case OpcodeE::SegmentMessage: {
						//分片消息帧(消息片段)，中间数据帧
						assert(controlMessage.getMessageHeader().getFrameHeaderSize() == 0);
						assert(dataMessage.getMessageHeader().getFrameHeaderSize() > 0);
						assert(
							dataMessage.getMessageHeader().getMessageType() == MessageE::TextMessage ||
							dataMessage.getMessageHeader().getMessageType() == MessageE::BinaryMessage);
						//websocket::MessageFrameE messageFrameType = dataMessage.addFrameHeader(extended_header);
						//一定不是头帧
						//assert(!(messageFrameType & websocket::MessageFrameE::HeadFrame));
						//////////////////////////////////////////////////////////////////////////
						//非控制帧(数据帧) frame body
						//Maybe include Extended payload length
						//Maybe include Masking-key
						//Must include Payload data
						break;
					}
					case OpcodeE::TextMessage: {
						//文本消息帧
						//开始解析消息体，解析buffer务必已置空
						assert(dataMessage.getMessageBuffer());
						assert(dataMessage.getMessageBuffer()->readableBytes() == 0);
						//数据帧消息类型
						dataMessage.setMessageType(websocket::OpcodeE::TextMessage);
						//初始数据帧头
						//websocket::MessageFrameE messageFrameType = dataMessage.addFrameHeader(header);
						//一定是头帧
						//assert(messageFrameType & websocket::MessageFrameE::HeadFrame);
						//////////////////////////////////////////////////////////////////////////
						//非控制帧(数据帧) frame body
						//Maybe include Extended payload length
						//Maybe include Masking-key
						//Must include Payload data
						break;
					}
					case OpcodeE::BinaryMessage: {
						//二进制消息帧
						//开始解析消息体，解析buffer务必已置空
						assert(dataMessage.getMessageBuffer());
						assert(dataMessage.getMessageBuffer()->readableBytes() == 0);
						//数据帧消息类型
						dataMessage.setMessageType(websocket::OpcodeE::BinaryMessage);
						//初始数据帧头
						//websocket::MessageFrameE messageFrameType = dataMessage.addFrameHeader(header);
						//一定是头帧
						//assert(messageFrameType & websocket::MessageFrameE::HeadFrame);
						//////////////////////////////////////////////////////////////////////////
						//非控制帧(数据帧) frame body
						//Maybe include Extended payload length
						//Maybe include Masking-key
						//Must include Payload data
						break;
					}
					case OpcodeE::CloseMessage: {
						//连接关闭帧
						//开始解析消息体，解析buffer务必已置空
						assert(controlMessage.getMessageBuffer());
						assert(controlMessage.getMessageBuffer()->readableBytes() == 0);
						//控制帧消息类型
						controlMessage.setMessageType(websocket::OpcodeE::CloseMessage);
						//初始控制帧头
						//websocket::MessageFrameE messageFrameType = controlMessage.addFrameHeader(header);
						//既是是头帧也是尾帧
						//assert(messageFrameType & websocket::MessageFrameE::HeadFrame);
						//assert(messageFrameType & websocket::MessageFrameE::TailFrame);
						//////////////////////////////////////////////////////////////////////////
						//控制帧 frame body
						//Maybe include Extended payload length(<=126)
						//Maybe include Masking-key
						//Maybe include Payload data
						break;
					}
					case OpcodeE::PingMessage: {
						//心跳PING探测帧
						//开始解析消息体，解析buffer务必已置空
						assert(controlMessage.getMessageBuffer());
						assert(controlMessage.getMessageBuffer()->readableBytes() == 0);
						//控制帧消息类型
						controlMessage.setMessageType(websocket::OpcodeE::PingMessage);
						//初始控制帧头
						//websocket::MessageFrameE messageFrameType = controlMessage.addFrameHeader(header);
						//既是是头帧也是尾帧
						//assert(messageFrameType & websocket::MessageFrameE::HeadFrame);
						//assert(messageFrameType & websocket::MessageFrameE::TailFrame);
						//////////////////////////////////////////////////////////////////////////
						//控制帧 frame body
						//Maybe include Extended payload length(<=126)
						//Maybe include Masking-key
						//Maybe include Payload data
						break;
					}
					case OpcodeE::PongMessage: {
						//心跳PONG探测帧
						//开始解析消息体，解析buffer务必已置空
						assert(controlMessage.getMessageBuffer());
						assert(controlMessage.getMessageBuffer()->readableBytes() == 0);
						//控制帧消息类型
						controlMessage.setMessageType(websocket::OpcodeE::PongMessage);
						//初始控制帧头
						//websocket::MessageFrameE messageFrameType = controlMessage.addFrameHeader(header);
						//////////////////////////////////////////////////////////////////////////
						//控制帧 frame body
						//Maybe include Extended payload length(<=126)
						//Maybe include Masking-key
						//Maybe include Payload data
						break;
					}
					}
				} while (0);
				return bok;
			}

			//parse_frame_ReadExtendedPayloadlen2Byte
			//@param websocket::Context& 组件内部私有接口
			static int parse_frame_ReadExtendedPayloadlen2Byte(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);
			
			//parse_frame_ReadExtendedPayloadlen8Byte
			//@param websocket::Context& 组件内部私有接口
			static int parse_frame_ReadExtendedPayloadlen8Byte(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);
			
			//parse_frame_ReadMaskingkey
			//@param websocket::Context& 组件内部私有接口
			static int parse_frame_ReadMaskingkey(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);
			
			//parse_control_frame_ReadPayloadData
			//@param websocket::Context& 组件内部私有接口
			static bool parse_control_frame_ReadPayloadData(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);
			
			//parse_uncontrol_frame_ReadPayloadData
			//@param websocket::Context& 组件内部私有接口
			static bool parse_uncontrol_frame_ReadPayloadData(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//parse_uncontrol_frame_ReadExtendedPayloadDataU16
			//@param websocket::Context& 组件内部私有接口
			static bool parse_uncontrol_frame_ReadExtendedPayloadDataU16(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//parse_uncontrol_frame_ReadExtendedPayloadDataI64
			//@param websocket::Context& 组件内部私有接口
			static bool parse_uncontrol_frame_ReadExtendedPayloadDataI64(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//parse_control_frame_ReadPayloadData
			//@param websocket::Context& 组件内部私有接口
			static bool parse_control_frame_ReadPayloadData(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//parse_control_frame_ReadExtendedPayloadDataU16
			//@param websocket::Context& 组件内部私有接口
			static bool parse_control_frame_ReadExtendedPayloadDataU16(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno);

			//parse_control_frame_ReadExtendedPayloadDataI64
			//@param websocket::Context& 组件内部私有接口
			static bool parse_control_frame_ReadExtendedPayloadDataI64(
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
			static bool update_frame_body_parse_step(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				bool bok = true;
				do {
					//之前解析基础协议头/帧头(header) header_t，uint16_t
					assert(context.getWebsocketState() == websocket::StepE::ReadFrameHeader);
					//Payloadlen 判断
					switch (header.Payloadlen) {
					case 126: {
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadExtendedPayloadlen2Byte
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadExtendedPayloadlen2Byte);
						{
							parse_frame_ReadExtendedPayloadlen2Byte(context, buf, receiveTime, savedErrno);
						}
						break;
					}
					case 127: {
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadExtendedPayloadlen8Byte
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadExtendedPayloadlen8Byte);
						{
							parse_frame_ReadExtendedPayloadlen8Byte(context, buf, receiveTime, savedErrno);
						}
						break;
					}
					default: {
						assert(header.Payloadlen < 126);
						//判断 MASK = 1，读取Masking_key ///
						if (header.MASK == 1) {
							//////////////////////////////////////////////////////////////////////////
							//StepE::ReadMaskingkey
							//////////////////////////////////////////////////////////////////////////
							context.setWebsocketStep(websocket::StepE::ReadMaskingkey);
							{
								parse_frame_ReadMaskingkey(context, buf, receiveTime, savedErrno);
							}
							break;
						}
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadPayloadData
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadPayloadData);
						{
							parse_control_frame_ReadPayloadData(context, buf, receiveTime, savedErrno);
						}
						break;
					}
					}
				} while (0);
				return bok;
			}

			//parse_frame_ReadFrameHeader
			//	解析基础协议头/帧头(header)
			//	输出基础协议头信息
			//	消息帧有效性安全检查
			//	更新帧体(body)消息流解析步骤step
			//@param websocket::Context& 组件内部私有接口
			static bool parse_frame_ReadFrameHeader(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				bool enough = true;
				do {
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadFrameHeader
					//////////////////////////////////////////////////////////////////////////
					assert(context.getWebsocketStep() == StepE::ReadFrameHeader);
					//数据包不足够解析，等待下次接收再解析
					if (buf->readableBytes() < websocket::kHeaderLen) {
						printf("websocket::parse_frame_ReadFrameHeader[bad][%d]: %s(%d) readableBytes(%d)\n",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), websocket::kHeaderLen, buf->readableBytes());
						enough = false;
						break;
					}

					printf("websocket::parse_frame_ReadFrameHeader[ok][%d]: %s(%d) readableBytes(%d)\n",
						header.Payloadlen,
						websocket::Step_to_string(context.getWebsocketStep()).c_str(), websocket::kHeaderLen, buf->readableBytes());

					//解析基础协议头/帧头(header)
					parse_frame_header(header, buf);
#if 0
					//输出基础协议头信息
					dump_header_info(header);
#endif
					//消息帧有效性安全检查
					validate_message_frame(context, buf, receiveTime, savedErrno);
					//更新帧体(body)消息流解析步骤step
					//	非控制帧(数据帧) frame body
					//		Maybe include Extended payload length
					//		Maybe include Masking-key
					//		Must include Payload data
					//	控制帧 frame body
					//		Maybe include Extended payload length(<=126)
					//		Maybe include Masking-key
					//		Maybe include Payload data
					update_frame_body_parse_step(context, buf, receiveTime, savedErrno);
				} while (0);
				return enough;
			}

			//parse_frame_ReadExtendedPayloadlen2Byte
			//@param websocket::Context& 组件内部私有接口
			static int parse_frame_ReadExtendedPayloadlen2Byte(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				bool enough = true;
				do {
					//判断 Payloadlen = 126，读取后续2字节
					assert(header.Payloadlen == 126);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadExtendedPayloadlen2Byte
					//////////////////////////////////////////////////////////////////////////
					assert(context.getWebsocketStep() == websocket::StepE::ReadExtendedPayloadlen2Byte);
					if (buf->readableBytes() < websocket::kExtendedPayloadlen2Byte) {
						//读取后续2字节，不够
						printf("websocket::parse_frame_ReadExtendedPayloadlen2Byte[bad][%d]: %s(%d) readableBytes(%d)\n",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), websocket::kExtendedPayloadlen2Byte, buf->readableBytes());
						enough = false;
						break;
					}
					
					printf("websocket::parse_frame_ReadExtendedPayloadlen2Byte[ok][%d]: %s(%d) readableBytes(%d)\n",
						header.Payloadlen,
						websocket::Step_to_string(context.getWebsocketStep()).c_str(), websocket::kExtendedPayloadlen2Byte, buf->readableBytes());

					//读取后续2字节 Extended payload length real Payload Data length
					extended_header.setExtendedPayloadlen(Endian::networkToHost16(buf->readInt16()));
					//判断 MASK = 1，读取Masking_key ///
					if (header.MASK == 1) {
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadMaskingkey
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadMaskingkey);
						break;
					}
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadPayloadData/ReadExtendedPayloadDataU16
					//////////////////////////////////////////////////////////////////////////
					context.setWebsocketStep(websocket::StepE::ReadExtendedPayloadDataU16);
				} while (0);
				return enough;
			}
			
			//parse_frame_ReadExtendedPayloadlen8Byte
			//@param websocket::Context& 组件内部私有接口
			static int parse_frame_ReadExtendedPayloadlen8Byte(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				bool enough = true;
				do {
					//判断 Payloadlen = 127，读取后续8字节
					assert(header.Payloadlen == 127);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadExtendedPayloadlen8Byte
					//////////////////////////////////////////////////////////////////////////
					assert(context.getWebsocketStep() == websocket::StepE::ReadExtendedPayloadlen8Byte);
					if (buf->readableBytes() < websocket::kExtendedPayloadlen8Byte) {
						//读取后续8字节，不够
						printf("websocket::parse_frame_ReadExtendedPayloadlen8Byte[bad][%d]: %s(%d) readableBytes(%d)\n",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), websocket::kExtendedPayloadlen8Byte, buf->readableBytes());
						enough = false;
						break;
					}

					printf("websocket::parse_frame_ReadExtendedPayloadlen8Byte[ok][%d]: %s(%d) readableBytes(%d)\n",
						header.Payloadlen,
						websocket::Step_to_string(context.getWebsocketStep()).c_str(), websocket::kMaskingkeyLen, buf->readableBytes());

					//读取后续8字节 Extended payload length real Payload Data length
					extended_header.setExtendedPayloadlen((int64_t)Endian::networkToHost64(buf->readInt64()));
					//判断 MASK = 1，读取Masking_key ///
					if (header.MASK == 1) {
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadMaskingkey
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadMaskingkey);
						break;
					}
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadPayloadData/ReadExtendedPayloadDataI64
					//////////////////////////////////////////////////////////////////////////
					context.setWebsocketStep(websocket::StepE::ReadExtendedPayloadDataI64);
				} while (0);
				return enough;
			}

			//parse_frame_ReadMaskingkey
			//@param websocket::Context& 组件内部私有接口
			static int parse_frame_ReadMaskingkey(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				bool enough = true;
				do {
					//判断 MASK = 1，读取Masking_key
					assert(header.MASK == 1);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadMaskingkey
					//////////////////////////////////////////////////////////////////////////
					assert(context.getWebsocketStep() == websocket::StepE::ReadMaskingkey);
					if (buf->readableBytes() < websocket::kMaskingkeyLen) {
						//读取不够
						printf("websocket::parse_frame_ReadMaskingkey[bad][%d]: %s(%d) readableBytes(%d)\n",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), websocket::kMaskingkeyLen, buf->readableBytes());
						enough = false;
						break;
					}

					printf("websocket::parse_frame_ReadMaskingkey[ok][%d]: %s(%d) readableBytes(%d)\n",
						header.Payloadlen,
						websocket::Step_to_string(context.getWebsocketStep()).c_str(), websocket::kMaskingkeyLen, buf->readableBytes());

					//读取Masking_key
					extended_header.setMaskingkey((uint8_t const*)buf->peek(), websocket::kMaskingkeyLen);
					buf->retrieve(websocket::kMaskingkeyLen);
					//Payloadlen 判断
					switch (header.Payloadlen) {
					case 126: {
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadPayloadData/ReadExtendedPayloadDataU16
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadExtendedPayloadDataU16);
						{
							std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(header);
							assert(ty.first != FrameControlE::FrameInvalid);
							enough = (ty.first == FrameControlE::UnControlFrame) ?
								parse_uncontrol_frame_ReadExtendedPayloadDataU16(context, buf, receiveTime, savedErrno) :
								parse_control_frame_ReadExtendedPayloadDataU16(context, buf, receiveTime, savedErrno);
						}
						break;
					}
					case 127: {
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadPayloadData/ReadExtendedPayloadDataI64
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadExtendedPayloadDataI64);
						{
							std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(header);
							assert(ty.first != FrameControlE::FrameInvalid);
							enough = (ty.first == FrameControlE::UnControlFrame) ?
								parse_uncontrol_frame_ReadExtendedPayloadDataI64(context, buf, receiveTime, savedErrno) :
								parse_control_frame_ReadExtendedPayloadDataI64(context, buf, receiveTime, savedErrno);
						}
						break;
					}
					default: {
						assert(header.Payloadlen < 126);
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadPayloadData
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadPayloadData);
						{
							std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(header);
							assert(ty.first != FrameControlE::FrameInvalid);
							enough = (ty.first == FrameControlE::UnControlFrame) ?
								parse_uncontrol_frame_ReadPayloadData(context, buf, receiveTime, savedErrno) :
								parse_control_frame_ReadPayloadData(context, buf, receiveTime, savedErrno);
						}
						break;
					}
					}
				} while (0);
				return enough;
			}

			//parse_uncontrol_frame_ReadPayloadData
			//@param websocket::Context& 组件内部私有接口
			static bool parse_uncontrol_frame_ReadPayloadData(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				websocket::ICallbackPtr handler(context.getCallbackHandler().lock());
				//数据帧(非控制帧)，携带应用/扩展数据
				//websocket::Message& dataMessage = context.getDataMessage();
				//数据帧(非控制帧)，完整websocket消息头(header)
				//websocket::message_header_t& messageHeader = dataMessage.getMessageHeader();
				//数据帧(非控制帧)，完整websocket消息体(body)
				//IBytesBufferPtr messageBuffer = dataMessage.getMessageBuffer();
				bool enough = true;
				do {
					assert(header.Payloadlen < 126);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadPayloadData
					//////////////////////////////////////////////////////////////////////////
					assert(context.getWebsocketStep() == websocket::StepE::ReadPayloadData);
					//读取Payload Data ///
					if (buf->readableBytes() < header.Payloadlen) {
						//读取不够
						printf("websocket::parse_uncontrol_frame_ReadPayloadData[bad][%d]: %s(%d) readableBytes(%d)\n",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), header.Payloadlen, buf->readableBytes());
						enough = false;
						break;
					}
					websocket::MessageFrameE messageFrameType = context.getDataMessage().addFrameHeader(extended_header);
					//输出扩展协议头信息
					dump_extended_header_info(extended_header);
					{
						printf("websocket::parse_uncontrol_frame_ReadPayloadData[ok][%d]: %s(%d) readableBytes(%d)\n",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), header.Payloadlen, buf->readableBytes());

						//读取Payload Data ///
						context.getDataMessage().getMessageBuffer()->append(buf->peek(), header.Payloadlen);
						buf->retrieve(header.Payloadlen);
						//////////////////////////////////////////////////////////////////////////
						//对Payload Data做UnMask计算 ///
						context.getDataMessage().unMaskPayloadData(
							header.MASK,
							context.getDataMessage().getMessageBuffer().get(),
							extended_header.getMaskingkey());
#if 0
						//buf没有粘包/半包情况 ///
						assert(buf->readableBytes() == 0);
#endif
						switch (header.FIN) {
						case FinE::FrameContinue: {
							//帧已结束(分片消息)，非完整消息包，继续从帧头开始解析 //////
							//分片消息连续帧
							//FIN = FrameContinue opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//FIN = FrameContinue opcode = SegmentMessage

							//包不完整不能重置数据帧消息buffer
							//context.resetDataMessage();
							break;
						}
						case FinE::FrameFinished: {
							//帧已结束(未分片/分片消息)，完整消息包，执行消息回调，继续从帧头开始解析 //////
							//未分片消息结束帧 FIN = FrameFinished opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//分片消息结束帧 FIN = FrameFinished opcode = SegmentMessage
							//打印数据帧全部帧头信息
							context.getDataMessage().dumpFrameHeaders("websocket::parse_uncontrol_frame_ReadPayloadData");
							switch (header.opcode) {
							case OpcodeE::SegmentMessage: {
								//分片消息帧(消息片段)，中间数据帧
								break;
							}
							case OpcodeE::TextMessage: {
								//文本消息帧
								break;
							}
							case OpcodeE::BinaryMessage: {
								//二进制消息帧
								break;
							}
							default:
								assert(false);
								break;
							}
							if (handler) {
								handler->onMessageCallback(
									context.getDataMessage().getMessageBuffer(),
									context.getDataMessage().getMessageType(), receiveTime);
							}
							if (context.getDataMessage().getMessageBuffer()->readableBytes() == header.Payloadlen) {
								//context.getDataMessage().getMessageBuffer()没有分片情况 ///
							}
							else {
								//context.getDataMessage().getMessageBuffer()必是分片消息 ///
							}
							//重置数据帧消息buffer
							context.resetDataMessage();
							assert(context.getDataMessage().getMessageBuffer()->readableBytes() == 0);
							break;
						}
						}
						//正处于解析当中的帧头(当前帧头)
						context.resetExtendedHeader();
						//继续从帧头开始解析下一帧
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadFrameHeader
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadFrameHeader);
					}
				} while (0);
				return enough;
			}

			//parse_uncontrol_frame_ReadExtendedPayloadDataU16
			//@param websocket::Context& 组件内部私有接口
			static bool parse_uncontrol_frame_ReadExtendedPayloadDataU16(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				websocket::ICallbackPtr handler(context.getCallbackHandler().lock());
				//数据帧(非控制帧)，携带应用/扩展数据
				//websocket::Message& dataMessage = context.getDataMessage();
				//数据帧(非控制帧)，完整websocket消息头(header)
				//websocket::message_header_t& messageHeader = dataMessage.getMessageHeader();
				//数据帧(非控制帧)，完整websocket消息体(body)
				//IBytesBufferPtr messageBuffer = dataMessage.getMessageBuffer();
				bool enough = true;
				do {
					assert(header.Payloadlen == 126);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadPayloadData/ReadExtendedPayloadDataU16
					//////////////////////////////////////////////////////////////////////////
					assert(context.getWebsocketStep() == websocket::StepE::ReadExtendedPayloadDataU16);
					//读取Payload Data ///
					if (buf->readableBytes() < extended_header.getExtendedPayloadlenU16()) {
						//读取不够
						printf("websocket::parse_uncontrol_frame_ReadExtendedPayloadDataU16[bad][%d]: %s(%d) readableBytes(%d)\n",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), extended_header.getExtendedPayloadlenU16(), buf->readableBytes());
						enough = false;
						break;
					}
					websocket::MessageFrameE messageFrameType = context.getDataMessage().addFrameHeader(extended_header);
					//输出扩展协议头信息
					dump_extended_header_info(extended_header);
					{
						printf("websocket::parse_uncontrol_frame_ReadExtendedPayloadDataU16[ok][%d]: %s(%d) readableBytes(%d)\n",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), extended_header.getExtendedPayloadlenU16(), buf->readableBytes());

						//读取Payload Data ///
						context.getDataMessage().getMessageBuffer()->append(buf->peek(), extended_header.getExtendedPayloadlenU16());
						buf->retrieve(extended_header.getExtendedPayloadlenU16());
						//////////////////////////////////////////////////////////////////////////
						//对Payload Data做UnMask计算 ///
						context.getDataMessage().unMaskPayloadData(
							header.MASK,
							context.getDataMessage().getMessageBuffer().get(),
							extended_header.getMaskingkey());
#if 0
						//buf没有粘包/半包情况 ///
						assert(buf->readableBytes() == 0);
#endif
						switch (header.FIN) {
						case FinE::FrameContinue: {
							//帧已结束(分片消息)，非完整消息包，继续从帧头开始解析 //////
							//分片消息连续帧
							//FIN = FrameContinue opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//FIN = FrameContinue opcode = SegmentMessage

							//包不完整不能重置数据帧消息buffer
							//context.resetDataMessage();
							break;
						}
						case FinE::FrameFinished: {
							//帧已结束(未分片/分片消息)，完整消息包，执行消息回调，继续从帧头开始解析 //////
							//未分片消息结束帧 FIN = FrameFinished opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//分片消息结束帧 FIN = FrameFinished opcode = SegmentMessage
							//打印数据帧全部帧头信息
							context.getDataMessage().dumpFrameHeaders("websocket::parse_uncontrol_frame_ReadExtendedPayloadDataU16");
							switch (header.opcode) {
							case OpcodeE::SegmentMessage: {
								//分片消息帧(消息片段)，中间数据帧
								break;
							}
							case OpcodeE::TextMessage: {
								//文本消息帧
								break;
							}
							case OpcodeE::BinaryMessage: {
								//二进制消息帧
								break;
							}
							default:
								assert(false);
								break;
							}
							if (handler) {
								handler->onMessageCallback(
									context.getDataMessage().getMessageBuffer(),
									context.getDataMessage().getMessageType(), receiveTime);
							}
							if (context.getDataMessage().getMessageBuffer()->readableBytes() == extended_header.getExtendedPayloadlenU16()) {
								//context.getDataMessage().getMessageBuffer()没有分片情况 ///
							}
							else {
								//context.getDataMessage().getMessageBuffer()必是分片消息 ///
							}
							//重置数据帧消息buffer
							context.resetDataMessage();
							assert(context.getDataMessage().getMessageBuffer()->readableBytes() == 0);
							break;
						}
						}
						//正处于解析当中的帧头(当前帧头)
						context.resetExtendedHeader();
						//继续从帧头开始解析下一帧
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadFrameHeader
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadFrameHeader);
					}
				} while (0);
				return enough;
			}

			//parse_uncontrol_frame_ReadExtendedPayloadDataI64
			//@param websocket::Context& 组件内部私有接口
			static bool parse_uncontrol_frame_ReadExtendedPayloadDataI64(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				websocket::ICallbackPtr handler(context.getCallbackHandler().lock());
				//数据帧(非控制帧)，携带应用/扩展数据
				//websocket::Message& dataMessage = context.getDataMessage();
				//数据帧(非控制帧)，完整websocket消息头(header)
				//websocket::message_header_t& messageHeader = dataMessage.getMessageHeader();
				//数据帧(非控制帧)，完整websocket消息体(body)
				//IBytesBufferPtr messageBuffer = dataMessage.getMessageBuffer();
				bool enough = true;
				do {
					assert(header.Payloadlen == 127);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadPayloadData/ReadExtendedPayloadDataI64
					//////////////////////////////////////////////////////////////////////////
					assert(context.getWebsocketStep() == websocket::StepE::ReadExtendedPayloadDataI64);
					//读取Payload Data ///
					if (buf->readableBytes() < extended_header.getExtendedPayloadlenI64()) {
						//读取不够
						//parse_uncontrol_frame_ReadExtendedPayloadDataI64[bad][127]: StepE::ReadPayloadData(131072) readableBytes(16370)
						printf("websocket::parse_uncontrol_frame_ReadExtendedPayloadDataI64[bad][%d]: %s(%lld) readableBytes(%d)\n",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), extended_header.getExtendedPayloadlenI64(), buf->readableBytes());
						enough = false;
						break;
					}
					websocket::MessageFrameE messageFrameType = context.getDataMessage().addFrameHeader(extended_header);
					//输出扩展协议头信息
					dump_extended_header_info(extended_header);
					{
						printf("websocket::parse_uncontrol_frame_ReadExtendedPayloadDataI64[ok][%d]: %s(%lld) readableBytes(%d)\n",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), extended_header.getExtendedPayloadlenI64(), buf->readableBytes());

						//读取Payload Data ///
						context.getDataMessage().getMessageBuffer()->append(buf->peek(), extended_header.getExtendedPayloadlenI64());
						buf->retrieve(extended_header.getExtendedPayloadlenI64());
						//////////////////////////////////////////////////////////////////////////
						//对Payload Data做UnMask计算 ///
						context.getDataMessage().unMaskPayloadData(
							header.MASK,
							context.getDataMessage().getMessageBuffer().get(),
							extended_header.getMaskingkey());
#if 0
						//buf没有粘包/半包情况 ///
						//assert(buf->readableBytes() == 0);
#endif
						switch (header.FIN) {
						case FinE::FrameContinue: {
							//帧已结束(分片消息)，非完整消息包，继续从帧头开始解析 //////
							//分片消息连续帧
							//FIN = FrameContinue opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//FIN = FrameContinue opcode = SegmentMessage

							//包不完整不能重置数据帧消息buffer
							//context.resetDataMessage();
							break;
						}
						case FinE::FrameFinished: {
							//帧已结束(未分片/分片消息)，完整消息包，执行消息回调，继续从帧头开始解析 //////
							//未分片消息结束帧 FIN = FrameFinished opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//分片消息结束帧 FIN = FrameFinished opcode = SegmentMessage
							//打印数据帧全部帧头信息
							context.getDataMessage().dumpFrameHeaders("websocket::parse_uncontrol_frame_ReadExtendedPayloadDataI64");
							//打印控制帧全部帧头信息
							context.getControlMessage().dumpFrameHeaders("websocket::parse_control_frame_ReadExtendedPayloadDataI64");
							switch (header.opcode) {
							case OpcodeE::SegmentMessage: {
								//分片消息帧(消息片段)，中间数据帧
								break;
							}
							case OpcodeE::TextMessage: {
								//文本消息帧
								break;
							}
							case OpcodeE::BinaryMessage: {
								//二进制消息帧
								break;
							}
							default:
								assert(false);
								break;
							}
							if (handler) {
								handler->onMessageCallback(
									context.getDataMessage().getMessageBuffer(),
									context.getDataMessage().getMessageType(), receiveTime);
							}
							if (context.getDataMessage().getMessageBuffer()->readableBytes() == extended_header.getExtendedPayloadlenI64()) {
								//context.getDataMessage().getMessageBuffer()没有分片情况 ///
							}
							else {
								//context.getDataMessage().getMessageBuffer()必是分片消息 ///
							}
							//重置数据帧消息buffer
							context.resetDataMessage();
							assert(context.getDataMessage().getMessageBuffer()->readableBytes() == 0);
							break;
						}
						}
						//正处于解析当中的帧头(当前帧头)
						context.resetExtendedHeader();
						//继续从帧头开始解析下一帧
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadFrameHeader
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadFrameHeader);
					}
				} while (0);
				return enough;
			}

			//parse_control_frame_ReadPayloadData
			//@param websocket::Context& 组件内部私有接口
			static bool parse_control_frame_ReadPayloadData(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				websocket::ICallbackPtr handler(context.getCallbackHandler().lock());
				//控制帧，携带应用/扩展数据
				//websocket::Message& controlMessage = context.getControlMessage();
				//控制帧，完整websocket消息头(header)
				//websocket::message_header_t& controlHeader = controlMessage.getMessageHeader();
				//控制帧，完整websocket消息体(body)
				//IBytesBufferPtr controlBuffer = controlMessage.getMessageBuffer();
				bool enough = true;
				do {
					assert(header.Payloadlen < 126);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadPayloadData
					//////////////////////////////////////////////////////////////////////////
					assert(context.getWebsocketStep() == websocket::StepE::ReadPayloadData);
					//读取Payload Data ///
					if (buf->readableBytes() < header.Payloadlen) {
						//读取不够
						printf("websocket::parse_control_frame_ReadPayloadData[bad][%d]: %s(%d) readableBytes(%d)\n",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), header.Payloadlen, buf->readableBytes());
						enough = false;
						break;
					}
					websocket::MessageFrameE messageFrameType = context.getControlMessage().addFrameHeader(extended_header);
					//输出扩展协议头信息
					dump_extended_header_info(extended_header);
					{
						printf("websocket::parse_control_frame_ReadPayloadData[ok][%d]: %s(%d) readableBytes(%d)\n",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), header.Payloadlen, buf->readableBytes());

						//读取Payload Data ///
						context.getControlMessage().getMessageBuffer()->append(buf->peek(), header.Payloadlen);
						buf->retrieve(header.Payloadlen);
						//////////////////////////////////////////////////////////////////////////
						//对Payload Data做UnMask计算 ///
						context.getControlMessage().unMaskPayloadData(
							header.MASK,
							context.getControlMessage().getMessageBuffer().get(),
							extended_header.getMaskingkey());
#if 0
						//buf没有粘包/半包情况 ///
						assert(buf->readableBytes() == 0);
#endif
						switch (header.FIN) {
						case FinE::FrameContinue: {
							//帧已结束(分片消息)，非完整消息包，继续从帧头开始解析 //////
							//分片消息连续帧
							//FIN = FrameContinue opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//FIN = FrameContinue opcode = SegmentMessage

							//包不完整不能重置控制帧消息buffer
							//context.resetControlMessage();
							break;
						}
						case FinE::FrameFinished: {
							//帧已结束(未分片/分片消息)，完整消息包，执行消息回调，继续从帧头开始解析 //////
							//未分片消息结束帧 FIN = FrameFinished opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//分片消息结束帧 FIN = FrameFinished opcode = SegmentMessage
							//打印控制帧全部帧头信息
							context.getControlMessage().dumpFrameHeaders("websocket::parse_control_frame_ReadPayloadData");
							switch (header.opcode) {
							case OpcodeE::CloseMessage: {
								//连接关闭帧
								if (handler) {
#if 0
									//BytesBuffer rspdata;
									pack_unmask_close_frame(&rspdata,
										context.getControlMessage().getMessageBuffer()->peek(),
										context.getControlMessage().getMessageBuffer()->readableBytes());
									handler->sendMessage(&rspdata);
#endif
									handler->onClosedCallback(context.getControlMessage().getMessageBuffer(), receiveTime);
#if 0
									//不再发送数据
									handler->shutdown();
#elif 1
									//直接强制关闭连接
									handler->forceClose();
#else
									//延迟0.2s强制关闭连接
									handler->forceCloseWithDelay(0.2f);
#endif
									//重置数据帧消息buffer
									context.resetDataMessage();
									assert(context.getDataMessage().getMessageBuffer()->readableBytes() == 0);
									//设置连接关闭状态
									context.setWebsocketState(websocket::StateE::kClosed);
								}
								break;
							}
							case OpcodeE::PingMessage: {
								//心跳PING探测帧
								if (handler) {
									handler->onMessageCallback(
										context.getControlMessage().getMessageBuffer(),
										context.getControlMessage().getMessageType(), receiveTime);
								}
								break;
							}
							case OpcodeE::PongMessage: {
								//心跳PONG探测帧
								if (handler) {
									handler->onMessageCallback(
										context.getControlMessage().getMessageBuffer(),
										context.getControlMessage().getMessageType(), receiveTime);
								}
								break;
							}
							default:
								assert(false);
								break;
							}
							if (context.getControlMessage().getMessageBuffer()->readableBytes() == header.Payloadlen) {
								//context.getControlMessage().getMessageBuffer()->readableBytes()没有分片情况 ///
							}
							else {
								//context.getControlMessage().getMessageBuffer()->readableBytes()必是分片消息 ///
							}
							//重置控制帧消息buffer
							context.resetControlMessage();
							assert(context.getControlMessage().getMessageBuffer()->readableBytes() == 0);
							break;
						}
						}
						//正处于解析当中的帧头(当前帧头)
						context.resetExtendedHeader();
						//继续从帧头开始解析下一帧
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadFrameHeader
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadFrameHeader);
					}
				} while (0);
				return enough;
			}

			//parse_control_frame_ReadExtendedPayloadDataU16
			//@param websocket::Context& 组件内部私有接口
			static bool parse_control_frame_ReadExtendedPayloadDataU16(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				websocket::ICallbackPtr handler(context.getCallbackHandler().lock());
				//控制帧，携带应用/扩展数据
				//websocket::Message& controlMessage = context.getControlMessage();
				//控制帧，完整websocket消息头(header)
				//websocket::message_header_t& controlHeader = controlMessage.getMessageHeader();
				//控制帧，完整websocket消息体(body)
				//IBytesBufferPtr controlBuffer = controlMessage.getMessageBuffer();
				bool enough = true;
				do {
					assert(header.Payloadlen == 126);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadPayloadData
					//////////////////////////////////////////////////////////////////////////
					assert(context.getWebsocketStep() == websocket::StepE::ReadPayloadData);
					//读取Payload Data ///
					if (buf->readableBytes() < extended_header.getExtendedPayloadlenU16()) {
						//读取不够
						printf("websocket::parse_control_frame_ReadExtendedPayloadDataU16[bad][%d]: %s(%d) readableBytes(%d)\n",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), extended_header.getExtendedPayloadlenU16(), buf->readableBytes());
						enough = false;
						break;
					}
					websocket::MessageFrameE messageFrameType = context.getControlMessage().addFrameHeader(extended_header);
					//输出扩展协议头信息
					dump_extended_header_info(extended_header);
					{
						printf("websocket::parse_control_frame_ReadExtendedPayloadDataU16[ok][%d]: %s(%d) readableBytes(%d)\n",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), extended_header.getExtendedPayloadlenU16(), buf->readableBytes());

						//读取Payload Data ///
						context.getControlMessage().getMessageBuffer()->append(buf->peek(), extended_header.getExtendedPayloadlenU16());
						buf->retrieve(extended_header.getExtendedPayloadlenU16());
						//////////////////////////////////////////////////////////////////////////
						//对Payload Data做UnMask计算 ///
						context.getControlMessage().unMaskPayloadData(
							header.MASK,
							context.getControlMessage().getMessageBuffer().get(),
							extended_header.getMaskingkey());
#if 0
						//buf没有粘包/半包情况 ///
						assert(buf->readableBytes() == 0);
#endif
						switch (header.FIN) {
						case FinE::FrameContinue: {
							//帧已结束(分片消息)，非完整消息包，继续从帧头开始解析 //////
							//分片消息连续帧
							//FIN = FrameContinue opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//FIN = FrameContinue opcode = SegmentMessage

							//包不完整不能重置控制帧消息buffer
							//context.resetControlMessage();
							break;
						}
						case FinE::FrameFinished: {
							//帧已结束(未分片/分片消息)，完整消息包，执行消息回调，继续从帧头开始解析 //////
							//未分片消息结束帧 FIN = FrameFinished opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//分片消息结束帧 FIN = FrameFinished opcode = SegmentMessage
							//打印控制帧全部帧头信息
							context.getControlMessage().dumpFrameHeaders("websocket::parse_control_frame_ReadExtendedPayloadDataU16");
							switch (header.opcode) {
							case OpcodeE::CloseMessage: {
								//连接关闭帧
								if (handler) {
#if 0
									//BytesBuffer rspdata;
									pack_unmask_close_frame(&rspdata,
										context.getControlMessage().getMessageBuffer()->peek(),
										context.getControlMessage().getMessageBuffer()->readableBytes());
									handler->sendMessage(&rspdata);
#endif
									handler->onClosedCallback(context.getControlMessage().getMessageBuffer(), receiveTime);
#if 0
									//不再发送数据
									handler->shutdown();
#elif 1
									//直接强制关闭连接
									handler->forceClose();
#else
									//延迟0.2s强制关闭连接
									handler->forceCloseWithDelay(0.2f);
#endif
									//重置数据帧消息buffer
									context.resetDataMessage();
									assert(context.getDataMessage().getMessageBuffer()->readableBytes() == 0);
									//设置连接关闭状态
									context.setWebsocketState(websocket::StateE::kClosed);
								}
								break;
							}
							case OpcodeE::PingMessage: {
								//心跳PING探测帧
								if (handler) {
									handler->onMessageCallback(
										context.getControlMessage().getMessageBuffer(),
										context.getControlMessage().getMessageType(), receiveTime);
								}
								break;
							}
							case OpcodeE::PongMessage: {
								//心跳PONG探测帧
								if (handler) {
									handler->onMessageCallback(
										context.getControlMessage().getMessageBuffer(),
										context.getControlMessage().getMessageType(), receiveTime);
								}
								break;
							}
							default:
								assert(false);
								break;
							}
							if (context.getControlMessage().getMessageBuffer()->readableBytes() == extended_header.getExtendedPayloadlenU16()) {
								//context.getControlMessage().getMessageBuffer()没有分片情况 ///
							}
							else {
								//context.getControlMessage().getMessageBuffer()必是分片消息 ///
							}
							//重置控制帧消息buffer
							context.resetControlMessage();
							assert(context.getControlMessage().getMessageBuffer()->readableBytes() == 0);
							break;
						}
						}
						//正处于解析当中的帧头(当前帧头)
						context.resetExtendedHeader();
						//继续从帧头开始解析下一帧
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadFrameHeader
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadFrameHeader);
					}
				} while (0);
				return enough;
			}

			//parse_control_frame_ReadExtendedPayloadDataI64
			//@param websocket::Context& 组件内部私有接口
			static bool parse_control_frame_ReadExtendedPayloadDataI64(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				websocket::ICallbackPtr handler(context.getCallbackHandler().lock());
				//控制帧，携带应用/扩展数据
				//websocket::Message& controlMessage = context.getControlMessage();
				//控制帧，完整websocket消息头(header)
				//websocket::message_header_t& controlHeader = controlMessage.getMessageHeader();
				//控制帧，完整websocket消息体(body)
				//IBytesBufferPtr controlBuffer = controlMessage.getMessageBuffer();
				bool enough = true;
				do {
					assert(header.Payloadlen == 127);
					//////////////////////////////////////////////////////////////////////////
					//StepE::ReadPayloadData/ReadExtendedPayloadDataI64
					//////////////////////////////////////////////////////////////////////////
					assert(context.getWebsocketStep() == websocket::StepE::ReadExtendedPayloadDataI64);
					//读取Payload Data ///
					if (buf->readableBytes() < extended_header.getExtendedPayloadlenI64()) {
						//读取不够
						//parse_control_frame_ReadExtendedPayloadDataI64[bad][127]: StepE::ReadPayloadData(131072) readableBytes(16370)
						printf("websocket::parse_control_frame_ReadExtendedPayloadDataI64[bad][%d]: %s(%lld) readableBytes(%d)\n",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), extended_header.getExtendedPayloadlenI64(), buf->readableBytes());
						enough = false;
						break;
					}
					websocket::MessageFrameE messageFrameType = context.getControlMessage().addFrameHeader(extended_header);
					//输出扩展协议头信息
					dump_extended_header_info(extended_header);
					{
						printf("websocket::parse_control_frame_ReadExtendedPayloadDataI64[ok][%d]: %s(%lld) readableBytes(%d)\n",
							header.Payloadlen,
							websocket::Step_to_string(context.getWebsocketStep()).c_str(), extended_header.getExtendedPayloadlenI64(), buf->readableBytes());

						//读取Payload Data ///
						context.getControlMessage().getMessageBuffer()->append(buf->peek(), extended_header.getExtendedPayloadlenI64());
						buf->retrieve(extended_header.getExtendedPayloadlenI64());
						//////////////////////////////////////////////////////////////////////////
						//对Payload Data做UnMask计算 ///
						context.getControlMessage().unMaskPayloadData(
							header.MASK,
							context.getControlMessage().getMessageBuffer().get(),
							extended_header.getMaskingkey());
#if 0
						//buf没有粘包/半包情况 ///
						//assert(buf->readableBytes() == 0);
#endif
						switch (header.FIN) {
						case FinE::FrameContinue: {
							//帧已结束(分片消息)，非完整消息包，继续从帧头开始解析 //////
							//分片消息连续帧
							//FIN = FrameContinue opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//FIN = FrameContinue opcode = SegmentMessage

							//包不完整不能重置控制帧消息buffer
							//context.resetControlMessage();
							break;
						}
						case FinE::FrameFinished: {
							//帧已结束(未分片/分片消息)，完整消息包，执行消息回调，继续从帧头开始解析 //////
							//未分片消息结束帧 FIN = FrameFinished opcode = TextMessage|BinaryMessage|CloseMessage|PingMessage|PongMessage
							//分片消息结束帧 FIN = FrameFinished opcode = SegmentMessage
							//打印控制帧全部帧头信息
							context.getControlMessage().dumpFrameHeaders("websocket::parse_control_frame_ReadExtendedPayloadDataI64");
							switch (header.opcode) {
							case OpcodeE::CloseMessage: {
								//连接关闭帧
								if (handler) {
#if 0
									//BytesBuffer rspdata;
									pack_unmask_close_frame(&rspdata,
										context.getControlMessage().getMessageBuffer()->peek(),
										context.getControlMessage().getMessageBuffer()->readableBytes());
									handler->sendMessage(&rspdata);
#endif
									handler->onClosedCallback(context.getControlMessage().getMessageBuffer(), receiveTime);
#if 0
									//不再发送数据
									handler->shutdown();
#elif 1
									//直接强制关闭连接
									handler->forceClose();
#else
									//延迟0.2s强制关闭连接
									handler->forceCloseWithDelay(0.2f);
#endif
									//重置数据帧消息buffer
									context.resetDataMessage();
									assert(context.getDataMessage().getMessageBuffer()->readableBytes() == 0);
									//设置连接关闭状态
									context.setWebsocketState(websocket::StateE::kClosed);
								}
								break;
							}
							case OpcodeE::PingMessage: {
								//心跳PING探测帧
								if (handler) {
									handler->onMessageCallback(
										context.getControlMessage().getMessageBuffer(),
										context.getControlMessage().getMessageType(), receiveTime);
								}
								break;
							}
							case OpcodeE::PongMessage: {
								//心跳PONG探测帧
								if (handler) {
									handler->onMessageCallback(
										context.getControlMessage().getMessageBuffer(),
										context.getControlMessage().getMessageType(), receiveTime);
								}
								break;
							}
							default:
								assert(false);
								break;
							}
							if (context.getControlMessage().getMessageBuffer()->readableBytes() == extended_header.getExtendedPayloadlenI64()) {
								//context.getControlMessage().getMessageBuffer()没有分片情况 ///
							}
							else {
								//context.getControlMessage().getMessageBuffer()必是分片消息 ///
							}
							//重置控制帧消息buffer
							context.resetControlMessage();
							assert(context.getControlMessage().getMessageBuffer()->readableBytes() == 0);
							break;
						}
						}
						//正处于解析当中的帧头(当前帧头)
						context.resetExtendedHeader();
						//继续从帧头开始解析下一帧
						//////////////////////////////////////////////////////////////////////////
						//StepE::ReadFrameHeader
						//////////////////////////////////////////////////////////////////////////
						context.setWebsocketStep(websocket::StepE::ReadFrameHeader);
					}
				} while (0);
				return enough;
			}

			//parse_frame
			//@param websocket::Context& 组件内部私有接口
			static int parse_frame(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* savedErrno) {
				//websocket::extended_header_t 正处于解析当中的帧头(当前帧头)
				websocket::extended_header_t& extended_header = context.getExtendedHeader();
				//websocket::header_t，uint16_t
				websocket::header_t& header = extended_header.get_header();
				websocket::ICallbackPtr handler(context.getCallbackHandler().lock());
				//数据帧(非控制帧)，携带应用/扩展数据
				websocket::Message& dataMessage = context.getDataMessage();
				//数据帧(非控制帧)，完整websocket消息头(header)
				websocket::message_header_t& messageHeader = dataMessage.getMessageHeader();
				//数据帧(非控制帧)，完整websocket消息体(body)
				IBytesBufferPtr messageBuffer = dataMessage.getMessageBuffer();
				assert(messageBuffer);
				//控制帧，Payload len<=126字节，且不能被分片
				websocket::Message& controlMessage = context.getControlMessage();
				//控制帧，完整websocket消息头(header)
				//websocket::message_header_t& controlHeader = controlMessage.getMessageHeader();
				//控制帧，完整websocket消息体(body)
				IBytesBufferPtr controlBuffer = controlMessage.getMessageBuffer();
				assert(controlBuffer);
				std::pair<FrameControlE, MessageFrameE> ty = get_frame_control_message_type(header);
				bool enough = true;
				//先解析包头(header)，再解析包体(body)
				int i = 0;
				//关闭消息帧 PayloadDatalen == 0/buf->readableBytes() == 0
				while (enough && buf->readableBytes() > 0) {
					//websocket::parse_frame loop[1] StepE::ReadFrameHeader(0) readableBytes(6)
					//websocket::parse_frame loop[2] StepE::ReadMaskingkey(0) readableBytes(4)
					printf("websocket::parse_frame loop[%d] %s(%lld) readableBytes(%d)\n",
						++i,
						websocket::Step_to_string(context.getWebsocketStep()).c_str(), extended_header.getRealDatalen(), buf->readableBytes());
					//消息流解析步骤step
					websocket::StepE step = context.getWebsocketStep();
					switch (step) {
					case StepE::ReadFrameHeader: {
						//解析基础协议头/帧头(header) header_t，uint16_t
						//输出基础协议头信息
						//消息帧有效性安全检查
						//更新帧体(body)消息流解析步骤step
						enough = parse_frame_ReadFrameHeader(context, buf, receiveTime, savedErrno);
						break;
					}
					case StepE::ReadExtendedPayloadlen2Byte: {
						enough = parse_frame_ReadExtendedPayloadlen2Byte(context, buf, receiveTime, savedErrno);
						break;
					}
					case StepE::ReadExtendedPayloadlen8Byte: {
						enough = parse_frame_ReadExtendedPayloadlen8Byte(context, buf, receiveTime, savedErrno);
						break;
					}
					case StepE::ReadMaskingkey: {
						enough = parse_frame_ReadMaskingkey(context, buf, receiveTime, savedErrno);
						break;
					}
					case StepE::ReadPayloadData: {
						assert(ty.first != FrameControlE::FrameInvalid);
						enough = (ty.first == FrameControlE::UnControlFrame) ?
							parse_uncontrol_frame_ReadPayloadData(context, buf, receiveTime, savedErrno):
							parse_control_frame_ReadPayloadData(context, buf, receiveTime, savedErrno);
						break;
					}
					case StepE::ReadExtendedPayloadDataU16:
						assert(ty.first != FrameControlE::FrameInvalid);
						enough = (ty.first == FrameControlE::UnControlFrame) ?
							parse_uncontrol_frame_ReadExtendedPayloadDataU16(context, buf, receiveTime, savedErrno) :
							parse_control_frame_ReadExtendedPayloadDataU16(context, buf, receiveTime, savedErrno);
						break;
					case StepE::ReadExtendedPayloadDataI64:
						assert(ty.first != FrameControlE::FrameInvalid);
						enough = (ty.first == FrameControlE::UnControlFrame) ?
							parse_uncontrol_frame_ReadExtendedPayloadDataI64(context, buf, receiveTime, savedErrno) :
							parse_control_frame_ReadExtendedPayloadDataI64(context, buf, receiveTime, savedErrno);
						break;
					default:
						assert(false);
						break;
					}
				}
				return 0;
			}
			
			/* websocket 握手协议
			*
			* 握手请求 GET ///
			* GET / HTTP/1.1
			* Host: 192.168.2.93:10000
			* Connection: Upgrade
			* Pragma: no-cache
			* Cache-Control: no-cache
			* User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.122 Safari/537.36
			* Upgrade: websocket
			* Origin: http://localhost:7456
			* Sec-WebSocket-Protocol: chat, superchat
			* Sec-WebSocket-Version: 13
			* Accept-Encoding: gzip, deflate, br
			* Accept-Language: zh-CN,zh;q=0.9
			* Sec-WebSocket-Key: ylPFmimkYxdg/eh968/lHQ==
			* Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits

			* 握手成功 SHA-1 Base64 ///
			* HTTP/1.1 101 Switching Protocols
			* Connection: Upgrade
			* Content-Type: application/octet-stream
			* Upgrade: websocket
			* Sec-WebSocket-Version: 13
			* Sec-WebSocket-Accept: RnZfgbNALFZVuKAtOjstw6SacYU=
			* Sec-WebSocket-Protocol: chat, superchat
			* Server: TXQP
			* Timing-Allow-Origin: *
			* Date: Sat, 29 Feb 2020 07:07:34 GMT
			*
			*/
			//create_websocket_response 填充websocket握手成功响应头信息
			static void create_websocket_response(http::IRequest const* req, std::string& rsp) {
				rsp = "HTTP/1.1 101 Switching Protocols\r\n";
				rsp += "Connection: Upgrade\r\n";
				rsp += "Content-Type: application/octet-stream\r\n";
				rsp += "Upgrade: websocket\r\n";
				rsp += "Sec-WebSocket-Version: 13\r\n";
 				//rsp += "Sec-WebSocket-Extensions: " + context.request().getHeader("Sec-WebSocket-Extensions") + "\r\n";
				//rsp += "Sec-WebSocket-Extensions: x-webkit-deflate-frame\r\n";
				//rsp += "Sec-WebSocket-Extensions: permessage-deflate\r\n";
				//rsp += "Sec-WebSocket-Extensions: deflate-stream\r\n";
				rsp += "Sec-WebSocket-Accept: ";
				const std::string SecWebSocketKey = req->getHeader("Sec-WebSocket-Key");
				std::string serverKey = getAcceptKey(SecWebSocketKey);
				rsp += serverKey + "\r\n";
				rsp += "Server: TXQP\r\n";
				rsp += "Timing-Allow-Origin: *\r\n";
				rsp += "Date: " + webtime_now() + "\r\n\r\n";
			}

			//do_handshake
			//@param websocket::Context& 组件内部私有接口
			static bool do_handshake(
				websocket::Context& context,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime, int* saveErrno) {
				websocket::ICallbackPtr handler(context.getCallbackHandler().lock());
				do {
					//先确定是HTTP数据报文，再解析 ///
					//assert(buf->readableBytes() > 4 && buf->findCRLFCRLF());

					//数据包太小 ///
					if (buf->readableBytes() <= 4) {
						*saveErrno = HandShakeE::WS_ERROR_WANT_MORE;
						printf("-----------------------------------------------------------------------------\n");
						printf("websocket::do_handshake WS_ERROR_WANT_MORE\n");
						break;
					}
					//数据包太大 ///
					else if (buf->readableBytes() > 1024) {
						*saveErrno = HandShakeE::WS_ERROR_PACKSZ;
						printf("-----------------------------------------------------------------------------\n");
						printf("websocket::do_handshake WS_ERROR_PACKSZ\n");
						break;
					}
					//查找\r\n\r\n ///
					const char* crlfcrlf = buf->findCRLFCRLF();
					if (!crlfcrlf) {
						*saveErrno = HandShakeE::WS_ERROR_CRLFCRLF;
						printf("-----------------------------------------------------------------------------\n");
						printf("websocket::do_handshake WS_ERROR_CRLFCRLF\n");
						break;
					}

					//LOG_ERROR << __FUNCTION__ << " --- *** ";
					printf("----------------------------------------------\n");
					printf("size = %d %.*s\n", buf->readableBytes(), buf->readableBytes(), buf->peek());
					//http::IContext ///
					http::IContextPtr httpContext(context.getHttpContext().lock());
					assert(httpContext);
					if (!httpContext->parseRequestPtr(buf, receiveTime)) {
						//发生错误
						*saveErrno = HandShakeE::WS_ERROR_PARSE;
						printf("-----------------------------------------------------------------------------\n");
						printf("websocket::do_handshake WS_ERROR_PARSE\n");
						break;
					}
					else if (httpContext->gotAll()) {
						std::string ipaddr;
						std::string rspdata;
						//填充websocket握手成功响应头信息 ///
						create_websocket_response(httpContext->requestConstPtr(), rspdata);
						std::string ipaddrs = httpContext->requestPtr()->getHeader("X-Forwarded-For");
						if (ipaddrs.empty()) {
							if (handler) {
								ipaddr = handler->peerIpAddrToString();
							}
						}
						else {
#if 0
							//第一个IP为客户端真实IP，可伪装，第二个IP为一级代理IP，第三个IP为二级代理IP
							std::string::size_type spos = ipaddrs.find_first_of(',');
							if (spos == std::string::npos) {
							}
							else {
								ipaddr = ipaddrs.substr(0, spos);
							}
#else
							boost::replace_all<std::string>(ipaddrs, " ", "");
							std::vector<std::string> vec;
							boost::algorithm::split(vec, ipaddrs, boost::is_any_of(","));
							for (std::vector<std::string>::const_iterator it = vec.begin();
								it != vec.end(); ++it) {
								if (!it->empty() /*&&
									boost::regex_match(*it, boost::regex(
										"^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\." \
										"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
										"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\." \
										"(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$"))*/) {

									if (strncasecmp(it->c_str(), "10.", 3) != 0 &&
										strncasecmp(it->c_str(), "192.168", 7) != 0 &&
										strncasecmp(it->c_str(), "172.16.", 7) != 0) {
										ipaddr = *it;
										break;
									}
								}
							}
#endif
						}
						assert(!buf->readableBytes());
						buf->retrieveAll();
						//设置连接建立状态
						context.setWebsocketState(websocket::StateE::kConnected);
						if (handler) {
							handler->sendMessage(rspdata);
							handler->onConnectedCallback(ipaddr);
						}
						//释放httpContext资源 ///
						httpContext.reset();
						//握手成功 ///
						printf("-----------------------------------------------------------------\n");
						printf("websocket::do_handshake succ\n");
						return true;
					}
				} while (0);
				switch (*saveErrno)
				{
				case HandShakeE::WS_ERROR_PARSE:
				case HandShakeE::WS_ERROR_PACKSZ:
					//握手失败 ///
					printf("-----------------------------------------------------------------\n");
					printf("websocket::do_handshake failed\n");
					break;
				}
				return false;
			}

			//parse_message_frame
			//@param WeakIContextPtr const& "websocket context"
			int parse_message_frame(
				WeakIContextPtr const& weakContext,
				IBytesBuffer /*const*/* buf,
				ITimestamp* receiveTime) {
				websocket::IContextPtr ctx(weakContext.lock());
				if (ctx) {
					//assert(context_ && dynamic_cast<libwebsocket::Context*>(ctx.get());
					websocket::Context* pctx = reinterpret_cast<websocket::Context*>(ctx.get());
					websocket::Context& context = *pctx;
					int saveErrno = 0;
					if (context.getWebsocketState() == StateE::kClosed) {
						//do_handshake 握手
						bool wsConnected = websocket::do_handshake(context, buf, receiveTime, &saveErrno);
						switch (saveErrno)
						{
						case WS_ERROR_WANT_MORE:
						case WS_ERROR_CRLFCRLF:
							break;
						case WS_ERROR_PARSE:
						case WS_ERROR_PACKSZ: {
							websocket::ICallbackPtr handler(context.getCallbackHandler().lock());
							if (handler) {
								handler->sendMessage("HTTP/1.1 400 Bad Request\r\n\r\n");
								handler->shutdown();
							}
							break;
						}
						case 0:
							assert(wsConnected);
							//succ
							break;
						}
					}
					else {
						assert(context.getWebsocketState() == StateE::kConnected);
						//parse_frame 解析帧
						websocket::parse_frame(context, buf, receiveTime, &saveErrno);
					}
					return saveErrno;
				}
				return HandShakeE::WS_ERROR_CONTEXT;
			}
			
			void websocket_test_demo() {
				websocket::header_t h = { 0 };
#if 0
				h.FIN = 0;
				h.RSV123 = 1;
				h.opcode = 1;
				h.MASK = 0;
				h.Payloadlen = 1;
				unsigned short x;
				memcpy(&x, &h, 2);
				printf("%#x\n", x);
#else
				int x = 0x212;
				memcpy(&h, &x, 2);
				printf("FIN = %d\nRSV1 = %d\nRSV2 = %d\nRSV3 = %d\nopcode = %d\nMASK = %d\nPayloadlen=%d\n",
					h.FIN, h.RSV1, h.RSV2, h.RSV3, h.opcode, h.MASK, h.Payloadlen);
#endif
			}

		}//namespace websocket
	}//namespace net
}//namespace muduo