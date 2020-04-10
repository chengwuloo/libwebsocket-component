#ifndef UTILS_INCLUDE_H
#define UTILS_INCLUDE_H

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

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

//createUUID 创建uuid
static std::string createUUID() {
	boost::uuids::random_generator rgen;
	boost::uuids::uuid u = rgen();
	std::string uuid;
	uuid.assign(u.begin(), u.end());
	return uuid;
}

//buffer2HexStr
static std::string buffer2HexStr(unsigned char const* buf, size_t len)
{
	std::ostringstream oss;
	oss << std::hex << std::uppercase << std::setfill('0');
	for (size_t i = 0; i < len; ++i) {
		oss << std::setw(2) << (unsigned int)(buf[i]);
	}
	return oss.str();
}

#endif