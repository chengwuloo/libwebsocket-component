/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.03.2020                                           */
/************************************************************************/
#ifndef _MUDUO_NET_IBYTESBUFFER_H_
#define _MUDUO_NET_IBYTESBUFFER_H_

#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <stdint.h>
#include <endian.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>  // ssize_t
#include <sys/types.h>
#include <muduo/net/libwebsocket/base.h>
#include <memory>

namespace muduo {
	namespace net {
		///
		/// @code
		/// +-------------------+------------------+------------------+
		/// | prependable bytes |  readable bytes  |  writable bytes  |
		/// |                   |     (CONTENT)    |                  |
		/// +-------------------+------------------+------------------+
		/// |                   |                  |                  |
		/// 0      <=      readerIndex   <=   writerIndex    <=     size
		/// @endcode
		//
		//@@ IBytesBuffer
		class IBytesBuffer {
		public:
			virtual size_t readableBytes() const = 0;
			virtual size_t writableBytes() const = 0;
			virtual size_t prependableBytes() const = 0;
			virtual const char* peek() const = 0;
			virtual const char* findCRLFCRLF() const = 0;
			virtual const char* findCRLF() const = 0;
			virtual const char* findCRLF(const char* start) const = 0;
			virtual const char* findEOL() const = 0;
			virtual const char* findEOL(const char* start) const = 0;
			virtual void retrieve(size_t len) = 0;
			virtual void retrieveUntil(const char* end) = 0;
			virtual void retrieveInt64() = 0;
			virtual void retrieveInt32() = 0;
			virtual void retrieveInt16() = 0;
			virtual void retrieveInt8() = 0;
			virtual void retrieveAll() = 0;
			virtual std::string retrieveAllAsString() = 0;
			virtual std::string retrieveAsString(size_t len) = 0;
			virtual void append(const char* /*restrict*/ data, size_t len) = 0;
			virtual void append(const void* /*restrict*/ data, size_t len) = 0;
			virtual void ensureWritableBytes(size_t len) = 0;
			virtual char* beginWrite() = 0;
			virtual const char* beginWrite() const = 0;
			virtual void hasWritten(size_t len) = 0;
			virtual void unwrite(size_t len) = 0;
			virtual void appendInt64(int64_t x) = 0;
			virtual void appendInt32(int32_t x) = 0;
			virtual void appendInt16(int16_t x) = 0;
			virtual void appendInt8(int8_t x) = 0;
			virtual int64_t readInt64() = 0;
			virtual int32_t readInt32() = 0;
			virtual int16_t readInt16() = 0;
			virtual int8_t readInt8() = 0;
			virtual int64_t peekInt64() const = 0;
			virtual int32_t peekInt32() const = 0;
			virtual int16_t peekInt16() const = 0;
			virtual int8_t peekInt8() const = 0;
			virtual void prependInt64(int64_t x) = 0;
			virtual void prependInt32(int32_t x) = 0;
			virtual void prependInt16(int16_t x) = 0;
			virtual void prependInt8(int8_t x) = 0;
			virtual void prepend(const void* /*restrict*/ data, size_t len) = 0;
			virtual void shrink(size_t reserve) = 0;
			virtual size_t internalCapacity() const = 0;
			virtual ssize_t readFd(int fd, int* savedErrno) = 0;

			//readFull for EPOLLET
			static ssize_t readFull(int sockfd, IBytesBuffer* buf, int* savedErrno);

			//writeFull for EPOLLET
			static ssize_t writeFull(int sockfd, void const* data, size_t len, int* savedErrno);
		};

		//@@
		typedef std::unique_ptr<IBytesBuffer> IBytesBufferPtr;

	}  // namespace net
}  // namespace muduo

#endif
