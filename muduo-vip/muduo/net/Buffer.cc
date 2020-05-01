// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include "muduo/net/Buffer.h"

#include "muduo/net/SocketsOps.h"

#include <errno.h>
#include <sys/uio.h>

#include <muduo/net/libwebsocket/ssl.h>

using namespace muduo;
using namespace muduo::net;

const char Buffer::kCRLF[] = "\r\n";
const char Buffer::kCRLFCRLF[] = "\r\n\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

ssize_t Buffer::readFd(int fd, int* savedErrno)
{
  // saved an ioctl()/FIONREAD call to tell how much to read
  char extrabuf[65536];
  struct iovec vec[2];
  const size_t writable = writableBytes();
  vec[0].iov_base = begin()+writerIndex_;
  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof extrabuf;
  // when there is enough space in this buffer, don't read into extrabuf.
  // when extrabuf is used, we read 128k-1 bytes at most.
  const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
  const ssize_t n = sockets::readv(fd, vec, iovcnt);
  if (n < 0)
  {
    *savedErrno = errno;
  }
  else if (implicit_cast<size_t>(n) <= writable)
  {
    writerIndex_ += n;
  }
  else
  {
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  // if (n == writable + sizeof extrabuf)
  // {
  //   goto line_30;
  // }
  return n;
}

//readFull for EPOLLET
ssize_t Buffer::readFull(int sockfd, int* savedErrno) {
    return IBytesBuffer::readFull(sockfd, this, savedErrno);
}

//writeFull for EPOLLET
ssize_t Buffer::writeFull(int sockfd, void const* data, size_t len, int* savedErrno) {
    return IBytesBuffer::writeFull(sockfd, data, len, savedErrno);
}

//SSL_read
ssize_t Buffer::SSL_read(SSL* ssl, int* savedErrno) {
    return ssl::SSL_read(ssl, this, savedErrno);
}

//SSL_write
ssize_t Buffer::SSL_write(SSL* ssl, void const* data, size_t len, int* savedErrno) {
    return ssl::SSL_write(ssl, data, len, savedErrno);
}
