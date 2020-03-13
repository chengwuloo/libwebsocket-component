// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/Logging.h"
#include "muduo/net/Channel.h"
#include "muduo/net/EventLoop.h"

#include <sstream>

#include <poll.h>
#include <sys/epoll.h>

using namespace muduo;
using namespace muduo::net;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kReadEventET = POLLIN | POLLPRI | EPOLLET;
const int Channel::kWriteEvent = POLLOUT;
const int Channel::kWriteEventET = POLLOUT | EPOLLET;

Channel::Channel(EventLoop* loop, int fd)
  : loop_(loop),
    fd_(fd),
    events_(0),
    revents_(0),
    index_(-1),
    logHup_(true),
    tied_(false),
    eventHandling_(false),
    addedToLoop_(false)
{
}

Channel::~Channel()
{
  assert(!eventHandling_);
  assert(!addedToLoop_);
  if (loop_->isInLoopThread())
  {
    assert(!loop_->hasChannel(this));
  }
}

void Channel::tie(const std::shared_ptr<void>& obj)
{
  tie_ = obj;
  tied_ = true;
}

void Channel::update()
{
  addedToLoop_ = true;
  loop_->updateChannel(this);
}

void Channel::remove()
{
  assert(isNoneEvent());
  addedToLoop_ = false;
  loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
	handleEvent(receiveTime, revents_);
}

void Channel::handleEvent(Timestamp receiveTime, short revents) {
	std::shared_ptr<void> guard;
	if (tied_)
	{
		guard = tie_.lock();
		if (guard)
		{
			handleEventWithGuard(receiveTime, revents);
		}
	}
	else
	{
		handleEventWithGuard(receiveTime, revents);
	}
}

void Channel::handleEventWithGuard(Timestamp receiveTime, short revents) {
	eventHandling_ = true;
	LOG_TRACE << reventsToString();
	//¹Ø±Õ(close)
	if ((revents & POLLHUP) && !(revents & POLLIN))
	{
		if (logHup_)
		{
			//LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
		}
		if (closeCallback_) closeCallback_();
	}
	//ÎÞÐ§
	if (revents & POLLNVAL)
	{
		//LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLNVAL";
	}
	//´íÎó(error)
	if (revents & (POLLERR | POLLNVAL))
	{
		if (errorCallback_) errorCallback_();
	}
	//¶Á(accept/read/recv)
	if (revents & (POLLIN | POLLPRI | POLLRDHUP))
	{
		if (readCallback_) readCallback_(receiveTime, revents);
	}
	//Ð´(connect/write/send)
	if (revents & POLLOUT)
	{
		if (writeCallback_) writeCallback_();
	}
	eventHandling_ = false;
}

string Channel::reventsToString() const
{
  return eventsToString(fd_, revents_);
}

string Channel::eventsToString() const
{
  return eventsToString(fd_, events_);
}

string Channel::eventsToString(int fd, int ev)
{
  std::ostringstream oss;
  oss << fd << ": ";
  if (ev & POLLIN)
    oss << "IN ";
  if (ev & POLLPRI)
    oss << "PRI ";
  if (ev & POLLOUT)
    oss << "OUT ";
  if (ev & POLLHUP)
    oss << "HUP ";
  if (ev & POLLRDHUP)
    oss << "RDHUP ";
  if (ev & POLLERR)
    oss << "ERR ";
  if (ev & POLLNVAL)
    oss << "NVAL ";

  return oss.str();
}
