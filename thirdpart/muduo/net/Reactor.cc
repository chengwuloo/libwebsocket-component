
#include "Reactor.h"

#include "muduo/base/Logging.h"
#include "muduo/net/Acceptor.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThreadPool.h"
#include "muduo/net/SocketsOps.h"

#include <stdio.h>  // snprintf

namespace muduo {
	namespace net {

		static std::shared_ptr<muduo::net::EventLoopThreadPool> threadPool_;
		static AtomicInt32 started_;

		void ReactorSingleton::inst(muduo::net::EventLoop* loop, std::string const& name) {
			if (!threadPool_) {
#if 0
				std::shared_ptr<muduo::net::EventLoopThreadPool> obj(new muduo::net::EventLoopThreadPool(loop, name));
				threadPool_ = obj;
#else
				threadPool_.reset(new muduo::net::EventLoopThreadPool(loop, name));
#endif
			}
		}

		void ReactorSingleton::setThreadNum(int numThreads) {
			assert(0 <= numThreads);
			assert(threadPool_);
			threadPool_->setThreadNum(numThreads);
		}

		void ReactorSingleton::start(const EventLoopThreadPool::ThreadInitCallback& cb) {
			if (started_.getAndSet(1) == 0) {
				assert(threadPool_);
				threadPool_->start(cb);
			}
		}

		std::shared_ptr<muduo::net::EventLoopThreadPool> ReactorSingleton::threadPool() {
			assert(threadPool_);
			return threadPool_;
		}

		muduo::net::EventLoop* ReactorSingleton::getNextLoop() {
			assert(threadPool_);
			return threadPool_->getNextLoop();
		}

		void ReactorSingleton::reset() {
			threadPool_.reset();
		}

	}// namespace net
} //namespace muduo