// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/TcpConnection.h"

#include "muduo/base/Logging.h"
#include "muduo/base/WeakCallback.h"
#include "muduo/net/Channel.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/Socket.h"
#include "muduo/net/SocketsOps.h"
#include "muduo/net/ssl/ssl.h"

#include <errno.h>

#include "libwebsocket/websocket.h"

using namespace muduo;
using namespace muduo::net;

void muduo::net::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
  LOG_TRACE << conn->localAddress().toIpPort() << " -> "
            << conn->peerAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
  // do not call conn->forceClose(), because some users want to register message callback only.
}

void muduo::net::defaultMessageCallback(const TcpConnectionPtr&,
                                        Buffer* buf,
                                        Timestamp)
{
  buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop,
                             const string& nameArg,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr,
                             SSL_CTX* ctx)
  : loop_(CHECK_NOTNULL(loop)),
    name_(nameArg),
    state_(kConnecting),
    reading_(true),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr),
    highWaterMark_(64*1024*1024),
    ssl_ctx_(ctx),
    ssl_(NULL),
    sslConnected_(false)
{
  channel_->setReadCallback(
      std::bind(&TcpConnection::handleRead, this, _1));
  channel_->setWriteCallback(
      std::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(
      std::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(
      std::bind(&TcpConnection::handleError, this));
  //LOG_DEBUG << "TcpConnection::ctor[" <<  name_ << "] at " << this
  //          << " fd=" << sockfd;
  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    //websocket support ///
    websocket::context_free(websocket_ctx_);
    //openSSL support ///
    ssl::SSL_free(ssl_);
    //////////////////////////////////////////////////////////////////////////
    //释放conn连接对象sockfd资源流程 ///
    //TcpServer::removeConnection ->
    //TcpServer::removeConnectionInLoop ->
    //TcpConnection::dtor ->
    //Socket::dtor -> sockets::close(sockfd_)
    //////////////////////////////////////////////////////////////////////////
    LOG_INFO << "TcpConnection::dtor[" <<  name_ << "] at " << this
            << " fd=" << channel_->fd()
            << " state=" << stateToString();
    if (state_ != kDisconnected) {
        LOG_ERROR << __FUNCTION__ << " --- *** " << " state_ = " << state_;
    }
  assert(state_ == kDisconnected);
}

bool TcpConnection::getTcpInfo(struct tcp_info* tcpi) const
{
  return socket_->getTcpInfo(tcpi);
}

string TcpConnection::getTcpInfoString() const
{
  char buf[1024];
  buf[0] = '\0';
  socket_->getTcpInfoString(buf, sizeof buf);
  return buf;
}

//
int TcpConnection::getFd() const {
    return socket_->fd();
}

void TcpConnection::send(const void* data, int len)
{
  send(StringPiece(static_cast<const char*>(data), len));
}

//overide
void TcpConnection::sendMessage(std::string const& message) {
	send(message.data(), message.size());
}
//overide
void TcpConnection::sendMessage(IBytesBuffer* message) {
    assert(message);
    Buffer* buf = reinterpret_cast<Buffer*>(message);
    assert(buf);
    send(buf);
}
//overide
std::string TcpConnection::peerIpAddrToString() const {
    return peerAddr_.toIp();
}

bool TcpConnection::initWebsocketContext(bool enable) {
	bool bok = false;
 	if (enable) {
        TcpConnectionPtr conn(shared_from_this());
        websocket::context_free(websocket_ctx_);
        websocket_ctx_ = websocket::context_new(
                WeakTcpConnectionPtr(conn),
		        http::IContextPtr(new HttpContext()),
		        IBytesBufferPtr(new Buffer()),
		        IBytesBufferPtr(new Buffer()));
        return websocket_ctx_ ? true : false;
    }
	return false;
}

//overide
void TcpConnection::onConnectedCallback(std::string const& ipaddr) {
    if (wsConnectedCallback_) {
        wsConnectedCallback_(shared_from_this(), ipaddr);
    }
}
//overide
void TcpConnection::onMessageCallback(IBytesBufferPtr buf, int msgType, ITimestamp* receiveTime) {
	if (wsMessageCallback_) {
		Buffer* buff = reinterpret_cast<Buffer*>(buf.get());
		assert(buff);

        Timestamp* preceiveTime = reinterpret_cast<Timestamp*>(receiveTime);
        assert(preceiveTime);

        wsMessageCallback_(shared_from_this(), buff, msgType, *preceiveTime);
	}
}
//overide
void TcpConnection::onClosedCallback(IBytesBufferPtr buf, ITimestamp* receiveTime) {
	if (wsClosedCallback_) {
		Buffer* buff = reinterpret_cast<Buffer*>(buf.get());
		assert(buff);

		Timestamp* preceiveTime = reinterpret_cast<Timestamp*>(receiveTime);
		assert(preceiveTime);

        wsClosedCallback_(shared_from_this(), buff, *preceiveTime);
	}
}

void TcpConnection::send(const StringPiece& message)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(message);
    }
    else
    {
      void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
      loop_->runInLoop(
          std::bind(fp,
                    this,     // FIXME
                    message.as_string()));
                    //std::forward<string>(message)));
    }
  }
}

// FIXME efficiency!!!
void TcpConnection::send(Buffer* buf)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(buf->peek(), buf->readableBytes());
      buf->retrieveAll();
    }
    else
    {
      void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
      loop_->runInLoop(
          std::bind(fp,
                    this,     // FIXME
                    buf->retrieveAllAsString()));
                    //std::forward<string>(message)));
    }
  }
}

void TcpConnection::sendInLoop(const StringPiece& message)
{
  sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool faultError = false;
  if (state_ == kDisconnected)
  {
    //LOG_WARN << "disconnected, give up writing";
    return;
  }
  // if no thing in output queue, try writing directly
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
  {
#if 1
      printf("\n----------------------------------------------\n");
      !ssl_ ?
          printf("write_with_no_copy >>>\n%.*s\n", len, data) :
          printf("SSL_write_with_no_copy >>>\n%.*s\n", len, data);
#endif
      int savedErrno = 0;
      nwrote = !ssl_ ?
          sockets::write(channel_->fd(), data, len) :
          Buffer::SSL_write(ssl_, data, len, &savedErrno);
    if (nwrote >= 0)
    {
      remaining = len - nwrote;
      if (remaining == 0 && writeCompleteCallback_)
      {
        loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
      }
    }
    else if (savedErrno != 0) {
		switch (savedErrno)
		{
		case SSL_ERROR_ZERO_RETURN:
			handleClose();
			break;
		default:
			handleClose();
			break;
		}
    }
    else // nwrote < 0
    {
      nwrote = 0;
      if (errno != EWOULDBLOCK)
      {
        //ERROR Broken pipe (errno=32) TcpConnection::sendInLoop - TcpConnection.cc:170
        //LOG_SYSERR << "TcpConnection::sendInLoop";
        if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
        {
          faultError = true;
        }
      }
    }
  }

  assert(remaining <= len);
  if (!faultError && remaining > 0)
  {
    size_t oldLen = outputBuffer_.readableBytes();
    if (oldLen + remaining >= highWaterMark_
        && oldLen < highWaterMark_
        && highWaterMarkCallback_)
    {
      loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
    }
    outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
    if (!channel_->isWriting())
    {
      channel_->enableWriting();
    }
  }
}

void TcpConnection::shutdown()
{
  // FIXME: use compare and swap
  if (state_ == kConnected)
  {
    setState(kDisconnecting);
    // FIXME: shared_from_this()?
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop()
{
  loop_->assertInLoopThread();
  if (!channel_->isWriting())
  {
    //openSSL support ///
    //ssl::SSL_free(ssl_);
    // we are not writing
    socket_->shutdownWrite();
  }
}

// void TcpConnection::shutdownAndForceCloseAfter(double seconds)
// {
//   // FIXME: use compare and swap
//   if (state_ == kConnected)
//   {
//     setState(kDisconnecting);
//     loop_->runInLoop(std::bind(&TcpConnection::shutdownAndForceCloseInLoop, this, seconds));
//   }
// }

// void TcpConnection::shutdownAndForceCloseInLoop(double seconds)
// {
//   loop_->assertInLoopThread();
//   if (!channel_->isWriting())
//   {
//     // we are not writing
//     socket_->shutdownWrite();
//   }
//   loop_->runAfter(
//       seconds,
//       makeWeakCallback(shared_from_this(),
//                        &TcpConnection::forceCloseInLoop));
// }

void TcpConnection::forceClose()
{
  // FIXME: use compare and swap
  if (state_ == kConnected || state_ == kDisconnecting)
  {
    setState(kDisconnecting);
    loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
  }
}

void TcpConnection::forceCloseWithDelay(double seconds)
{
  if (state_ == kConnected || state_ == kDisconnecting)
  {
    setState(kDisconnecting);
    loop_->runAfter(
        seconds,
        makeWeakCallback(shared_from_this(),
                         &TcpConnection::forceClose));  // not forceCloseInLoop to avoid race condition
  }
}

void TcpConnection::forceCloseInLoop()
{
  loop_->assertInLoopThread();
  if (state_ == kConnected || state_ == kDisconnecting)
  {
    // as if we received 0 byte in handleRead();
    handleClose();
  }
}

const char* TcpConnection::stateToString() const
{
  switch (state_)
  {
    case kDisconnected:
      return "kDisconnected";
    case kConnecting:
      return "kConnecting";
    case kConnected:
      return "kConnected";
    case kDisconnecting:
      return "kDisconnecting";
    default:
      return "unknown state";
  }
}

void TcpConnection::setTcpNoDelay(bool on)
{
  socket_->setTcpNoDelay(on);
}

void TcpConnection::startRead()
{
  loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop()
{
  loop_->assertInLoopThread();
  if (!reading_ || !channel_->isReading())
  {
    channel_->enableReading();
    reading_ = true;
  }
}

void TcpConnection::stopRead()
{
  loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop()
{
  loop_->assertInLoopThread();
  if (reading_ || channel_->isReading())
  {
    channel_->disableReading();
    reading_ = false;
  }
}

void TcpConnection::connectEstablished()
{
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  setState(kConnected);
  channel_->tie(shared_from_this());
  channel_->enableReading();

  connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
  loop_->assertInLoopThread();
  if (state_ == kConnected)
  {
    setState(kDisconnected);
    channel_->disableAll();

    connectionCallback_(shared_from_this());
  }
  channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
  loop_->assertInLoopThread();
  if (ssl_ctx_ && !sslConnected_) {
      int saveErrno = 0;
      //SSL握手连接 ///
      sslConnected_ = ssl::SSL_handshake(ssl_ctx_, ssl_, socket_->fd(), saveErrno);
	  switch (saveErrno)
	  {
	  case SSL_ERROR_WANT_READ:
		  channel_->enableReading();
		  break;
	  case SSL_ERROR_WANT_WRITE:
		  channel_->enableWriting();
		  break;
      case SSL_ERROR_SSL:
          LOG_ERROR << __FUNCTION__ << " --- *** " << "SSL_ERROR_SSL handleClose()";
          handleClose();
          break;
	  case 0:
          //succ
		  break;
	  default:
          handleClose();
		  break;
	  }
  }
  else {
      int savedErrno = 0;
      ssize_t n = !ssl_ ?
          inputBuffer_.readFd(channel_->fd(), &savedErrno) :
          inputBuffer_.SSL_read(ssl_, &savedErrno);
      if (n > 0)
      {
          messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
      }
      else if (n == 0)
      {
          handleClose();
      }
      else
      {
          errno = savedErrno;
          //LOG_SYSERR << "TcpConnection::handleRead";
          handleError();
      }
  }
}

void TcpConnection::handleWrite()
{
  loop_->assertInLoopThread();
  if (ssl_ctx_ && !sslConnected_) {
	  int saveErrno = 0;
	  sslConnected_ = ssl::SSL_handshake(ssl_ctx_, ssl_, socket_->fd(), saveErrno);
	  switch (saveErrno)
	  {
	  case SSL_ERROR_WANT_READ:
		  channel_->enableReading();
		  break;
	  case SSL_ERROR_WANT_WRITE:
		  channel_->enableWriting();
		  break;
	  case SSL_ERROR_SSL:
          LOG_ERROR << __FUNCTION__ << " --- *** " << "SSL_ERROR_SSL handleClose()";
		  handleClose();
		  break;
	  case 0:
		  //succ
		  break;
	  default:
          handleClose();
		  break;
	  }
  }
  else {
      if (channel_->isWriting())
      {
#if 1
          printf("\n----------------------------------------------\n");
          !ssl_ ?
              printf("write_with_copy >>>\n%.*s\n", outputBuffer_.readableBytes(), outputBuffer_.peek()) :
              printf("SSL_write_with_copy >>>\n%.*s\n", outputBuffer_.readableBytes(), outputBuffer_.peek());
#endif
          int savedErrno = 0;
          ssize_t n = !ssl_ ?
              sockets::write(channel_->fd(),
                  outputBuffer_.peek(),
                  outputBuffer_.readableBytes()) :
              Buffer::SSL_write(ssl_,
                  outputBuffer_.peek(),
                  outputBuffer_.readableBytes(), &savedErrno);
#if 1
		  if (ssl_) {
			  printf("SSL_write_with_copy n = %d err = %d\n", n, savedErrno);
		  }
#endif
          if (n > 0)
          {
              outputBuffer_.retrieve(n);
              if (outputBuffer_.readableBytes() == 0)
              {
                  channel_->disableWriting();
                  if (writeCompleteCallback_)
                  {
                      loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                  }
                  if (state_ == kDisconnecting)
                  {
                      shutdownInLoop();
                  }
              }
          }
          else if (savedErrno != 0) {
              switch (savedErrno)
              {
              case SSL_ERROR_ZERO_RETURN:
                  handleClose();
                  break;
              default:
                  handleClose();
                  break;
              }
          }
          else
          {
              LOG_SYSERR << "TcpConnection::handleWrite";
              // if (state_ == kDisconnecting)
              // {
              //   shutdownInLoop();
              // }
          }
      }
      else
      {
          LOG_TRACE << "Connection fd = " << channel_->fd()
              << " is down, no more writing";
      }
  }
}

void TcpConnection::handleClose()
{
  loop_->assertInLoopThread();
  //LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();
  assert(state_ == kConnected || state_ == kDisconnecting);
  // we don't close fd, leave it to dtor, so we can find leaks easily.
  setState(kDisconnected);
  channel_->disableAll();

  TcpConnectionPtr guardThis(shared_from_this());
  connectionCallback_(guardThis);
  // must be the last line
  closeCallback_(guardThis);
}

void TcpConnection::handleError()
{
  int err = sockets::getSocketError(channel_->fd());
  //LOG_ERROR << "TcpConnection::handleError [" << name_
  //          << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}

