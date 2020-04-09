#ifndef PACKET_INCLUDE_H
#define PACKET_INCLUDE_H

#include <stdint.h>
#include "global.h"

#define SESSIONSZ  32
#define AESKEYSZ   16
#define SERVIDSZ   50

#define HEADER_SIGN          (0x5F5F)
#define PROTO_BUF_SIGN       (0xF5F5F5F5)

namespace packet {

#pragma pack(1)

	//@@ enctypeE 加密类型
	enum enctypeE {
		PUBENC_JSON_NONE         = 0x01,
		PUBENC_PROTOBUF_NONE     = 0x02,
		PUBENC_JSON_BIT_MASK     = 0x11,
		PUBENC_PROTOBUF_BIT_MASK = 0x12,
		PUBENC_JSON_RSA          = 0x21,
		PUBENC_PROTOBUF_RSA      = 0x22,
		PUBENC_JSON_AES          = 0x31,
		PUBENC_PROTOBUF_AES      = 0x32,
	};

	//@@ header_t 数据包头
	struct header_t {
		uint16_t len;      //包总长度
		uint16_t crc;      //CRC校验位
		uint16_t ver;      //版本号
		uint16_t sign;     //签名
		uint8_t  mainID;   //主消息mainID
		uint8_t  subID;    //子消息subID
		uint8_t  enctype;  //加密类型
		uint8_t  reserved; //预留
		uint32_t reqID;
		uint16_t realsize; //用户数据长度
	};

	//@@ internal_prev_header_t 数据包头(内部使用)
	struct internal_prev_header_t {
		uint16_t len;
		int16_t  kicking;
		int32_t  ok;
		int64_t  userID;
		uint32_t ipaddr;             //真实IP
		uint8_t  session[SESSIONSZ]; //用户会话
		uint8_t  aesKey[AESKEYSZ];   //AES_KEY
		uint8_t  servID[SERVIDSZ];   //来自节点ID
		uint16_t checksum;           //校验和CHKSUM
	};

#pragma pack()

	//@@
	static const size_t kHeaderLen = sizeof(header_t);
	static const size_t kPrevHeaderLen = sizeof(internal_prev_header_t);
	static const size_t kMaxPacketSZ = 60 * 1024;
	static const size_t kMinPacketSZ = sizeof(int16_t);
	
	//enword
	static inline int enword(int mainID, int subID) {
		return ((0xFF & mainID) << 8) | (0xFF & subID);
	}
	
	//deword
	static inline void deword(int cmd, int& mainID, int& subID) {
		mainID = (0xFF & (cmd >> 8));
		subID = (0xFF & cmd);
	}

	//getCheckSum 计算校验和
	static uint16_t getCheckSum(uint8_t const* header, size_t size) {
		uint16_t sum = 0;
		uint16_t const* ptr = (uint16_t const*)header;
		for (size_t i = 0; i < size / 2; ++i) {
			//读取uint16，2字节
			sum += *ptr++;
		}
		if (size % 2) {
			//读取uint8，1字节
			sum += *(uint8_t const*)ptr;
		}
		return sum;
	}

	//setCheckSum 计算校验和
	static void setCheckSum(internal_prev_header_t* header) {
		uint16_t sum = 0;
		uint16_t* ptr = (uint16_t*)header;
		for (size_t i = 0; i < kPrevHeaderLen / 2 - 1; ++i) {
			//读取uint16，2字节
			sum += *ptr++;
		}
		//CRC校验位
		*ptr = sum;
	}

	//checkCheckSum 计算校验和
	static bool checkCheckSum(internal_prev_header_t const* header) {
		uint16_t sum = 0;
		uint16_t const* ptr = (uint16_t const*)header;
		for (size_t i = 0; i < kPrevHeaderLen / 2 - 1; ++i) {
			//读取uint16，2字节
			sum += *ptr++;
		}
		//校验CRC
		return *ptr == sum;
	}

}//namespace packet

#endif