#ifndef MYAESECB_H
#define MYAESECB_H

#include <openssl/rsa.h>
#include <openssl/aes.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/des.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <chrono>
#include <vector>

#include "base64.h"

#include "AES_Cipher.h"

#define KEYCODELENGTH 16

namespace Crypto {
	static void Padding(unsigned char* pSrc, int nSrcLen)
	{
		if (nSrcLen < KEYCODELENGTH)
		{
			unsigned char ucPad = KEYCODELENGTH - nSrcLen;
			for (int nID = KEYCODELENGTH; nID > nSrcLen; --nID)
			{
				pSrc[nID - 1] = ucPad;
			}
		}
	}

	static void StringToHex(const char* pSrc, unsigned char* pDest, int nDestLen)
	{
		int nSrcLen = 0;
		if (pSrc != 0)
		{
			nSrcLen = strlen(pSrc);
			memcpy(pDest, pSrc, nSrcLen > nDestLen ? nDestLen : nSrcLen);
		}
		Padding(pDest, nSrcLen > nDestLen ? nDestLen : nSrcLen);
	}

	//AES_ECBEncrypt AES加密 ///
	static std::string AES_ECBEncrypt(std::string const& data, std::string const& key)
	{
		AES_KEY ctx = { 0 };
		const char* pSrc = 0;
		const char* pTmpSrc = 0;
		unsigned char* pDest = 0;
		unsigned char* pTmpDest = 0;
		int nSrcLen = 0;
		int nDestLen = 0;
		unsigned char buf[KEYCODELENGTH];
		unsigned char KEY[KEYCODELENGTH];
		StringToHex(key.c_str(), KEY, KEYCODELENGTH);
		if (AES_set_encrypt_key((unsigned char const*)KEY, 128, &ctx) < 0) {
			printf("AES_set_encrypt_key error\n");
			return "";
		}
		pSrc = data.c_str();
		nSrcLen = data.size();
		nDestLen = (nSrcLen / KEYCODELENGTH) * KEYCODELENGTH + KEYCODELENGTH;
		pDest = new unsigned char[nDestLen];
		memset(pDest, 0, nDestLen);
		pTmpSrc = pSrc;
		pTmpDest = pDest;
		while ((pTmpSrc - pSrc) < nSrcLen) {
			StringToHex(pTmpSrc, buf, KEYCODELENGTH);
			AES_encrypt((const unsigned char*)buf, buf, &ctx);
			memcpy(pTmpDest, buf, KEYCODELENGTH);
			pTmpSrc += KEYCODELENGTH;
			pTmpDest += KEYCODELENGTH;
		}
		if ((pTmpDest - pDest) < nDestLen) {	// if the source size % KEYCODELENGTH == 0 then need to add an extra buffer 
			StringToHex(0, buf, KEYCODELENGTH);
			AES_encrypt((const unsigned char*)buf, buf, &ctx);
			memcpy(pTmpDest, buf, KEYCODELENGTH);
		}
		std::string strRet = Base64::Encode(pDest, nDestLen);
		delete[] pDest;
		return strRet;
	}

	//AES_ECBDecrypt AES解密 ///
	static std::string AES_ECBDecrypt(std::string const& data, std::string const& key)
	{
#if 0
		AES_KEY ctx = { 0 };
		const char* pSrc = 0;
		const char* pTmpSrc = 0;
		unsigned char* pDest = 0;
		unsigned char* pTmpDest = 0;
		int nSrcLen = 0;
		int nDestLen = 0;
		unsigned char buf[KEYCODELENGTH];
		//unsigned char KEY[KEYCODELENGTH];
		//StringToHex(key.c_str(), KEY, KEYCODELENGTH);
		if (AES_set_decrypt_key((unsigned char const*)key.c_str(), 128, &ctx) < 0) {
			printf("AES_set_decrypt_key error\n");
			return "";
		}
		std::string strSrcHex = Base64::Decode(data);
		pSrc = strSrcHex.data();
		nDestLen = strSrcHex.length();
		pDest = new unsigned char[nDestLen];
		memset(pDest, 0, nDestLen);
		pTmpSrc = pSrc;
		pTmpDest = pDest;
		while ((pTmpSrc - pSrc) < strSrcHex.length()) {
			memcpy(buf, pTmpSrc, KEYCODELENGTH);
			AES_decrypt((const unsigned char*)buf, buf, &ctx);
			memcpy(pTmpDest, buf, KEYCODELENGTH);
			pTmpSrc += KEYCODELENGTH;
			pTmpDest += KEYCODELENGTH;
		}
		unsigned char ucTest = 0;
		while (ucTest = *(pTmpDest - 1)) {
			if (ucTest > 0 && ucTest <= 0x10)
				*(pTmpDest - 1) = 0;
			else
				break;
			pTmpDest--;
		}
		std::string strRet = (char*)pDest;
		delete[] pDest;
		return strRet;
#else
		unsigned char dst[1024] = { 0 };
		uint32_t len = sizeof dst;
		std::string strSrcHex = Base64::Decode(data);
		if (!strSrcHex.empty()) {
			cipher::AES_ECB_Cipher cipher((const unsigned char*)key.c_str(), false);
			if (cipher.decrypt(
				(const unsigned char*)strSrcHex.c_str(),
				strSrcHex.length(), dst, len) < 0) {
				return "";
			}
		}
		return std::string((const char*)dst);
#endif
	}

	char* PKCS5Padding(std::vector<char>& buf, std::string const& data, uint32_t unBlockSize)
	{
		int nRaw_size = data.size();
		int i = 0, j = nRaw_size / 8 + 1, k = nRaw_size % 8;
		int nPidding_size = 8 - k;
		buf.clear();
		buf.resize(nRaw_size + nPidding_size);
		memcpy(&buf.front(), data.c_str(), nRaw_size);
		for (int i = nRaw_size; i < (nRaw_size + nPidding_size); i++)
		{
			// PKCS5Padding 算法：
			buf[i] = nPidding_size;
		}
	}

	std::string Des_Encrypt(std::string const& data, char* key)
	{
		char szCommand[1024] = { 0 };
		memset(szCommand, 0, 1024);
		int nRaw_size = data.size();
		std::vector<char> buf;
		PKCS5Padding(buf, data, sizeof(DES_cblock));
		int j = nRaw_size / 8 + 1;
		DES_cblock KEY;// = "12341234";
		memcpy(KEY, key, 8);
		const_DES_cblock sInput;
		DES_cblock sOutput;
		DES_key_schedule sSchedule;
		//转换成schedule
		DES_set_key_unchecked(&KEY, &sSchedule);
		for (int i = 0; i < j; ++i)
		{
			memset(sInput, 0, 8);
			memset(sOutput, 0, 8);
			memcpy(sInput, &buf.front() + (i * 8), 8);
			DES_ecb_encrypt(&sInput, &sOutput, &sSchedule, DES_ENCRYPT);
			memcpy(szCommand + (i * 8), sOutput, 8);
		}
		int nLen = j * 8;
		//base64编码
		return Base64::Encode(reinterpret_cast<const unsigned char*>(szCommand), nLen);
	}

	std::string Des_Decrypt(std::string const& data, const char* key)
	{
		//base64解码
		std::string strSrcHex = Base64::Decode(data);

		std::vector<unsigned char> vecCleartext;
		DES_cblock KEY;
		memcpy(KEY, key, 8);
		DES_key_schedule sSchedule;
		//转换成schedule
		DES_set_key_unchecked(&KEY, &sSchedule);
		const_DES_cblock sInput;
		DES_cblock sOutput;
		int nRaw_size = strSrcHex.length();
		int j = nRaw_size / 8;
		const char* szData = strSrcHex.c_str();
		//解密
		unsigned char szTmp[8];
		for (int i = 0; i < j; ++i)
		{
			memset(sInput, 0, 8);
			memset(sOutput, 0, 8);
			memcpy(sInput, szData + (i * 8), 8);
			DES_ecb_encrypt(&sInput, &sOutput, &sSchedule, DES_DECRYPT);
			for (int k = 0; k < 8; k++)
				vecCleartext.push_back(sOutput[k]);
		}
		//填充数量
		int nPadNum = vecCleartext[nRaw_size - 1];
		std::string strRet;
		strRet.assign(vecCleartext.begin(), vecCleartext.end() - nPadNum);
		return strRet;
	}
}

#endif