/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#include <muduo/base/Logging.h>
#include "public/global.h"
#include "Context.h"

//add
void ContextConnector::add(std::vector<std::string> const& names) {
	{
		WRITE_LOCK(mutex_);
		names_.assign(names.begin(), names.end());
	}
	{
#if 0
		READ_LOCK(mutex_);
		for (std::string const& name : names_) {
#else
		for (std::string const& name : names) {
#endif
			//添加新节点
			ContextConnector::add(name);
		}
	}
}

//process
void ContextConnector::process(std::vector<std::string> const& names) {
	std::set<std::string> oldset, newset(names.begin(), names.end());
	{
		READ_LOCK(mutex_);
		for (std::string const& name : names_) {
			oldset.emplace(name);
		}
	}
	//失效节点：names_中有，而names中没有
	std::vector<std::string> diff(oldset.size());
	std::vector<std::string>::iterator it;
	it = set_difference(oldset.begin(), oldset.end(), newset.begin(), newset.end(), diff.begin());
	diff.resize(it - diff.begin());
	for (std::string const& name : diff) {
		//names_中有
		assert(std::find(std::begin(names_), std::end(names_), name) != names_.end());
		//names中没有
		assert(std::find(std::begin(names), std::end(names), name) == names.end());
		//失效则移除
		ContextConnector::remove(name);
	}
	//活动节点：names中有，而names_中没有
	diff.clear();
	diff.resize(newset.size());
	it = set_difference(newset.begin(), newset.end(), oldset.begin(), oldset.end(), diff.begin());
	diff.resize(it - diff.begin());
	for (std::string const& name : diff) {
		//names_中没有
		assert(std::find(std::begin(names_), std::end(names_), name) == names_.end());
		//names中有
		assert(std::find(std::begin(names), std::end(names), name) != names.end());
		//添加新节点
		ContextConnector::add(name);
	}
	{
		//添加names到names_
		WRITE_LOCK(mutex_);
		names_.assign(names.begin(), names.end());
	}
}

//add
void ContextConnector::add(std::string const& name) {
	switch (ty_) {
	case servTyE::kHallTy: {
		//大厅服
		std::vector<std::string> vec;
		boost::algorithm::split(vec, name, boost::is_any_of(":"));
		//name：ip:port
		muduo::net::InetAddress serverAddr(vec[0], atoi(vec[1].c_str()));
		LOG_WARN << __FUNCTION__ << " >>> 大厅服[" << vec[0] << ":" << vec[1] << "]";
		//try add & connect
		connector_->add(name, serverAddr);
		break;
	}
	case servTyE::kGameTy: {
		//游戏服 
		std::vector<std::string> vec;
		boost::algorithm::split(vec, name, boost::is_any_of(":"));
		//name：roomid:ip:port
		muduo::net::InetAddress serverAddr(vec[1], atoi(vec[2].c_str()));
		LOG_WARN << __FUNCTION__ << " >>> 游戏服[" << vec[1] << ":" << vec[2] << "] 房间号[" << vec[0] << "]";
		//try add & connect
		connector_->add(name, serverAddr);
		break;
	}
	}
}

//remove
void ContextConnector::remove(std::string const& name) {
	switch (ty_) {
	case servTyE::kHallTy: {
		//大厅服
		LOG_WARN << __FUNCTION__ << " >>> 大厅服[" << name << "]";
		//try remove
		connector_->remove(name, false);
		break;
	}
	case servTyE::kGameTy: {
		//游戏服
		LOG_WARN << __FUNCTION__ << " >>> 游戏服[" << name << "]";
		//try remove
		connector_->remove(name, true);
		break;
	}
	}
}