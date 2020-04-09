/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#include "Context.h"

//assign
void ContextConnector::assign(std::vector<std::string> const& ips) {
	WRITE_LOCK(mutex_);
	ips_.assign(ips.begin(), ips.end());
}

//processIps
void ContextConnector::processIps(std::vector<std::string> const& ips) {
	std::set<std::string> oldset, newset(ips.begin(), ips.end());
	{
		WRITE_LOCK(mutex_);
		for (std::string const& ip : ips_) {
			oldset.emplace(ip);
		}
	}
	//ʧЧ�ڵ㣺ips_���У���ips��û��
	std::vector<std::string> diff(oldset.size());
	std::vector<std::string>::iterator it;
	it = set_difference(oldset.begin(), oldset.end(), newset.begin(), newset.end(), diff.begin());
	diff.resize(it - diff.begin());
	for (std::string const& ip : diff) {
		//ips_����
		assert(std::find(
			std::begin(ips_),
			std::end(ips_), ip) != ips_.end());
		//ips��û��
		assert(std::find(
			std::begin(ips),
			std::end(ips), ip) == ips.end());
		connector_->remove(ip, false);
	}
	//��ڵ㣺ips���У���ips_��û��
	diff.clear();
	diff.resize(newset.size());
	it = set_difference(newset.begin(), newset.end(), oldset.begin(), oldset.end(), diff.begin());
	diff.resize(it - diff.begin());
	for (std::string const& ip : diff) {
		//ips_��û��
		assert(std::find(
			std::begin(ips_),
			std::end(ips_), ip) == ips_.end());
		//ips����
		assert(std::find(
			std::begin(ips),
			std::end(ips), ip) != ips.end());
		connect(ip);
	}
	{
		//���ips��ips_
		WRITE_LOCK(mutex_);
		ips_.assign(ips.begin(), ips.end());
	}
}

//connect
void ContextConnector::connect(std::string const& ip) {
	LOG_ERROR << __FUNCTION__;
	//��ȡ����I/Oģ��EventLoop��
	//std::shared_ptr<muduo::net::EventLoopThreadPool> threadPool = server_.server_.threadPool();
	//std::vector<muduo::net::EventLoop*> loops = threadPool->getAllLoops();
	std::vector<std::string> vec;
	boost::algorithm::split(vec, ip, boost::is_any_of(":"));
	//vec��ip:port
	muduo::net::InetAddress serverAddr(vec[0], stoi(vec[1]));
	//muduo::net::EventLoop* ioLoop = threadPool->getNextLoop();
	connector_->create(ip, serverAddr);
}

//connectAll
void ContextConnector::connectAll() {
	for (std::vector<std::string>::const_iterator it = ips_.begin();
		it != ips_.end(); ++it) {
		connect(*it);
	}
}