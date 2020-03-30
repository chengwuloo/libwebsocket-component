
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

	std::string Encode(const std::string& str)
	{
		std::string strResult;
		size_t nLength = str.length();
		unsigned char* pBytes = (unsigned char*)str.c_str();
		char szAlnum[2];
		char szOther[4];
		for (size_t i = 0; i < nLength; i++)
		{
			if (isalnum((char)str[i]))
			{
				snprintf(szAlnum, sizeof(szAlnum), "%c", str[i]);
				strResult.append(szAlnum);
			}
			else if (isspace((char)str[i]))
			{
				strResult.append("+");
			}
			else
			{
				snprintf(szOther, sizeof(szOther), "%%%X%X", pBytes[i] >> 4, pBytes[i] % 16);
				strResult.append(szOther);
			}
		}
		return strResult;
	}

	std::string Decode(const std::string& str)
	{
		std::string strResult;
		char szTemp[2];
		size_t i = 0;
		size_t nLength = str.length();
		while (i < nLength)
		{
			if (str[i] == '%')
			{
				szTemp[0] = str[i + 1];
				szTemp[1] = str[i + 2];
				strResult += StrToBin(szTemp);
				i = i + 3;
			}
			else if (str[i] == '+')
			{
				strResult += ' ';
				i++;
			}
			else
			{
				strResult += str[i];
				i++;
			}
		}
		return strResult;
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
}