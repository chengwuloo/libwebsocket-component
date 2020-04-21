/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#ifndef ENTRYPTR_INCLUDE_H
#define ENTRYPTR_INCLUDE_H

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/base/noncopyable.h>
#include <muduo/base/Mutex.h>

#include <boost/filesystem.hpp>
#include <boost/circular_buffer.hpp>
//#include <boost/unordered_set.hpp>
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
#include <unordered_set>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "public/global.h"
#include "Clients.h"

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
	explicit Entry(const muduo::net::WeakTcpConnectionPtr& weakConn)
		: weakConn_(weakConn) {
	}
	inline muduo::net::WeakTcpConnectionPtr const& getWeakConnPtr() {
		return weakConn_;
	}
	~Entry() {
		muduo::net::TcpConnectionPtr conn(weakConn_.lock());
		if (conn) {
			conn->getLoop()->assertInLoopThread();
			LOG_WARN << __FUNCTION__ << " Entry::dtor";
#ifdef _DEBUG_BUCKETS_
			LOG_WARN << __FUNCTION__ << " 客户端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
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
	muduo::net::WeakTcpConnectionPtr weakConn_;
};

typedef std::shared_ptr<Entry> EntryPtr;
typedef std::weak_ptr<Entry> WeakEntryPtr;
//boost::unordered_set改用std::unordered_set
typedef std::unordered_set<EntryPtr> Bucket;
typedef boost::circular_buffer<Bucket> WeakConnList;

//////////////////////////////////////////////////////////////////////////
//@@ ConnBucket
struct ConnBucket : public muduo::noncopyable {
	explicit ConnBucket(muduo::net::EventLoop* loop, int index, size_t size)
		:loop_(CHECK_NOTNULL(loop)), index_(index) {
		//指定时间轮盘大小(bucket桶大小)
		//即环形数组大小(size) >=
		//心跳超时清理时间(timeout) >
		//心跳间隔时间(interval)
		buckets_.resize(size);
#ifdef _DEBUG_BUCKETS_
		LOG_WARN << __FUNCTION__ << " loop[" << index << "] timeout = " << size << "s";
#endif
	}
	//tick检查，间隔1s，踢出超时conn
	void onTimer() {
		loop_->assertInLoopThread();
		//////////////////////////////////////////////////////////////////////////
		//shared_ptr/weak_ptr 引用计数lock持有/递减是读操作，线程安全!
		//shared_ptr/unique_ptr new创建与reset释放是写操作，非线程安全，操作必须在同一线程!
		//new时内部引用计数递增，reset时递减，递减为0时销毁对象释放资源
		//////////////////////////////////////////////////////////////////////////
		buckets_.push_back(Bucket());
#ifdef _DEBUG_BUCKETS_
		LOG_WARN << __FUNCTION__ << " loop[" << index_ << "] timeout[" << buckets_.size() << "]";
#endif
		//重启连接超时定时器检查，间隔1s
		loop_->runAfter(1.0f, std::bind(&ConnBucket::onTimer, this));
	}
	//连接成功，压入桶元素
	void pushBucket(EntryPtr const& entry) {
		loop_->assertInLoopThread();
		if (likely(entry)) {
			muduo::net::TcpConnectionPtr conn(entry->weakConn_.lock());
			if (likely(conn)) {
				assert(conn->getLoop() == loop_);
				//必须使用shared_ptr，持有entry引用计数(加1)
				buckets_.back().insert(entry);
#ifdef _DEBUG_BUCKETS_
				LOG_WARN << __FUNCTION__ << " loop[" << index_ << "] timeout[" << buckets_.size() << "] 客户端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
					<< conn->localAddress().toIpPort() << "]";
#endif
			}
		}
		else {
			//assert(false);
		}
	}
	//收到消息包，更新桶元素
	void updateBucket(EntryPtr const& entry) {
		loop_->assertInLoopThread();
		if (likely(entry)) {
			muduo::net::TcpConnectionPtr conn(entry->weakConn_.lock());
			if (likely(conn)) {
				assert(conn->getLoop() == loop_);
				//必须使用shared_ptr，持有entry引用计数(加1)
				buckets_.back().insert(entry);
#ifdef _DEBUG_BUCKETS_
				LOG_WARN << __FUNCTION__ << " loop[" << index_ << "] timeout[" << buckets_.size() << "] 客户端[" << conn->peerAddress().toIpPort() << "] -> 网关服["
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
	muduo::net::EventLoop* loop_;
};

typedef std::unique_ptr<ConnBucket> ConnBucketPtr;

//////////////////////////////////////////////////////////////////////////
//@@ Context
struct Context : public muduo::noncopyable {
	explicit Context()
		: index_(0XFFFFFFFF) {
		reset();
	}
	explicit Context(WeakEntryPtr const& weakEntry)
		: index_(0XFFFFFFFF), weakEntry_(weakEntry) {
		reset();
	}
	explicit Context(const boost::any& context)
		: index_(0XFFFFFFFF), context_(context) {
		assert(!context_.empty());
		reset();
	}
	explicit Context(WeakEntryPtr const& weakEntry, const boost::any& context)
		: index_(0XFFFFFFFF), weakEntry_(weakEntry), context_(context) {
		assert(!context_.empty());
		reset();
	}
	~Context() {
		//LOG_WARN << __FUNCTION__ << " Context::dtor";
		reset();
	}
	//reset
	inline void reset() {
		ipaddr_ = 0;
		userid_ = 0;
		session_.clear();
		aeskey_.clear();
		//此处调用无效
#if 0
		//引用计数加1
		EntryPtr entry(weakEntry_.lock());
		if (likely(entry)) {
			//引用计数减1，减为0时析构entry对象
			//因为bucket持有引用计数，所以entry直到超时才析构
			entry.reset();
		}
#endif
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
	inline boost::any& getContext() {
		return context_;
	}
	inline boost::any* getMutableContext() {
		return &context_;
	}
 	inline WeakEntryPtr const& getWeakEntryPtr() {
 		return weakEntry_;
 	}
	//ipaddr
	inline void setFromIp(in_addr_t inaddr) { ipaddr_ = inaddr; }
	inline in_addr_t getFromIp() { return ipaddr_; }
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
		client_[ty].second = weakConn;
	}
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
	WeakEntryPtr weakEntry_;
	boost::any context_;
};

typedef std::shared_ptr<Context> ContextPtr;

//////////////////////////////////////////////////////////////////////////
//@@ EventLoopContext
class EventLoopContext : public muduo::noncopyable {
public:
	explicit EventLoopContext()
		: index_(0xFFFFFFFF) {
	}
	explicit EventLoopContext(int index)
		: index_(index) {
		assert(index_ >= 0);
	}
#if 0
	explicit EventLoopContext(EventLoopContext const& ref) {
		index_ = ref.index_;
		pool_.clear();
#if 0
		std::copy(ref.pool_.begin(), ref.pool_.end(), pool_.begin());
#else
		std::copy(ref.pool_.begin(), ref.pool_.end(), std::back_inserter(pool_));
#endif
	}
#endif
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
public:
	//bucketsPool_下标
	int index_;
	//threadPool_下标集合
	std::vector<int> pool_;
	//pool_游标
	muduo::AtomicInt32 nextPool_;
};

typedef std::shared_ptr<EventLoopContext> EventLoopContextPtr;

#endif
