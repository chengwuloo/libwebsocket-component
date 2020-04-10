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

#include "connector.h"
#include "EntryPtr.h"

typedef std::shared_ptr<muduo::net::Buffer> BufferPtr;

namespace STR {

	//@@ map[session] = entry
	typedef std::map<std::string, WeakEntryPtr> EntryMap;

	//@@ Entities
	class Entities : muduo::base::noncopyable {
	public:
		//add
		inline void add(std::string const& session, WeakEntryPtr const& weakEntry) {
			WRITE_LOCK(mutex_);
#ifndef NDEBUG
			EntryMap::const_iterator it = players_.find(session);
			assert(it == players_.end());
#endif
			players_[session] = weakEntry;
		}
		//get
		inline WeakEntryPtr get(std::string const& session) {
			WeakEntryPtr weakEntry;
			{
				READ_LOCK(mutex_);
				EntryMap::const_iterator it = players_.find(session);
				if (it != players_.end()) {
					weakEntry = it->second;
				}
			}
			return weakEntry;
		}
		//broadcast
		inline void broadcast(BufferPtr const buf) {
			READ_LOCK(mutex_);
			for (EntryMap::const_iterator it = players_.begin();
				it != players_.end(); ++it) {
				EntryPtr entry(it->second.lock());
				if (likely(entry)) {
					muduo::net::TcpConnectionPtr conn(entry->getWeakConnPtr().lock());
					if (conn) {
						muduo::net::websocket::Server::send(
							conn,
							buf->peek(), buf->readableBytes());
					}
				}
			}
		}
		//remove
		inline void remove(std::string const& session) {
			WRITE_LOCK(mutex_);
#if 1
			players_.erase(session);
#else
			EntryMap::const_iterator it = players_.find(session);
			if (it != players_.end()) {
				players_.erase(it);
			}
#endif
		}
	private:
		EntryMap players_;
		mutable boost::shared_mutex mutex_;
	};
}

namespace INT {

	//@@ map[userid] = entry
	typedef std::map<int64_t, WeakEntryPtr> EntryMap;

	//@@ Entities
	class Entities : muduo::base::noncopyable {
	public:
		//add
		void add(int64_t userid, WeakEntryPtr const& weakEntry) {
			WRITE_LOCK(mutex_);
			players_[userid] = weakEntry;
		}
		//get
		inline WeakEntryPtr get(int64_t userid) {
			WeakEntryPtr weakEntry;
			{
				READ_LOCK(mutex_);
				EntryMap::const_iterator it = players_.find(userid);
				if (it != players_.end()) {
					weakEntry = it->second;
				}
			}
			return weakEntry;
		}
		//broadcast
		inline void broadcast(BufferPtr const buf) {
			READ_LOCK(mutex_);
			for (EntryMap::const_iterator it = players_.begin();
				it != players_.end(); ++it) {
				EntryPtr entry(it->second.lock());
				if (likely(entry)) {
					muduo::net::TcpConnectionPtr conn(entry->getWeakConnPtr().lock());
					if (conn) {
						muduo::net::websocket::Server::send(
							conn,
							buf->peek(), buf->readableBytes());
					}
				}
			}
		}
		//remove
		inline void remove(int64_t userid) {
			WRITE_LOCK(mutex_);
#if 1
			players_.erase(userid);
#else
			EntryMap::const_iterator it = players_.find(userid);
			if (it != players_.end()) {
				players_.erase(it);
			}
#endif
		}
	private:
		EntryMap players_;
		mutable boost::shared_mutex mutex_;
	};

	//@@ map[userid] = session
	typedef std::map<int64_t, std::string> SessionMap;

	//@@ Sessions
	class Sessions : muduo::base::noncopyable {
	public:
		//add
		inline std::string const add(int64_t userid, std::string const& session) {
			std::string old;
			{
				WRITE_LOCK(mutex_);
				SessionMap::const_iterator it = players_.find(userid);
				if (it != players_.end()) {
					old = it->second;
					players_.erase(it);
				}
				players_[userid] = session;
			}
			return old;
		}
		//get
		inline std::string const/*&*/ get(int64_t userid) const {
			{
				READ_LOCK(mutex_);
				SessionMap::const_iterator it = players_.find(userid);
				if (it != players_.end()) {
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
				WRITE_LOCK(mutex_);
				SessionMap::const_iterator it = players_.find(userid);
				if (it != players_.end()) {
					//check before remove
					if (it->second == session) {
						players_.erase(it);
					}
				}
			}
		}
	private:
		SessionMap players_;
		mutable boost::shared_mutex mutex_;
	};
}

#endif