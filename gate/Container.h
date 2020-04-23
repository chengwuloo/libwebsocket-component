/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#ifndef CONTAINER_INCLUDE_H
#define CONTAINER_INCLUDE_H

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
#include <iomanip>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "Clients.h"
#include "EntryPtr.h"

//@@ Repair
struct Repair {

	//add
	inline void add(std::string const& name) {
		WRITE_LOCK(mutex_);
#ifndef NDEBUG
		std::map<std::string, bool>::const_iterator it = names_.find(name);
		assert(it == names_.end());
#endif
		names_[name] = true;
	}
	//remove
	inline void remove(std::string const& name) {
		WRITE_LOCK(mutex_);
#if 0
		names_.erase(name);
#else
		std::map<std::string, bool>::const_iterator it = names_.find(name);
		if (it != names_.end()) {
			names_.erase(it);
		}
#endif
	}
	//exist
	inline bool exist(std::string const& name) /*const*/ {
		READ_LOCK(mutex_);
		std::map<std::string, bool>::const_iterator it = names_.find(name);
		return it != names_.end();
	}
	//count
	inline size_t count() /*const*/ {
		READ_LOCK(mutex_);
		return names_.size();
	}
public:
	std::map<std::string, bool> names_;
	mutable boost::shared_mutex mutex_;
};

//@@ Container
struct Container {
	
	//add
	void add(std::vector<std::string> const& names);

	//process
	void process(std::vector<std::string> const& names);

	//exist
	inline bool exist(std::string const& name) /*const*/ {
		//判断是否在指定类型服务中
		return clients_->exists(name);
	}
	
	//count
	inline size_t count() /*const*/ {
		//活动节点数
		return clients_->count();
	}

	//维护节点
	inline void repair(std::string const& name) {
		repair_.add(name);
	}
	
	//恢复服务
	inline void recover(std::string const& name) {
		repair_.remove(name);
	}
	
	//是否维护中
	inline bool isRepairing(std::string const& name) /*const*/ {
		//判断是否在维护节点中
		return repair_.exist(name);
	}
	
	//服务中节点数
	inline ssize_t remaining() /*const*/ {
		//活动节点数 - 维护中节点数
		return clients_->count() - repair_.count();
	}
private:
	//add
	void add(std::string const& name);
	
	//remove
	void remove(std::string const& name);

public:
	Repair repair_;
	servTyE ty_;
	Connector* clients_;
	std::vector<std::string> names_;
	mutable boost::shared_mutex mutex_;
};

#endif