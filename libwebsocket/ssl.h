/************************************************************************/
/*    @author create by andy_ro@qq.com/Qwuloo@qq.com                    */
/*    @Date		   03.03.2020                                           */
/************************************************************************/
#ifndef _MUDUO_NET_SSL_H_
#define _MUDUO_NET_SSL_H_

//openssl
//https://www.zhihu.com/question/41717174
//https://segmentfault.com/a/1190000016855991
//https://github.com/yhirose/cpp-httplib/blob/master/httplib.h
//https://blog.csdn.net/zzhongcy/article/details/21989899
//https://blog.csdn.net/oj847935591/article/details/79362542
//http://openssl.6102.n7.nabble.com/SSL-connect-on-non-blocking-socket-Works-but-need-better-understanding-td25168.html
//https://www.jianshu.com/p/92afb46c4a7d
//https://mta.openssl.org/pipermail/openssl-users/2016-September/004430.html

//https://stackoverflow.com/questions/38755515/ssl-accept-with-edge-triggered-non-blocking-epoll-always-returns-ssl-error-want
//https://github.com/yedf/openssl-example/blob/master/async-ssl-svr.cc
//https://github.com/yedf/openssl-example/blob/master/async-ssl-cli.cc
//https://github.com/yedf/handy-ssl
//https://www.cnblogs.com/dongfuye/p/4121066.html

#ifdef __cplusplus
extern "C" {
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/md5.h>
#include <openssl/x509v3.h>

#ifdef __cplusplus
}
#endif

#include <assert.h>
#include <mutex>

#include <iomanip>
#include <sstream>

#include <libwebsocket/IBytesBuffer.h>

namespace muduo {
	namespace net {
		namespace ssl {
			
#ifdef __cplusplus
extern "C" {
#endif
			//SSL_read SSL读
			/*extern*/ ssize_t SSL_read(SSL* ssl, IBytesBuffer* buf, int* savedErrno);

			//SSL_write SSL写
			/*extern*/ ssize_t SSL_write(SSL* ssl, void const* data, size_t len, int* savedErrno);

			//SSL_free 释放SSL
			/*inline*/ void SSL_free(SSL*& ssl);

			//SSL_handshake SSL握手建链
			/*inline*/ bool SSL_handshake(SSL_CTX* ctx, SSL*& ssl, int sockfd, int& saveErrno);

#ifdef __cplusplus
}
#endif
			//@@ SSL_CTX_Init
			class SSL_CTX_Init {
			public:
				explicit SSL_CTX_Init(
					std::string const& cert_path,
					std::string const& private_key_path,
					std::string const& client_ca_cert_file_path = "",
					std::string const& client_ca_cert_dir_path = "");
				
				~SSL_CTX_Init();
				
				//SSL_CTX_Valid
				static /*inline*/ bool SSL_CTX_Valid();
				
				//SSL_CTX_Get
				static /*inline*/ SSL_CTX* SSL_CTX_Get();
				
			private:
				//SSL_library_init
				static void SSL_library_init();
				
				//SSL_library_free
				static void SSL_library_free();
				
			private:
				//SSL_CTX_create
				static /*inline*/ bool SSL_CTX_create();
				
				//SSL_CTX_free
				static /*inline*/ void SSL_CTX_free();
			
			public:
				//SSL_CTX_setup_certs 加载CA证书
				static /*inline*/ void SSL_CTX_setup_certs(
					std::string const& cert_path,
					std::string const& private_key_path,
					std::string const& client_ca_cert_file_path = "",
					std::string const& client_ca_cert_dir_path = "");
			};

		};//namespace ssl
	};//namespace net
}; //namespace muduo

#endif
