// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/poller/EPollPoller.h"

#include "muduo/base/Logging.h"
#include "muduo/net/Channel.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

// On Linux, the constants of poll(2) and epoll(4)
// are expected to be the same.
static_assert(EPOLLIN == POLLIN,        "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI,      "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT,      "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP,  "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR,      "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP,      "epoll uses same flag values as poll");

namespace
{
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}

EPollPoller::EPollPoller(EventLoop* loop)
  : Poller(loop),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventListSize)
{
  if (epollfd_ < 0)
  {
    LOG_SYSFATAL << "EPollPoller::EPollPoller";
  }
}

EPollPoller::~EPollPoller()
{
  ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
#ifndef _MUDUO_OPTIMIZE_CHNANNEL_
  //LOG_TRACE << "fd total count " << channels_.size();
  int numEvents = ::epoll_wait(epollfd_,
                               &*events_.begin(),
                               static_cast<int>(events_.size()),
                               timeoutMs);
  int savedErrno = errno;
  Timestamp now(Timestamp::now());
  if (numEvents > 0)
  {
    //LOG_TRACE << numEvents << " events happened";
    fillActiveChannels(numEvents, activeChannels);
    if (implicit_cast<size_t>(numEvents) == events_.size())
    {
      events_.resize(events_.size()*2);
    }
  }
  else if (numEvents == 0)
  {
    //LOG_TRACE << "nothing happened";
  }
  else
  {
    // error happens, log uncommon ones
    if (savedErrno != EINTR)
    {
      errno = savedErrno;
      //LOG_SYSERR << "EPollPoller::poll()";
    }
  }
  return now;
#else
	//LOG_ERROR << __FUNCTION__;
	int numEvents = ::epoll_wait(epollfd_,
								&*events_.begin(),
								static_cast<int>(events_.size()),
								timeoutMs);
	int saveErrno = errno;
	Timestamp recvTime(Timestamp::now());
	if (numEvents > 0) {
		//LOG_ERROR << numEvents << " events happened !!!";
		handleActiveChannels(numEvents, recvTime);
		if (numEvents == static_cast<int>(events_.size())) {
			events_.resize(events_.size() * 2);
		}
	}
	else if (numEvents == 0) {
	}
	else {
		if (saveErrno != EINTR) {
			errno = saveErrno;
		}
	}
	return recvTime;
#endif
}

void EPollPoller::fillActiveChannels(int numEvents,
                                     ChannelList* activeChannels) const
{
  assert(implicit_cast<size_t>(numEvents) <= events_.size());
  for (int i = 0; i < numEvents; ++i)
  {
    Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
    int fd = channel->fd();
    ChannelMap::const_iterator it = channels_.find(fd);
    assert(it != channels_.end());
    assert(it->second == channel);
#endif
	//Fixed BUG 同一个channel，不同的events，会出现覆盖的情况
	//by andy_ro 2019.12.14
    channel->set_revents(events_[i].events);
    activeChannels->push_back(channel);
  }
}

void EPollPoller::handleActiveChannels(int numEvents, Timestamp recvTime) const {
	for (int i = 0; i < numEvents; ++i) {
		Channel* channel = (Channel *)events_[i].data.ptr;
		if (channel == NULL) {
			LOG_FATAL << "channel = NULL";
		}
		channel->handleEvent(recvTime, events_[i].events);
	}
}

void EPollPoller::updateChannel(Channel* channel)
{
  Poller::assertInLoopThread();
#ifndef _MUDUO_OPTIMIZE_CHNANNEL_
  const int index = channel->index();
  LOG_TRACE << "fd = " << channel->fd()
    << " events = " << channel->events() << " index = " << index;
  if (index == kNew || index == kDeleted)
  {
    // a new one, add with EPOLL_CTL_ADD
    int fd = channel->fd();
    if (index == kNew)
    {
      assert(channels_.find(fd) == channels_.end());
      channels_[fd] = channel;
    }
    else // index == kDeleted
    {
      assert(channels_.find(fd) != channels_.end());
      assert(channels_[fd] == channel);
    }

    channel->set_index(kAdded);
    update(EPOLL_CTL_ADD, channel);
  }
  else
  {
    // update existing one with EPOLL_CTL_MOD/DEL
    int fd = channel->fd();
    (void)fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(index == kAdded);
    if (channel->isNoneEvent())
    {
      update(EPOLL_CTL_DEL, channel);
      channel->set_index(kDeleted);
    }
    else
    {
      update(EPOLL_CTL_MOD, channel);
    }
  }
#else
  if (channel->index() == kAdded) {
	  if (channel->isNoneEvent()) {
		  update(EPOLL_CTL_DEL, channel);
		  channel->set_index(kDeleted);
	  }
	  else {
		  update(EPOLL_CTL_MOD, channel);
	  }
  }
  else if (channel->index() == kNew) {
	  channel->set_index(kAdded);
	  update(EPOLL_CTL_ADD, channel);
  }
  else if (channel->index() == kDeleted) {
	  channel->set_index(kAdded);
	  update(EPOLL_CTL_ADD, channel);
  }
#endif
}

void EPollPoller::removeChannel(Channel* channel)
{
  Poller::assertInLoopThread();
#ifndef _MUDUO_OPTIMIZE_CHNANNEL_
  int fd = channel->fd();
  //LOG_TRACE << "fd = " << fd;
  assert(channels_.find(fd) != channels_.end());
  assert(channels_[fd] == channel);
  assert(channel->isNoneEvent());
  int index = channel->index();
  assert(index == kAdded || index == kDeleted);
  size_t n = channels_.erase(fd);
  (void)n;
  assert(n == 1);

  if (index == kAdded)
  {
    update(EPOLL_CTL_DEL, channel);
  }
  channel->set_index(kNew);
#else
  assert(
	  channel->index() == kAdded ||
	  channel->index() == kDeleted);
  if (channel->index() == kAdded) {
	  update(EPOLL_CTL_DEL, channel);
	  //channel->setstat(kDeleted);
  }
  else if (channel->index() == kDeleted) {
  }
  channel->set_index(kNew);
#endif
}

void EPollPoller::update(int operation, Channel* channel)
{
#ifndef _MUDUO_OPTIMIZE_CHNANNEL_
  struct epoll_event event;
  memZero(&event, sizeof event);
  event.events = channel->events();
  event.data.ptr = channel;
  int fd = channel->fd();
  //LOG_TRACE << "epoll_ctl op = " << operationToString(operation)
  //  << " fd = " << fd << " event = { " << channel->eventsToString() << " }";
  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
  {
    if (operation == EPOLL_CTL_DEL)
    {
      //LOG_SYSERR << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
    }
    else
    {
      //LOG_SYSFATAL << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
    }
  }
#else
	struct epoll_event ev = { 0 };
	ev.events = channel->events();
	ev.data.ptr = channel;

	if (::epoll_ctl(epollfd_, operation, channel->fd(), &ev) < 0) {

		if (operation == EPOLL_CTL_DEL) {
			//LOG_ERROR("%s %s",
			//	channel->eventsToString(channel->events()).c_str(), "EPOLL_CTL_DEL");
		}
		else {
			//LOG_FATAL("%s %s",
			//	channel->eventsToString(channel->events()).c_str(),
			//	operation == EPOLL_CTL_ADD ? "EPOLL_CTL_ADD" : "EPOLL_CTL_MOD");
		}
	}
#endif
}

const char* EPollPoller::operationToString(int op)
{
  switch (op)
  {
    case EPOLL_CTL_ADD:
      return "ADD";
    case EPOLL_CTL_DEL:
      return "DEL";
    case EPOLL_CTL_MOD:
      return "MOD";
    default:
      assert(false && "ERROR op");
      return "Unknown Operation";
  }
}
