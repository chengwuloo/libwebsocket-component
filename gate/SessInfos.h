/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#ifndef SESSINFO_INCLUDE_H
#define SESSINFO_INCLUDE_H

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

#ifndef USE_USERID_KEY
//session作为key
typedef std::map<std::string, WeakEntryPtr> SessInfosMap;
#else
//userid作为key
typedef std::map<int64_t, WeakEntryPtr> SessInfosMap;
#endif

class SessInfos : muduo::base::noncopyable {
public:
#ifndef USE_SESSION_KEY
	//addSessInfo
	inline void addSessInfo(std::string const& session, WeakEntryPtr const& weakEntry) {
		WRITE_LOCK(mutex_);
		players_[session] = weakEntry;
	}
	//getSessInfo
	inline WeakEntryPtr getSessInfo(std::string const& session) {
		WeakEntryPtr weakEntry;
		{
			READ_LOCK(mutex_);
			SessInfosMap::const_iterator it = players_.find(session);
			if (it != players_.end()) {
				weakEntry = it->second;
			}
		}
		return weakEntry;
	}
	//removeSessInfo
	inline void removeSessInfo(std::string const& session) {
		WRITE_LOCK(mutex_);
#if 1
		players_.erase(session);
#else
		SessInfosMap::const_iterator it = players_.find(session);
		if (it != players_.end()) {
			players_.erase(it);
		}
#endif
	}
#else
	//addSessInfo
	void addSessInfo(int64_t userid, WeakEntryPtr const& weakEntry) {
		WRITE_LOCK(mutex_);
		players_[userid] = weakEntry;
	}
	//getSessInfo
	inline WeakEntryPtr getSessInfo(int64_t userid) {
		WeakEntryPtr weakEntry;
		{
			READ_LOCK(mutex_);
			SessInfosMap::const_iterator it = players_.find(userid);
			if (it != players_.end()) {
				weakEntry = it->second;
			}
		}
		return weakEntry;
	}
	//removeSessInfo
	inline void removeSessInfo(int64_t userid) {
		WRITE_LOCK(mutex_);
#if 1
		players_.erase(userid);
#else
		SessInfosMap::const_iterator it = players_.find(userid);
		if (it != players_.end()) {
			players_.erase(it);
		}
#endif
	}
#endif
private:
	SessInfosMap players_;
	mutable boost::shared_mutex mutex_;
};

#endif