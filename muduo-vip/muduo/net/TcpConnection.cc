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

#include <errno.h>

#include <muduo/net/libwebsocket/context.h>
#include <muduo/net/libwebsocket/ssl.h>

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
                             SSL_CTX* ctx,
                             bool et)
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
    sslConnected_(false),
    enable_et_(et)
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
	//////////////////////////////////////////////////////////////////////////
    //释放conn连接对象sockfd资源流程 ///
    //TcpConnection::handleClose ->
    //TcpConnection::closeCallback_ ->
    //TcpServer::removeConnection ->
    //TcpServer::removeConnectionInLoop ->
    //TcpConnection::dtor ->
    //Socket::dtor -> sockets::close(sockfd_)
    //////////////////////////////////////////////////////////////////////////
    //LOG_WARN << "TcpConnection::dtor[" <<  name_ << "] at " << this
    //       << " fd=" << channel_->fd()
    //        << " state=" << stateToString();
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

//setWsContext
void TcpConnection::setWsContext(WsContextPtr& context) {
    wsContext_ = std::move(context);
}

//getWsContext
WsContextPtr& TcpConnection::getWsContext() {
    return wsContext_;
}

void TcpConnection::send(const void* data, int len)
{
  send(StringPiece(static_cast<const char*>(data), len));
}

void TcpConnection::send(const StringPiece& message)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(message.data(), message.size()/*message*/);
    }
    else
    {
#if 1
      void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
      QueueInLoop(loop_,
          std::bind(fp,
                    this,     // FIXME
                    message.as_string()));
                    //std::forward<string>(message)));
#else
        std::string msg(message.data(), message.size());
        QueueInLoop(loop_,
			std::bind(&TcpConnection::sendInLoop_,
				this, msg));
#endif
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
#if 1
      void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
      QueueInLoop(loop_,
          std::bind(fp,
                    this,     // FIXME
                    buf->retrieveAllAsString()));
                    //std::forward<string>(message)));
#else
      std::string msg(buf->peek(), buf->readableBytes());
      QueueInLoop(loop_,
         std::bind(&TcpConnection::sendInLoop_,
                    this, msg));
#endif
    }
  }
}

void TcpConnection::sendInLoop(const StringPiece& message)
{
  sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop_(const std::string& message) {
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
#if 0
      printf("\n----------------------------------------------\n");
      !ssl_ ?
          printf("write_with_no_copy >>>\n%.*s\n", len, data) :
          printf("SSL_write_with_no_copy >>>\n%.*s\n", len, data);
#endif
      int savedErrno = 0;
      nwrote = !ssl_ ?
          /*sockets::write(channel_->fd(), data, len)*/
          Buffer::writeFull(channel_->fd(), data, len, &savedErrno) :
          Buffer::SSL_write(ssl_, data, len, &savedErrno);
    if (nwrote >= 0)
    {
      remaining = len - nwrote;
      if (remaining == 0 && writeCompleteCallback_)
      {
        QueueInLoop(loop_, std::bind(writeCompleteCallback_, shared_from_this()));
      }
    }
	if (ssl_) {
		switch (savedErrno)
		{
		case SSL_ERROR_WANT_WRITE:
			break;
		case SSL_ERROR_ZERO_RETURN:
			//SSL has been shutdown
			handleClose();
			break;
		case 0:
			break;
		default:
			handleClose();
			break;
		}
	}
    else { // nwrote < 0
		if (savedErrno > 0) {
			//rc > 0 errno = EAGAIN(11)
			//rc > 0 errno = 0
            //rc > 0 errno = EINPROGRESS(115)
			switch (errno)
			{
            case EINPROGRESS:
			case EAGAIN:
            case EINTR:
				//make sure that reset errno after callback
				errno = 0;
				break;
            case 0:
                break;
			default:
				//LOG_SYSERR << "TcpConnection::handleWrite";
				//handleError();
				handleClose();
				break;
			}
		}
		else if (savedErrno == 0) { // n = 0
			//rc = 0 errno = EAGAIN(11)
			//rc = 0 errno = 0 peer close
			handleClose();
		}
		else /*if (savedErrno < 0)*/ {
			//rc = -1 errno = EAGAIN(11)
			//rc = -1 errno = 0
			switch (errno)
			{
			case EAGAIN:
			case EINTR:
				//make sure that reset errno after callback
				errno = 0;
				break;
			case 0:
				break;
			default:
				//LOG_SYSERR << "TcpConnection::handleWrite";
				//handleError();
				handleClose();
				break;
			}
		}
		nwrote = 0;
        if (errno != EAGAIN /*&&
            errno != EWOULDBLOCK &&
            errno != ECONNABORTED*/ &&
            errno != EINTR)
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
      QueueInLoop(loop_, std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
    }
    outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
    if (!channel_->isWriting())
    {
      channel_->enableWriting(enable_et_);
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
    RunInLoop(loop_, std::bind(&TcpConnection::shutdownInLoop, this));
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
    QueueInLoop(loop_, std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
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
    RunInLoop(loop_, std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop()
{
  loop_->assertInLoopThread();
  if (!reading_ || !channel_->isReading())
  {
    channel_->enableReading(enable_et_);
    reading_ = true;
  }
}

void TcpConnection::stopRead()
{
    RunInLoop(loop_, std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop()
{
  loop_->assertInLoopThread();
  if (reading_ || channel_->isReading())
  {
    channel_->disableReading(enable_et_);
    reading_ = false;
  }
}

void TcpConnection::connectEstablished()
{
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  setState(kConnected);
  channel_->tie(shared_from_this());
  channel_->enableReading(enable_et_);

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
	  int savedErrno = 0;
	  //SSL握手连接 ///
	  sslConnected_ = ssl::SSL_handshake(ssl_ctx_, ssl_, socket_->fd(), savedErrno);
	  switch (savedErrno)
	  {
	  case SSL_ERROR_WANT_READ:
		  channel_->enableReading(enable_et_);
		  break;
	  case SSL_ERROR_WANT_WRITE:
		  channel_->enableWriting(enable_et_);
		  break;
	  case SSL_ERROR_SSL:
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
		  /* inputBuffer_.readFd(channel_->fd(), &savedErrno)*/
		  inputBuffer_.readFull(channel_->fd(), &savedErrno) :
		  inputBuffer_.SSL_read(ssl_, &savedErrno);
	  if (n > 0)
	  {
		  if (errno == EAGAIN ||
			  errno == EINTR) {
			  errno = 0;
		  }
		  messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
	  }
      if (ssl_) {
		  switch (savedErrno)
		  {
		  case SSL_ERROR_WANT_READ:
			  channel_->enableReading(enable_et_);
			  break;
		  case SSL_ERROR_WANT_WRITE:
			  channel_->enableWriting(enable_et_);
			  break;
          case SSL_ERROR_ZERO_RETURN:
			  handleClose();
			  break;
		  case SSL_ERROR_SSL:
			  handleClose();
			  break;
		  case 0:
              //SSL_read for EPOLLET
              if (channel_->isETReading()) {
                  channel_->enableReading(enable_et_);
              }
			  break;
		  default:
			  handleClose();
			  break;
		  }
      }
      else {
		  if (savedErrno > 0) {
			  //rc > 0 errno = EAGAIN(11)
			  //rc > 0 errno = 0
			  switch (errno)
			  {
			  case EAGAIN:
			  case EINTR:
				  //make sure that reset errno after callback
				  errno = 0;
                  channel_->enableReading(enable_et_);
				  break;
			  case 0:
				  break;
			  default:
				  //LOG_SYSERR << "TcpConnection::handleRead";
				  //handleError();
				  handleClose();
				  break;
			  }
		  }
		  else if (savedErrno == 0) { // n = 0
			  //rc = 0 errno = EAGAIN(11)
			  //rc = 0 errno = 0 peer close
			  handleClose();
		  }
		  else /*if (savedErrno < 0)*/ {
			  //rc = -1 errno = EAGAIN(11)
			  //rc = -1 errno = 0
			  switch (errno)
			  {
			  case EAGAIN:
			  case EINTR:
				  //make sure that reset errno after callback
				  errno = 0;
                  channel_->enableReading(enable_et_);
				  break;
			  case 0:
				  break;
			  default:
				  //LOG_SYSERR << "TcpConnection::handleRead";
				  //handleError();
				  handleClose();
				  break;
			  }
		  }
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
		  channel_->enableReading(enable_et_);
		  break;
	  case SSL_ERROR_WANT_WRITE:
		  channel_->enableWriting(enable_et_);
		  break;
	  case SSL_ERROR_SSL:
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
#if 0
          printf("\n----------------------------------------------\n");
          !ssl_ ?
              printf("write_with_copy >>>\n%.*s\n", outputBuffer_.readableBytes(), outputBuffer_.peek()) :
              printf("SSL_write_with_copy >>>\n%.*s\n", outputBuffer_.readableBytes(), outputBuffer_.peek());
#endif
          int savedErrno = 0;
      _loop:
          ssize_t n = !ssl_ ?
              /*sockets::write(channel_->fd(),
                  outputBuffer_.peek(),
                  outputBuffer_.readableBytes())*/
              Buffer::writeFull(channel_->fd(),
                  outputBuffer_.peek(),
                  outputBuffer_.readableBytes(), &savedErrno) :
              Buffer::SSL_write(ssl_,
                  outputBuffer_.peek(),
                  outputBuffer_.readableBytes(), &savedErrno);
          if (n > 0)
          {
              outputBuffer_.retrieve(n);
              if (outputBuffer_.readableBytes() == 0)
              {
                  channel_->disableWriting(enable_et_);
                  if (writeCompleteCallback_)
                  {
                      QueueInLoop(loop_, std::bind(writeCompleteCallback_, shared_from_this()));
                  }
                  if (state_ == kDisconnecting)
                  {
                      shutdownInLoop();
                  }
              }
          }
		  if (ssl_) {
			  switch (savedErrno)
			  {
			  case SSL_ERROR_WANT_WRITE:
				  channel_->enableWriting(enable_et_);
				  //goto _loop;
				  break;
			  case SSL_ERROR_ZERO_RETURN:
				  //SSL has been shutdown
				  handleClose();
				  break;
              case 0:
                  break;
			  default:
				  handleClose();
				  break;
			  }
		  }
		  else {
			  if (savedErrno > 0) {
				  //rc > 0 errno = EAGAIN(11)
				  //rc > 0 errno = 0
                  //rc > 0 errno = EINPROGRESS(115)
				  switch (errno)
				  {
                  case EINPROGRESS:
				  case EAGAIN:
                  case EINTR:
                      //make sure that reset errno after callback
                      errno = 0;
					  channel_->enableWriting(enable_et_);
					  //goto _loop;
					  break;
                  case 0:
                      break;
				  default:
					  //LOG_SYSERR << "TcpConnection::handleWrite";
					  //handleError();
					  handleClose();
					  break;
				  }
			  }
			  else if (savedErrno == 0) { // n = 0
				  //rc = 0 errno = EAGAIN(11)
				  //rc = 0 errno = 0 peer close
				  handleClose();
			  }
			  else /*if (savedErrno < 0)*/ {
				  //rc = -1 errno = EAGAIN(11)
				  //rc = -1 errno = 0
				  switch (errno)
				  {
				  case EAGAIN:
                  case EINTR:
					  //make sure that reset errno after callback
					  errno = 0;
					  channel_->enableWriting(enable_et_);
					  //goto _loop;
					  break;
				  case 0:
					  break;
				  default:
					  //LOG_SYSERR << "TcpConnection::handleWrite";
					  //handleError();
					  handleClose();
					  break;
				  }
			  }
		  }
          //else
          //{
          //    LOG_SYSERR << "TcpConnection::handleWrite";
          //    // if (state_ == kDisconnecting)
          //    // {
          //    //   shutdownInLoop();
          //    // }
          //}
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
  //openSSL support ///
  ssl::SSL_free(ssl_);
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

