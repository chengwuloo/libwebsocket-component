/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#include <muduo/base/Logging.h>
#include "public/global.h"
#include "Context.h"

//add
void ContextConnector::add(std::vector<std::string> const& ips) {
	{
		WRITE_LOCK(mutex_);
		ips_.assign(ips.begin(), ips.end());
	}
	{
#if 0
		READ_LOCK(mutex_);
		for (std::string const& ip : ips_) {
			//添加新节点
			ContextConnector::add(ip);
		}
#else
		for (std::string const& ip : ips) {
			//添加新节点
			ContextConnector::add(ip);
		}
#endif
	}
}

//process
void ContextConnector::process(std::vector<std::string> const& ips) {
	std::set<std::string> oldset, newset(ips.begin(), ips.end());
	{
		READ_LOCK(mutex_);
		for (std::string const& ip : ips_) {
			oldset.emplace(ip);
		}
	}
	//失效节点：ips_中有，而ips中没有
	std::vector<std::string> diff(oldset.size());
	std::vector<std::string>::iterator it;
	it = set_difference(oldset.begin(), oldset.end(), newset.begin(), newset.end(), diff.begin());
	diff.resize(it - diff.begin());
	for (std::string const& ip : diff) {
		//ips_中有
		assert(std::find(std::begin(ips_), std::end(ips_), ip) != ips_.end());
		//ips中没有
		assert(std::find(std::begin(ips), std::end(ips), ip) == ips.end());
		//失效则移除
		connector_->remove(ip, false);
	}
	//活动节点：ips中有，而ips_中没有
	diff.clear();
	diff.resize(newset.size());
	it = set_difference(newset.begin(), newset.end(), oldset.begin(), oldset.end(), diff.begin());
	diff.resize(it - diff.begin());
	for (std::string const& ip : diff) {
		//ips_中没有
		assert(std::find(std::begin(ips_), std::end(ips_), ip) == ips_.end());
		//ips中有
		assert(std::find(std::begin(ips), std::end(ips), ip) != ips.end());
		//添加新节点
		ContextConnector::add(ip);
	}
	{
		//添加ips到ips_
		WRITE_LOCK(mutex_);
		ips_.assign(ips.begin(), ips.end());
	}
}

//add
void ContextConnector::add(std::string const& ip) {
	std::vector<std::string> vec;
	boost::algorithm::split(vec, ip, boost::is_any_of(":"));
	//vec：ip:port
	muduo::net::InetAddress serverAddr(vec[0], atoi(vec[1].c_str()));
	//try add & connect
	connector_->add(ip, serverAddr);
}
