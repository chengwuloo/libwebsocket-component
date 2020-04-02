#ifndef URLCODEC_INCLUDE_H
#define URLCODEC_INCLUDE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <iconv.h>

namespace URL {

	std::string Encode(const std::string& str);
	std::string Decode(const std::string& str);

	std::string MultipleEncode(const std::string& str);
	std::string MultipleDecode(const std::string& str);
}

#endif