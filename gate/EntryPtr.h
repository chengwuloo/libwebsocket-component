/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#ifndef ENTRYPTR_INCLUDE_H
#define ENTRYPTR_INCLUDE_H

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include "muduo/net/TcpConnection.h"

#include <boost/filesystem.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>

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
#include <sstream>
#include <iomanip>
#include <assert.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "public/global.h"
#include "connector.h"

//@@ servTyE
enum servTyE {
	kHallTy = 0,//大厅服
	kGameTy = 1,//游戏服
	kMaxServTy,
};

//////////////////////////////////////////////////////////////////////////
//@@ Entry 避免恶意连接占用系统sockfd资源不请求处理也不关闭fd情况，超时强行关闭连接
struct Entry : public muduo::noncopyable {
public:
	//@@ Context
	struct Context : public muduo::copyable {
		explicit Context()
			: index_(0XFFFFFFFF) {
		}
		explicit Context(int index)
			: index_(index) {
			assert(index_ >= 0);
		}
		explicit Context(const boost::any& context)
			: index_(0XFFFFFFFF), context_(context) {
			assert(!context_.empty());
		}
		explicit Context(int index, const boost::any& context)
			: index_(index), context_(context) {
			assert(index_ >= 0);
			assert(!context_.empty());
		}
		~Context() {
		}
		inline void setWorkerIndex(int index) {
			index_ = index;
			assert(index_ >= 0);
		}
		inline int getWorkerIndex() const {
			return index_;
		}
		inline void setContext(const boost::any& context) {
			context_ = context;
		}
		inline const boost::any& getContext() const {
			return context_;
		}
		inline boost::any* getMutableContext() {
			return &context_;
		}
		//ipaddr
		void setFromIp(in_addr_t inaddr) { ipaddr_ = inaddr; }
		in_addr_t getFromIp() { return ipaddr_; }
		//session
		inline void setSession(std::string const& session) { session_ = session; }
		inline std::string const& getSession() const { return session_; }
		//userid
		inline void setUserID(int64_t userid) { userid_ = userid; }
		inline int64_t getUserID() const { return userid_; }
		//aeskey
		inline void setAesKey(std::string key) { aeskey_ = key; }
		inline std::string const& getAesKey() const { return aeskey_; }
		//setClientConn
		inline void setClientConn(servTyE ty,
			std::string const& name,
			muduo::net::WeakTcpConnectionPtr const& weakConn) {
			assert(!name.empty());
			client_[ty].first = name;
			client_[ty].second = weakConn; }
		//setClientConn
		inline void setClientConn(servTyE ty, ClientConn const& client) {
			client_[ty] = client;
		}
		//getClientConn
		inline ClientConn const& getClientConn(servTyE ty) { return client_[ty]; }
	public:
		//threadPool_下标
		int index_;
		uint32_t ipaddr_;
		int64_t userid_;
		std::string session_;
		std::string aeskey_;
		ClientConn client_[kMaxServTy];
		boost::any context_;
	};
	explicit Entry(const muduo::net::WeakTcpConnectionPtr& weakConn)
		: weakConn_(weakConn) {
	}
	explicit Entry(const muduo::net::WeakTcpConnectionPtr& weakConn, const boost::any& context)
		: weakConn_(weakConn), context_(context) {
		assert(!context_.empty());
	}
	inline muduo::net::WeakTcpConnectionPtr const& getWeakConnPtr() {
		return weakConn_;
	}
	inline void setContext(const boost::any& context) {
		context_ = context;
	}
	inline const boost::any& getContext() const {
		return context_;
	}
	inline boost::any* getMutableContext() {
		return &context_;
	}
	~Entry() {
		muduo::net::TcpConnectionPtr conn(weakConn_.lock());
		if (conn) {
#ifdef _DEBUG_BUCKETS_
			LOG_WARN << __FUNCTION__ << " --- *** " << "客户端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
				<< conn->localAddress().toIpPort() << "] timeout closing";
#endif
#if 0
			//不再发送数据
			conn->shutdown();
#elif 1
			//直接强制关闭连接
			conn->forceClose();
#else
			//延迟0.2s强制关闭连接
			conn->forceCloseWithDelay(0.2f);
#endif
		}
	}
	boost::any context_;
	muduo::net::WeakTcpConnectionPtr weakConn_;
};

typedef std::shared_ptr<Entry> EntryPtr;
typedef std::weak_ptr<Entry> WeakEntryPtr;
typedef boost::unordered_set<EntryPtr> Bucket;
typedef boost::circular_buffer<Bucket> WeakConnList;

namespace detail {

	//resetEntry std::bind(&detail::resetEntry, weakEntry);
	static void resetEntry(WeakEntryPtr const weakEntry) {
		EntryPtr entry(weakEntry.lock());
		if (likely(entry)) {
			entry.reset();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//@@ EventLoopContext
class EventLoopContext : public muduo::copyable {
public:
	explicit EventLoopContext()
		: index_(0xFFFFFFFF) {
	}
	explicit EventLoopContext(int index)
		: index_(index) {
		assert(index_ >= 0);
	}
	explicit EventLoopContext(EventLoopContext const& ref) {
		index_ = ref.index_;
		pool_.clear();
#if 0
		std::copy(ref.pool_.begin(), ref.pool_.end(), pool_.begin());
#else
		std::copy(ref.pool_.begin(), ref.pool_.end(), std::back_inserter(pool_));
#endif
	}
	inline void setBucketIndex(int index) {
		index_ = index;
		assert(index_ >= 0);
	}
	inline int getBucketIndex() const {
		return index_;
	}
	inline void addWorkerIndex(int index) {
		pool_.emplace_back(index);
	}
	inline int allocWorkerIndex() {
		int index = nextPool_.getAndAdd(1) % pool_.size();
		if (index >= 0XFFFFFFFF) {
			nextPool_.getAndSet(-1);
			index = nextPool_.addAndGet(1);
		}
		assert(index >= 0 && index < pool_.size());
		return pool_[index];
	}
	~EventLoopContext() {
	}
private:
	//bucketsPool_下标
	int index_;
	//threadPool_下标集合
	std::vector<int> pool_;
	//pool_游标
	muduo::AtomicInt32 nextPool_;
};

//////////////////////////////////////////////////////////////////////////
//@@ ConnectionBucket
struct ConnectionBucket {
	explicit ConnectionBucket(muduo::net::EventLoop& loop, int index, size_t size)
		:loop_(loop), index_(index) {
		//指定时间轮盘大小(bucket桶大小)
		//即环形数组大小(size) >=
		//心跳超时清理时间(timeout) >
		//心跳间隔时间(interval)
		buckets_.resize(size);
#ifdef _DEBUG_BUCKETS_
		LOG_WARN << __FUNCTION__ << " --- *** [" << index << "] timeout = " << size << "s";
#endif
	}
	//tick检查，间隔1s，踢出超时conn
	void onTimer() {
		buckets_.push_back(Bucket());
#ifdef _DEBUG_BUCKETS_
		LOG_WARN << __FUNCTION__ << " --- *** loop[" << index_ << "] timeout[" << buckets_.size() << "]";
#endif
		//重启连接超时定时器检查，间隔1s
		loop_.runAfter(1.0f, std::bind(&ConnectionBucket::onTimer, this));
	}
	//连接成功，压入桶元素
	void pushBucket(EntryPtr const entry) {
		if (likely(entry)) {
			muduo::net::TcpConnectionPtr conn(entry->weakConn_.lock());
			if (likely(conn)) {
				//必须使用shared_ptr，持有entry引用计数(加1)
				buckets_.back().insert(entry);
#ifdef _DEBUG_BUCKETS_
				LOG_WARN << __FUNCTION__ << " --- *** loop[" << index_ << "] timeout[" << buckets_.size() << "] 客户端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
					<< conn->localAddress().toIpPort() << "]";
#endif
			}
		}
		else {
			//assert(false);
		}
	}
	//收到消息包，更新桶元素
	void updateBucket(EntryPtr const entry) {
		if (likely(entry)) {
			muduo::net::TcpConnectionPtr conn(entry->weakConn_.lock());
			if (likely(conn)) {
				//必须使用shared_ptr，持有entry引用计数(加1)
				buckets_.back().insert(entry);
#ifdef _DEBUG_BUCKETS_
				LOG_WARN << __FUNCTION__ << " --- *** loop[" << index_ << "] timeout[" << buckets_.size() << "] 客户端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
					<< conn->localAddress().toIpPort() << "]";
#endif
			}
		}
		else {
			//assert(false);
		}
	}
	//bucketsPool_下标
	int index_;
	WeakConnList buckets_;
	muduo::net::EventLoop& loop_;
};

#endif
