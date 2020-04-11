/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.18.2020                                           */
/************************************************************************/
#ifndef CONTEXTCONNECTOR_INCLUDE_H
#define CONTEXTCONNECTOR_INCLUDE_H

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

#include "connector.h"
#include "EntryPtr.h"

//@@ ContextConnector
struct ContextConnector {
	
	//add
	void add(std::vector<std::string> const& names);

	//process
	void process(std::vector<std::string> const& names);

private:
	//add
	void add(std::string const& name);
	
	//remove
	void remove(std::string const& name);

public:
	servTyE ty_;
	Connector* connector_;
	std::vector<std::string> names_;
	mutable boost::shared_mutex mutex_;
};

#endif