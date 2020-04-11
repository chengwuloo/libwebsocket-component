#include "packet.h"
#include <assert.h>

namespace packet {

	//pack data[len] to buffer with packet::header_t
	void packMessage(muduo::net::Buffer* buffer, int mainID, int subID, char const* data, size_t len) {
		assert(buffer->writableBytes() >= packet::kHeaderLen + len);
		//buffer[packet::kHeaderLen]
		if (len > 0) {
			assert(data);
			memcpy(buffer->beginWrite() + packet::kHeaderLen, data, len);
		}
		{
			//命令消息头header_t
			packet::header_t* header = (packet::header_t*)buffer->beginWrite();
			header->len = packet::kHeaderLen + len;
			header->ver = 1;
			header->sign = HEADER_SIGN;
			header->mainID = mainID;
			header->subID = subID;
			header->enctype = packet::enctypeE::PUBENC_PROTOBUF_NONE;
			header->reserved = 0;
			header->reqID = 0;
			header->realsize = len;
			//CRC校验位 header->len = packet::kHeaderLen + len
			//header.len uint16_t
			//header.crc uint16_t
			//header.ver ~ header.realsize + protobuf
			header->crc = packet::getCheckSum((uint8_t const*)&header->ver, header->len - 4);
		}
		buffer->hasWritten(packet::kHeaderLen + len);
	}
	//pack data[len] to buffer with packet::header_t
	BufferPtr packMessage(int mainID, int subID, char const* data, size_t len) {
		//命令消息头header_t + len
		BufferPtr buffer(new muduo::net::Buffer(packet::kHeaderLen + len));
		packMessage(buffer.get(), mainID, subID, data, len);
		return buffer;
	}

	//pack protobuf to buffer with packet::header_t
	bool packMessage(muduo::net::Buffer* buffer, int mainID, int subID, ::google::protobuf::Message* data) {
		//protobuf
		size_t len = data ? data->ByteSizeLong() : 0;
		assert(buffer->writableBytes() >= packet::kHeaderLen + len);
		//buffer[packet::kHeaderLen]
		if (len > 0) {
			assert(data);
			if (!data->SerializeToArray(buffer->beginWrite() + packet::kHeaderLen, len)) {
				return false;
			}
		}
		{
			//命令消息头header_t
			packet::header_t* header = (packet::header_t*)buffer->beginWrite();
			header->len = packet::kHeaderLen + len;
			header->ver = 1;
			header->sign = HEADER_SIGN;
			header->mainID = mainID;
			header->subID = subID;
			header->enctype = packet::enctypeE::PUBENC_PROTOBUF_NONE;
			header->reserved = 0;
			header->reqID = 0;
			header->realsize = len;
			//CRC校验位 header->len = packet::kHeaderLen + len
			//header.len uint16_t
			//header.crc uint16_t
			//header.ver ~ header.realsize + protobuf
			header->crc = packet::getCheckSum((uint8_t const*)&header->ver, header->len - 4);
		}
		buffer->hasWritten(packet::kHeaderLen + len);
		return true;
	}
	//pack protobuf to buffer with packet::header_t
	BufferPtr packMessage(int mainID, int subID, ::google::protobuf::Message* data) {
		//protobuf
		size_t len = data ? data->ByteSizeLong() : 0;
		//命令消息头header_t + len
		BufferPtr buffer(new muduo::net::Buffer(packet::kHeaderLen + len));
		if (!packMessage(buffer.get(), mainID, subID, data)) {
			buffer.reset();
			return buffer;
		}
		return buffer;
	}

	//pack data[len] to buffer with packet::internal_prev_header_t
	void packMessage(
		muduo::net::Buffer* buffer,
		int64_t userid,
		std::string const& session,
		std::string const& aeskey,
		uint32_t clientip,
		int16_t kicking,
#if 0
		std::string const& servid,
#endif
		char const* data, size_t len) {
		assert(len >= packet::kHeaderLen);
		assert(data);
		assert(buffer->writableBytes() >= packet::kPrevHeaderLen + len);
		//buffer[packet::kPrevHeaderLen]
		memcpy(buffer->beginWrite() + packet::kPrevHeaderLen, data, len);
		{
			//内部消息头internal_prev_header_t
			packet::internal_prev_header_t* pre_header = (packet::internal_prev_header_t*)buffer->beginWrite();
			memset(pre_header, 0, packet::kPrevHeaderLen);
			pre_header->len = packet::kPrevHeaderLen + len;
			//kicking
			pre_header->kicking = kicking;
			//userid
			pre_header->userID = userid;
			//clientip
			pre_header->ipaddr = clientip;
			//session
			assert(session.length() <= packet::kSessionSZ);
			memcpy(pre_header->session, session.c_str(), std::min(packet::kSessionSZ, session.length()));
			//aeskey
			assert(aeskey.length() <= packet::kAesKeySZ);
			memcpy(pre_header->aeskey, aeskey.c_str(), std::min(packet::kAesKeySZ, aeskey.length()));
#if 0
			//servid
			assert(servid.length() <= packet::kServIDSZ);
			memcpy(pre_header->servID, servid.c_str(), std::min(packet::kServIDSZ, servid.length()));
#endif
			//checksum
			packet::setCheckSum(pre_header);
		}
		buffer->hasWritten(packet::kPrevHeaderLen + len);
	}
	//pack data[len] to buffer with packet::internal_prev_header_t
	BufferPtr packMessage(
		int64_t userid,
		std::string const& session,
		std::string const& aeskey,
		uint32_t clientip,
		int16_t kicking,
#if 0
		std::string const& servid,
#endif
		char const* data, size_t len) {
		//内部消息头internal_prev_header_t + len
		BufferPtr buffer(new muduo::net::Buffer(packet::kPrevHeaderLen + len));
		packMessage(buffer.get(), userid, session, aeskey, clientip, kicking,
#if 0
			servid,
#endif
			data, len);
		return buffer;
	}

	//pack data[len] to buffer with packet::internal_prev_header_t & packet::header_t
	void packMessage(
		muduo::net::Buffer* buffer,
		int64_t userid,
		std::string const& session,
		std::string const& aeskey,
		uint32_t clientip,
		int16_t kicking,
#if 0
		std::string const& servid,
#endif
		int mainID, int subID,
		char const* data, size_t len) {
		assert(buffer->writableBytes() >= packet::kPrevHeaderLen + packet::kHeaderLen + len);
		//buffer[packet::kPrevHeaderLen + packet::kHeaderLen]
		if (len > 0) {
			assert(data);
			memcpy(buffer->beginWrite() + packet::kPrevHeaderLen + packet::kHeaderLen, data, len);
		}
		{
			//内部消息头internal_prev_header_t
			packet::internal_prev_header_t* pre_header = (packet::internal_prev_header_t*)buffer->beginWrite();
			memset(pre_header, 0, packet::kPrevHeaderLen + packet::kHeaderLen);
			pre_header->len = packet::kPrevHeaderLen + packet::kHeaderLen + len;
			//kicking
			pre_header->kicking = kicking;
			//userid
			pre_header->userID = userid;
			//clientip
			pre_header->ipaddr = clientip;
			//session
			assert(session.length() <= packet::kSessionSZ);
			memcpy(pre_header->session, session.c_str(), std::min(packet::kSessionSZ, session.length()));
			//aeskey
			assert(aeskey.length() <= packet::kAesKeySZ);
			memcpy(pre_header->aeskey, aeskey.c_str(), std::min(packet::kAesKeySZ, aeskey.length()));
#if 0
			//servid
			assert(servid.length() <= packet::kServIDSZ);
			memcpy(pre_header->servID, servid.c_str(), std::min(packet::kServIDSZ, servid.length()));
#endif
			//checksum
			packet::setCheckSum(pre_header);
		}
		{
			//命令消息头header_t
			packet::header_t* header = (packet::header_t*)buffer->beginWrite() + packet::kPrevHeaderLen;
			header->len = packet::kHeaderLen + len;
			header->ver = 1;
			header->sign = HEADER_SIGN;
			header->mainID = mainID;
			header->subID = subID;
			header->enctype = packet::enctypeE::PUBENC_PROTOBUF_NONE;
			header->reserved = 0;
			header->reqID = 0;
			header->realsize = len;
			//CRC校验位 header->len = packet::kHeaderLen + len
			//header.len uint16_t
			//header.crc uint16_t
			//header.ver ~ header.realsize + protobuf
			header->crc = packet::getCheckSum((uint8_t const*)&header->ver, header->len - 4);
		}
		buffer->hasWritten(packet::kPrevHeaderLen + packet::kHeaderLen + len);
	}
	//pack data[len] to buffer with packet::internal_prev_header_t & packet::header_t
	BufferPtr packMessage(
		int64_t userid,
		std::string const& session,
		std::string const& aeskey,
		uint32_t clientip,
		int16_t kicking,
#if 0
		std::string const& servid,
#endif
		int mainID, int subID,
		char const* data, size_t len) {
		//内部消息头internal_prev_header_t + 命令消息头header_t + len
		BufferPtr buffer(new muduo::net::Buffer(packet::kPrevHeaderLen + packet::kHeaderLen + len));
		packMessage(buffer.get(), userid, session, aeskey, clientip, kicking,
#if 0
			servid,
#endif
			mainID, subID, data, len);
		return buffer;
	}

	//pack protobuf to buffer with packet::internal_prev_header_t & packet::header_t
	bool packMessage(
		muduo::net::Buffer* buffer,
		int64_t userid,
		std::string const& session,
		std::string const& aeskey,
		uint32_t clientip,
		int16_t kicking,
#if 0
		std::string const& servid,
#endif
		int mainID, int subID,
		::google::protobuf::Message* data) {
		//protobuf
		size_t len = data ? data->ByteSizeLong() : 0;
		assert(buffer->writableBytes() >= packet::kPrevHeaderLen + packet::kHeaderLen + len);
		//buffer[packet::kPrevHeaderLen + packet::kHeaderLen]
		if (len > 0) {
			assert(data);
			if (!data->SerializeToArray(buffer->beginWrite() + packet::kPrevHeaderLen + packet::kHeaderLen, len)) {
				return false;
			}
		}
		{
			//内部消息头internal_prev_header_t
			packet::internal_prev_header_t* pre_header = (packet::internal_prev_header_t*)buffer->beginWrite();
			memset(pre_header, 0, packet::kPrevHeaderLen + packet::kHeaderLen);
			pre_header->len = packet::kPrevHeaderLen + packet::kHeaderLen + len;
			//kicking
			pre_header->kicking = kicking;
			//userid
			pre_header->userID = userid;
			//clientip
			pre_header->ipaddr = clientip;
			//session
			assert(session.length() <= packet::kSessionSZ);
			memcpy(pre_header->session, session.c_str(), std::min(packet::kSessionSZ, session.length()));
			//aeskey
			assert(aeskey.length() <= packet::kAesKeySZ);
			memcpy(pre_header->aeskey, aeskey.c_str(), std::min(packet::kAesKeySZ, aeskey.length()));
#if 0
			//servid
			assert(servid.length() <= packet::kServIDSZ);
			memcpy(pre_header->servID, servid.c_str(), std::min(packet::kServIDSZ, servid.length()));
#endif
			//checksum
			packet::setCheckSum(pre_header);
		}
		{
			//命令消息头header_t
			packet::header_t* header = (packet::header_t*)buffer->beginWrite() + packet::kPrevHeaderLen;
			header->len = packet::kHeaderLen + len;
			header->ver = 1;
			header->sign = HEADER_SIGN;
			header->mainID = mainID;
			header->subID = subID;
			header->enctype = packet::enctypeE::PUBENC_PROTOBUF_NONE;
			header->reserved = 0;
			header->reqID = 0;
			header->realsize = len;
			//CRC校验位 header->len = packet::kHeaderLen + len
			//header.len uint16_t
			//header.crc uint16_t
			//header.ver ~ header.realsize + protobuf
			header->crc = packet::getCheckSum((uint8_t const*)&header->ver, header->len - 4);
		}
		buffer->hasWritten(packet::kPrevHeaderLen + packet::kHeaderLen + len);
		return true;
	}
	//pack protobuf to buffer with packet::internal_prev_header_t & packet::header_t
	BufferPtr packMessage(
		int64_t userid,
		std::string const& session,
		std::string const& aeskey,
		uint32_t clientip,
		int16_t kicking,
#if 0
		std::string const& servid,
#endif
		int mainID, int subID,
		::google::protobuf::Message* data) {
		//protobuf
		size_t len = data ? data->ByteSizeLong() : 0;
		//内部消息头internal_prev_header_t + 命令消息头header_t + len
		BufferPtr buffer(new muduo::net::Buffer(packet::kPrevHeaderLen + packet::kHeaderLen + len));
		if (!packMessage(buffer.get(), userid, session, aeskey, clientip, kicking,
#if 0
			servid,
#endif
			mainID, subID, data)) {
			buffer.reset();
			return buffer;
		}
		return buffer;
	}

}//namespace packet
