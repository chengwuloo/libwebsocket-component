/************************************************************************/
/*    @author create by andy_ro@qq.com/Qwuloo@qq.com                    */
/*    @Date		   03.03.2020                                           */
/************************************************************************/
#include "libwebsocket/IBytesBuffer.h"

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/uio.h>
#include <stdint.h>
#include <string.h>  // memset
#include <string>
#include <assert.h>
#include <arpa/inet.h>

static inline void memZero(void* p, size_t n)
{
	memset(p, 0, n);
}

//implicit_cast<ToType>(expr)
template<typename To, typename From>
inline To implicit_cast(From const& f)
{
	return f;
}

// use like this: down_cast<T*>(foo);
// so we only accept pointers
template<typename To, typename From>     
inline To down_cast(From* f)                     
{
	if (false)
	{
		implicit_cast<From*, To>(0);
	}
	assert(f == NULL || dynamic_cast<To>(f) != NULL);
	return static_cast<To>(f);
}

namespace muduo {
	namespace net {
		namespace ssl {

			///
			/// @code
			/// +-------------------+------------------+------------------+
			/// | prependable bytes |  readable bytes  |  writable bytes  |
			/// |                   |     (CONTENT)    |                  |
			/// +-------------------+------------------+------------------+
			/// |                   |                  |                  |
			/// 0      <=      readerIndex   <=   writerIndex    <=     size
			/// @endcode
			///
			//readFull for EPOLLET
			ssize_t readFull(int sockfd, IBytesBuffer* buf, int* savedErrno) {
				assert(buf->writableBytes() >= 0);
				//make sure that writable > 0
				if (buf->writableBytes() == 0) {
					buf->ensureWritableBytes(implicit_cast<size_t>(4096));
				}
				//printf("\nIBytesBuffer::readFull begin {{{\n");
				ssize_t n = 0;
				do {
#if 0 //test
					const size_t writable = 5;
#else
					const size_t writable = buf->writableBytes();
#endif
					const ssize_t rc = ::read(sockfd, buf->beginWrite(), writable);
					if (rc > 0) {
						//只要可读(内核buf中还有数据)，就一直读，直到返回0，或者errno = EAGAIN
						n += (ssize_t)rc;
						buf->hasWritten(rc);
						if (buf->writableBytes() == 0) {
							buf->ensureWritableBytes(implicit_cast<size_t>(4096));
						}
						continue;
					}
					else if (rc < 0) {
						if (errno != EAGAIN /*&&
							errno != EWOULDBLOCK &&
							errno != ECONNABORTED &&
							errno != EPROTO*/ &&
							errno != EINTR) {
							printf("IBytesBuffer::readFull rc = %d errno = %d errmsg = %s\n",
								rc, errno, strerror(errno));
							*savedErrno = errno;
						}
						else {
							*savedErrno = errno;
						}
						break;
					}
					else /*if (rc == 0)*/ {
						//Connection has been aborted by peer
					}
				} while (true);
				//printf("IBytesBuffer::readFull end }}}\n\n");
				return n;
			}//readFull
			
			//writeFull for EPOLLET
			ssize_t writeFull(int sockfd, void const* data, size_t len, int* savedErrno) {
				ssize_t left = (ssize_t)len;
				ssize_t n = 0;
				while (left > 0) {
					int rc = ::write(sockfd, (char const*)data + n, left);
					if (rc > 0) {
						//只要可写(内核buf还有空间且用户待写数据还未写完)，就一直写，直到数据发送完，或者errno = EAGAIN
						n += (ssize_t)rc;
						left -= (ssize_t)rc;
						assert(errno == 0);
					}
					else if (rc < 0) {
						if (errno != EAGAIN /*&&
							errno != EWOULDBLOCK &&
							errno != ECONNABORTED &&
							errno != EPROTO*/ &&
							errno != EINTR) {
							printf("IBytesBuffer::writeFull rc = %d left = %d errno = %d errmsg = %s\n",
								rc, left, errno, strerror(errno));
							*savedErrno = errno;
						}
						else {
							*savedErrno = errno;
						}
						break;
					}
					else /*if (rc == 0)*/ {
						//assert(left == 0);
						//Connection has been aborted by peer
						break;
					}
				}
				return n;
			}//writeFull

		}//namespace ssl
    } // namespace net
} // namespace muduo