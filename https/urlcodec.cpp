
#include "urlcodec.h"
#include <assert.h>

namespace URL {
	static char CharToInt(char ch)
	{
		if (ch >= '0' && ch <= '9')
		{
			return (char)(ch - '0');
		}
		if (ch >= 'a' && ch <= 'f')
		{
			return (char)(ch - 'a' + 10);
		}
		if (ch >= 'A' && ch <= 'F')
		{
			return (char)(ch - 'A' + 10);
		}
		return -1;
	}

	static char StrToBin(char* pString)
	{
		char szBuffer[2];
		char ch;
		szBuffer[0] = CharToInt(pString[0]); //make the B to 11 -- 00001011 
		szBuffer[1] = CharToInt(pString[1]); //make the 0 to 0 -- 00000000 
		ch = (szBuffer[0] << 4) | szBuffer[1]; //to change the BO to 10110000 
		return ch;
	}
	
	static unsigned char ToHex(unsigned char x)
	{
		return  x > 9 ? x + 55 : x + 48;
	}

	static unsigned char FromHex(unsigned char x)
	{
		unsigned char y;
		if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
		else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
		else if (x >= '0' && x <= '9') y = x - '0';
		else assert(0);
		return y;
	}

#if 0
	std::string Encode2(const std::string& str)
	{
		std::string strTemp = "";
		size_t length = str.length();
		for (size_t i = 0; i < length; i++)
		{
			if (isalnum((unsigned char)str[i]) ||
				(str[i] == '-') ||
				(str[i] == '_') ||
				(str[i] == '.') ||
				(str[i] == '~'))
				strTemp += str[i];
			else if (str[i] == ' ')
				strTemp += "+";
			else
			{
				strTemp += '%';
				strTemp += ToHex((unsigned char)str[i] >> 4);
				strTemp += ToHex((unsigned char)str[i] % 16);
			}
		}
		return strTemp;
	}

	std::string Decode2(const std::string& str)
	{
		std::string strTemp = "";
		size_t length = str.length();
		for (size_t i = 0; i < length; i++)
		{
			if (str[i] == '+') strTemp += ' ';
			else if (str[i] == '%')
			{
				assert(i + 2 < length);
				unsigned char high = FromHex((unsigned char)str[++i]);
				unsigned char low = FromHex((unsigned char)str[++i]);
				strTemp += high * 16 + low;
			}
			else strTemp += str[i];
		}
		return strTemp;
	}
#endif

	static std::string Encode_2(const std::string& str)
	{
		std::string result;
		size_t nLength = str.length();
		unsigned char* pBytes = (unsigned char*)str.c_str();
		char szAlnum[2];
		char szOther[4];
		for (size_t i = 0; i < nLength; i++)
		{
			if (isalnum((char)str[i]))
			{
				snprintf(szAlnum, sizeof(szAlnum), "%c", str[i]);
				result.append(szAlnum);
			}
			else if (isspace((char)str[i]))
			{
				result.append("+");
			}
			else
			{
				snprintf(szOther, sizeof(szOther), "%%%X%X", pBytes[i] >> 4, pBytes[i] % 16);
				result.append(szOther);
			}
		}
		return result;
	}

	// 1-2重编码
	static int32_t Decode_(const std::string& str, std::string& result)
	{
		result.clear();
		int ret = 0;
		char szTemp[2];
		size_t i = 0;
		size_t nLength = str.length();
		while (i < nLength) {
			if (str[i] == '%') {
				szTemp[0] = str[i + 1];
				szTemp[1] = str[i + 2];
				// 做两重url解密
				char ch = StrToBin(szTemp);
				if (ch == '%') {
					szTemp[0] = str[i + 3];
					szTemp[1] = str[i + 4];
					ch = StrToBin(szTemp);

					result += ch;
					i = i + 5;

					// 还有多重编码
					if (ch == '%') {
						ret = 1;
					}
				}
				else {
					result += StrToBin(szTemp);
					i = i + 3;
				}
			}
			else if (str[i] == '+') {
				result += ' ';
				++i;
			}
			else {
				result += str[i];
				++i;
			}
		}
		return ret;
	}

	// 防止多重编码
	static std::string Decode_2(const std::string& str) {
		std::string result;
		if (URL::Decode_(str, result) == 1) {
			std::string temp;
			do {
				temp = result;
			} while (URL::Decode_(temp, result) == 1);
		}
		return result;
	}

	static std::string Encode_1(const std::string &d)
	{
#define _INLINE_C_2_H(x) (unsigned char)(x > 9 ? x + 55 : x + 48)
		std::string e = "";
		std::size_t size = d.size();
		for (std::size_t i = 0; i < size; ++i) {
			if (isalnum((unsigned char)d.at(i)) ||
				(d.at(i) == '-') ||
				(d.at(i) == '_') ||
				(d.at(i) == '.') ||
				(d.at(i) == '~')) {
				e.append(1, d.at(i));
			}
			else if (d.at(i) == ' ') {
				e.append(1, '+');
			}
			else {
				e.append(1, '%');
				e.append(1, _INLINE_C_2_H((unsigned char)d.at(i) >> 4));
				e.append(1, _INLINE_C_2_H((unsigned char)d.at(i) % 16));
			}
		}
		return e;
	}

	static std::string Decode_1(const std::string &e)
	{
#define __INLINE_H_2_C(x) ((x >= 'A' && x <= 'Z') ? (x - 'A' + 10) : ((x >= 'a' && x <= 'z') ? (x - 'a' + 10) : ((x >= '0' && x <= '9') ? (x - '0') : x)))
		std::string d = "";
		std::size_t size = e.size();
		for (std::size_t i = 0; i < size; ++i) {
			if (e.at(i) == '+') {
				d.append(1, ' ');
			}
			else if (e.at(i) == '%') {
				if (i + 2 < size) {
					d.append(1, (unsigned char)(__INLINE_H_2_C((unsigned char)e.at(i + 1)) * 16 + __INLINE_H_2_C((unsigned char)e.at(i + 2))));
					i += 2;
				}
				else {
					for (; i < size; ++i) {
						d.append(1, e.at(i));
					}
				}
			}
			else {
				d.append(1, e.at(i));
			}
		}
		return d;
	}

	std::string Encode(const std::string& str) {
		return Encode_1(str);
	}
	
	std::string Decode(const std::string& str) {
		return Decode_1(str);
	}

	std::string MultipleEncode(const std::string& str) {
		return Encode_2(str);
	}
	
	std::string MultipleDecode(const std::string& str) {
		return Decode_2(str);
	}
}