#ifndef URLCODEC_H
#define URLCODEC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <iconv.h>

namespace URL {
	std::string Encode(const std::string& str);
	std::string Decode(const std::string& str);

	std::string Encode2(const std::string& str);
	std::string Decode2(const std::string& str);
}

#endif