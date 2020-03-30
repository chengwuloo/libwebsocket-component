#ifndef _CONSOLE_CLR_H_
#define _CONSOLE_CLR_H_

#include <stdio.h>
#include <string>
#include <muduo/base/AsyncLogging.h>

#define NONE			"\033[0m"
#define BLACK			"\033[0;32;30m"
#define DARKGRAY		"\033[1;32;30m"
#define BLUE			"\033[0;32;34m"
#define LIGHTBLUE		"\033[1;32;34m"
#define GREEN			"\033[0;32;32m"
#define LIGHTGREEN		"\033[1;32;32m"
#define CYAN			"\033[0;32;36m"
#define LIGHTCYAN		"\033[1;32;36m"
#define RED				"\033[0;32;31m"
#define LIGHTRED		"\033[1;32;31m"
#define PURPLE			"\033[0;32;35m"
#define LIGHTPURPLE		"\033[1;32;35m"
#define BROWN			"\033[0;32;33m"
#define YELLOW			"\033[1;32;33m"
#define LIGHTGRAY		"\033[0;32;37m"
#define WHITE			"\033[1;32;37m"


extern muduo::AsyncLogging* g_asyncLog;

static void asyncOutput(const char *msg, int len) {
	g_asyncLog->append(msg, len);
	std::string out = msg;
	// dump the error now.
	int pos = out.find("ERROR");
	if (pos >= 0) {
		out = RED + out + NONE;
	}
	// dump the warning now.
	pos = out.find("WARN");
	if (pos >= 0) {
		out = GREEN + out + NONE;
	}
	// dump the info now.
	pos = out.find("INFO");
	if (pos >= 0) {
		out = PURPLE + out + NONE;
	}
	// dump the debug now.
	pos = out.find("DEBUG");
	if (pos >= 0) {
		out = BROWN + out + NONE;
	}
	// dump the special content for write the output window now.
	size_t n = std::fwrite(out.c_str(), 1, out.length(), stdout);
	(void)n;
}

#endif