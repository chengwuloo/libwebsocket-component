/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#ifndef ENTITIES_INCLUDE_H
#define ENTITIES_INCLUDE_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>  // memset
#include <string>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>
#include <map>
#include <list>
#include <vector>
#include <memory>
#include <iomanip>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "Clients.h"
#include "EntryPtr.h"

#include <muduo/base/noncopyable.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/libwebsocket/server.h>

typedef std::shared_ptr<muduo::net::Buffer> BufferPtr;

namespace STR {

	//@@ map[session] = weakConn
	typedef std::map<std::string, muduo::net::WeakTcpConnectionPtr> WeakConnMap;

	//@@ Entities
	class Entities : muduo::noncopyable {
	public:
		//add
		inline void add(std::string const& session, muduo::net::WeakTcpConnectionPtr const& weakConn) {
#if 0
			muduo::MutexLockGuard lock(mutex_);
#else
			WRITE_LOCK(mutex_);
#endif
#ifndef NDEBUG
			WeakConnMap::const_iterator it = peers_.find(session);
			assert(it == peers_.end());
#endif
			peers_[session] = weakConn;
		}
		//get
		inline muduo::net::WeakTcpConnectionPtr get(std::string const& session) {
			muduo::net::WeakTcpConnectionPtr weakConn;
			{
#if 0
				muduo::MutexLockGuard lock(mutex_);
#else
				READ_LOCK(mutex_);
#endif
				WeakConnMap::const_iterator it = peers_.find(session);
				if (it != peers_.end()) {
					weakConn = it->second;
				}
			}
			return weakConn;
		}
		//broadcast
		inline void broadcast(BufferPtr const buf) {
#if 0
			muduo::MutexLockGuard lock(mutex_);
#else
			READ_LOCK(mutex_);
#endif
			assert(buf);
			for (WeakConnMap::const_iterator it = peers_.begin();
				it != peers_.end(); ++it) {
				muduo::net::TcpConnectionPtr peer(it->second.lock());
				if (peer) {
					muduo::net::websocket::send(
						peer,
						buf->peek(), buf->readableBytes());
				}
			}
		}
		//remove
		inline void remove(std::string const& session) {
#if 0
			muduo::MutexLockGuard lock(mutex_);
#else
			WRITE_LOCK(mutex_);
#endif
#if 0
			peers_.erase(session);
#else
			WeakConnMap::const_iterator it = peers_.find(session);
			if (it != peers_.end()) {
				peers_.erase(it);
			}
#endif
		}
	private:
		WeakConnMap peers_;
#if 0
		muduo::MutexLock mutex_;
#else
		mutable boost::shared_mutex mutex_;
#endif
	};
}

namespace INT {

	//@@ map[userid] = weakConn
	typedef std::map<int64_t, muduo::net::WeakTcpConnectionPtr> WeakConnMap;

	//@@ Entities
	class Entities : muduo::noncopyable {
	public:
		//add
		//@return old weakConn
		inline muduo::net::WeakTcpConnectionPtr const add(int64_t userid, muduo::net::WeakTcpConnectionPtr const& weakConn) {
			muduo::net::WeakTcpConnectionPtr old;
			{
#if 0
				muduo::MutexLockGuard lock(mutex_);
#else
				WRITE_LOCK(mutex_);
#endif
				WeakConnMap::const_iterator it = peers_.find(userid);
				if (it != peers_.end()) {
					old = it->second;
					peers_.erase(it);
				}
				peers_[userid] = weakConn;
			}
			return old;
		}
		//get
		inline muduo::net::WeakTcpConnectionPtr const get(int64_t userid) {
			muduo::net::WeakTcpConnectionPtr weakConn;
			{
#if 0
				muduo::MutexLockGuard lock(mutex_);
#else
				READ_LOCK(mutex_);
#endif
				WeakConnMap::const_iterator it = peers_.find(userid);
				if (it != peers_.end()) {
					weakConn = it->second;
				}
			}
			return weakConn;
		}
		//broadcast
		inline void broadcast(BufferPtr const buf) {
#if 0
			muduo::MutexLockGuard lock(mutex_);
#else
			READ_LOCK(mutex_);
#endif
			assert(buf);
			for (WeakConnMap::const_iterator it = peers_.begin();
				it != peers_.end(); ++it) {
				muduo::net::TcpConnectionPtr peer(it->second.lock());
				if (peer) {
					muduo::net::websocket::send(
						peer,
						buf->peek(), buf->readableBytes());
				}
			}
		}
		//remove
		inline void remove(int64_t userid, muduo::net::TcpConnectionPtr const& conn) {
#if 0
			muduo::MutexLockGuard lock(mutex_);
#else
			WRITE_LOCK(mutex_);
#endif
			WeakConnMap::const_iterator it = peers_.find(userid);
			if (it != peers_.end()) {
				muduo::net::TcpConnectionPtr peer(it->second.lock());
				assert(peer);
				//check before remove
				if (peer == conn) {
					peers_.erase(it);
				}
			}
		}
		//remove
		inline void remove(int64_t userid, std::string const& session) {
#if 0
			muduo::MutexLockGuard lock(mutex_);
#else
			WRITE_LOCK(mutex_);
#endif
			WeakConnMap::const_iterator it = peers_.find(userid);
			if (it != peers_.end()) {
				muduo::net::TcpConnectionPtr peer(it->second.lock());
				if (peer) {
					ContextPtr entryContext(boost::any_cast<ContextPtr>(peer->getContext()));
					assert(entryContext);
					//check before remove
					if (entryContext->getSession() == session) {
						peers_.erase(it);
					}
				}
				else {
					peers_.erase(it);
				}
			}
		}
	private:
		WeakConnMap peers_;
#if 0
		muduo::MutexLock mutex_;
#else
		mutable boost::shared_mutex mutex_;
#endif
	};

	//@@ map[userid] = session
	typedef std::map<int64_t, std::string> SessionMap;

	//@@ Sessions
	class Sessions : muduo::noncopyable {
	public:
		//add
		//@return old session
		inline std::string const add(int64_t userid, std::string const& session) {
			std::string old;
			{
#if 0
				muduo::MutexLockGuard lock(mutex_);
#else
				WRITE_LOCK(mutex_);
#endif
				SessionMap::const_iterator it = sessions_.find(userid);
				if (it != sessions_.end()) {
					old = it->second;
					sessions_.erase(it);
				}
				sessions_[userid] = session;
			}
			return old;
		}
		//get
		inline std::string const/*&*/ get(int64_t userid) /*const*/ {
			{
#if 0
				muduo::MutexLockGuard lock(mutex_);
#else
				READ_LOCK(mutex_);
#endif
				SessionMap::const_iterator it = sessions_.find(userid);
				if (it != sessions_.end()) {
					return it->second;
				}
			}
			//static std::string s("");
			//return s;
			return "";
		}
		//remove
		inline void remove(int64_t userid, std::string const& session) {
			{
#if 0
				muduo::MutexLockGuard lock(mutex_);
#else
				WRITE_LOCK(mutex_);
#endif
				SessionMap::const_iterator it = sessions_.find(userid);
				if (it != sessions_.end()) {
					//check before remove
					if (it->second == session) {
						sessions_.erase(it);
					}
				}
			}
		}
	private:
		SessionMap sessions_;
#if 0
		muduo::MutexLock mutex_;
#else
		mutable boost::shared_mutex mutex_;
#endif
	};
}

#endif