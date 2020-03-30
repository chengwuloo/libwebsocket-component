#ifndef NETCARDIP_INCLUDE_H
#define NETCARDIP_INCLUDE_H

#ifndef _WIN32
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <assert.h>
#include <time.h>

#include <sstream>
#include <fstream>

#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <dlfcn.h>

#include <linux/if.h>

#define MAXBUFSZ 1024

static int IpByNetCardName(std::string const& netName, std::string& strIP) {
	int sockfd;
	struct ifconf conf;
	struct ifreq* ifr;
	char buff[MAXBUFSZ] = { 0 };
	int num;
	int i;
	sockfd = ::socket(PF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		return -1;
	}
	conf.ifc_len = MAXBUFSZ;
	conf.ifc_buf = buff;
	if (::ioctl(sockfd, SIOCGIFCONF, &conf) < 0) {
		::close(sockfd);
		return -1;
	}
	num = conf.ifc_len / sizeof(struct ifreq);
	ifr = conf.ifc_req;

	for (int i = 0; i < num; ++i) {
		struct sockaddr_in* sin = (struct sockaddr_in*)(&ifr->ifr_addr);
		if (::ioctl(sockfd, SIOCGIFFLAGS, ifr) < 0) {
			::close(sockfd);
			return -1;
		}
		if ((ifr->ifr_flags & IFF_UP) &&
			strcmp(netName.c_str(), ifr->ifr_name) == 0) {
			strIP = ::inet_ntoa(sin->sin_addr);
			::close(sockfd);
			return 0;
		}

		++ifr;
	}
	::close(sockfd);
	return -1;
}

#endif