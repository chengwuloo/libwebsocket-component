#ifndef SUBNETIP_H
#define SUBNETIP_H

#ifndef _WIN32
#include <unistd.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string>

//转换IP
static void toIp_c(char *buf, size_t size, sockaddr const* addr)
{
	// char *inet_ntoa(struct in_addr in);
	if (AF_INET == addr->sa_family) {
		if (NULL == ::inet_ntop(AF_INET, &((sockaddr_in*)addr)->sin_addr, buf, size)) {
			assert(false);
		}
	}
	else if (AF_INET6 == addr->sa_family) {
		if (NULL == ::inet_ntop(AF_INET6, &((sockaddr_in6*)addr)->sin6_addr, buf, size))
			assert(false);
	}
}

//转换IP
static std::string toIp(sockaddr const* addr)
{
	char buf[64] = { 0 };
	toIp_c(buf, sizeof(buf), addr);
	return buf;
}

//转换IP
static std::string Inet2Ipstr(uint32_t inetIp) {
	sockaddr_in inaddr = { 0 };
	inaddr.sin_family = AF_INET;
	inaddr.sin_addr.s_addr = in_addr_t(inetIp);
	return toIp((sockaddr *)&inaddr);
}

//转换IP
static std::string Hnet2Ipstr(uint32_t hnetIp) {
	//主机字节序转换网络字节序
	return Inet2Ipstr(htonl(hnetIp));
}

//判断srcIpstr与dstIpstr是否在同一个子网，比较 x.y.z.0 与 a.b.c.0
static bool CheckSubnetIpstr(char const* srcIpstr, char const* dstIpstr) {
	////////////////////////////////////////
	//ipstr -> 网络字节序 -> 主机字节序
	//192.168.161.12 -> inet_pton -> 0XCA1A8C0 -> ntohl -> 0XC0A8A10C
	//11000000 10101000 10100001 00001100
	////////////////////////////////////////
	//192.168.160.2  -> inet_pton -> 0X2A0A8C0 -> ntohl -> 0XC0A8A002
	//11000000 10101000 10100000 00000010
	////////////////////////////////////////
	//255.255.255.0  -> inet_pton -> 0X00FFFFFF -> ntohl -> 0XFFFFFF00
	//11111111 11111111 11111111 00000000
	sockaddr_in sinaddr, dinaddr, minaddr;
	//char const* srcIpstr = "192.168.161.12";
	if (inet_pton(AF_INET, srcIpstr, &sinaddr.sin_addr) < 1) {
		printf("inet_pton error:%s\n", srcIpstr);
		assert(false);
	}
	//char const* dstIpstr = "192.168.160.2";
	if (inet_pton(AF_INET, dstIpstr, &dinaddr.sin_addr) < 1) {
		printf("inet_pton error:%s\n", dstIpstr);
		assert(false);
	}
	char const* maskIpstr = "255.255.255.0";
	if (inet_pton(AF_INET, maskIpstr, &minaddr.sin_addr) < 1) {
		printf("inet_pton error:%s\n", maskIpstr);
		assert(false);
	}
	//11000000 10101000 10100001 00001100
	uint32_t sinetaddr = ntohl(sinaddr.sin_addr.s_addr);
	//11000000 10101000 10100000 00000010
	uint32_t dinetaddr = ntohl(dinaddr.sin_addr.s_addr);
	//11111111 11111111 11111111 00000000
	uint32_t minetaddr = ntohl(minaddr.sin_addr.s_addr);
	//比较 x.y.z.0 与 a.b.c.0
	uint32_t srcmask = sinetaddr & minetaddr;
	uint32_t dstmask = dinetaddr & minetaddr;
	if (srcmask == dstmask) {
		printf("\n\n=======================================================\n");
		printf("same subnet: %#X:%s & %#X:%s\n\n", sinetaddr, srcIpstr, dinetaddr, dstIpstr);
	}
	else {
		printf("\n\n=======================================================\n");
		printf("different subnet: %#X:%s & %#X:%s\n\n", sinetaddr, srcIpstr, dinetaddr, dstIpstr);
	}
	return srcmask == dstmask;
}

static bool CheckSubnetInetIp(uint32_t srcInetIp, uint32_t dstInetIp) {
	sockaddr_in sinaddr, dinaddr, minaddr;
	char const* maskIpstr = "255.255.255.0";
	if (inet_pton(AF_INET, maskIpstr, &minaddr.sin_addr) < 1) {
		printf("inet_pton error:%s\n", maskIpstr);
		assert(false);
	}
	//11000000 10101000 10100001 00001100
	uint32_t sinetaddr = ntohl(in_addr_t(srcInetIp));
	//11000000 10101000 10100000 00000010
	uint32_t dinetaddr = ntohl(in_addr_t(dstInetIp));
	//11111111 11111111 11111111 00000000
	uint32_t minetaddr = ntohl(minaddr.sin_addr.s_addr);
	//比较 x.y.z.0 与 a.b.c.0
	uint32_t srcmask = sinetaddr & minetaddr;
	uint32_t dstmask = dinetaddr & minetaddr;
	char const* srcIpstr = Inet2Ipstr(srcInetIp).c_str();
	char const* dstIpstr = Inet2Ipstr(dstInetIp).c_str();
	if (srcmask == dstmask) {
		printf("\n\n=======================================================\n");
		printf("same subnet: %#X:%s & %#X:%s\n\n", sinetaddr, srcIpstr, dinetaddr, dstIpstr);
	}
	else {
		printf("\n\n=======================================================\n");
		printf("different subnet: %#X:%s & %#X:%s\n\n", sinetaddr, srcIpstr, dinetaddr, dstIpstr);
	}
	return srcmask == dstmask;
}

#endif