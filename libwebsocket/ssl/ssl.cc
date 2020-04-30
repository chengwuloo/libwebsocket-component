/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.03.2020                                           */
/************************************************************************/

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

#include <libwebsocket/ssl.h>

#define atom_incr(i) __sync_add_and_fetch(&i, 1)
#define atom_decr(i) __sync_add_and_fetch(&i, -1)
#define atom_get(i)  __sync_val_compare_and_swap(&i, 0, 0)

//#define MEMORYLEAK_DETECT

namespace muduo {
	namespace net {
		namespace ssl {

#ifdef MEMORYLEAK_DETECT
			static volatile int32_t s_c_;
#endif
			std::mutex s_mutex_;

			//@@ GuardLock
			class GuardLock {
			public:
				GuardLock(std::mutex& lock):lock_(lock) {
					lock_.lock();
				}
				~GuardLock() {
					lock_.unlock();
				}
			private:
				std::mutex& lock_;
			};

			static inline void SSL_free_(SSL*& ssl) {
				if (ssl) {
#ifdef MEMORYLEAK_DETECT
					//atomic decr
					atom_decr(s_c_);
					printf("SSL_free c = %d\n", atom_get(s_c_));
#endif
					//shutdown：SSL_shutdown
					::SSL_shutdown(ssl);
					//free：SSL_free
					::SSL_free(ssl);
					ssl = NULL;
				}
			}

			//SSL_free
			void SSL_free(SSL*& ssl) {
				GuardLock lock(s_mutex_);
				SSL_free_(ssl);
			}

			static inline bool SSL_handshake_(SSL_CTX* ctx, SSL*& ssl, int sockfd, int& saveErrno) {
				if (ctx) {
#ifdef LIBWEBSOCKET_DEBUG
					printf("-----------------------------------------------------------------------------\n");
#endif
#if 0
					//create the SSL：SSL_new
					if ((ssl = ::SSL_new(ctx)) == NULL) {
						printf("SSL_new failed\n");
						return false;
					}
					//create a BIO：BIO_new_socket
					BIO* bio = ::BIO_new_socket(sockfd, BIO_NOCLOSE);
					if (!bio) {
						printf("BIO_new_socket failed\n");
						muduo::net::ssl::SSL_free_(ssl);
						return false;
					}
					//set the BIO：SSL_set_bio
					::SSL_set_bio(ssl, bio, bio);
					int rc;
					//accept：SSL_accept
					if ((rc = ::SSL_accept(ssl)) != 1) {
						saveErrno = ::SSL_get_error(ssl, rc);
						//握手失败
						switch (saveErrno) {
						case SSL_ERROR_SSL:
							printf("SSL_accept SSL_ERROR_SSL\n");
							muduo::net::ssl::SSL_free_(ssl);
							break;
						case SSL_ERROR_WANT_READ:
							printf("SSL_accept SSL_ERROR_WANT_READ\n");
							break;
						case SSL_ERROR_WANT_WRITE:
							printf("SSL_accept SSL_ERROR_WANT_WRITE\n");
							break;
						case SSL_ERROR_WANT_X509_LOOKUP:
							printf("SSL_accept SSL_ERROR_WANT_X509_LOOKUP\n");
							muduo::net::ssl::SSL_free_(ssl);
							break;
						case SSL_ERROR_SYSCALL:
							printf("SSL_accept SSL_ERROR_SYSCALL\n");
							muduo::net::ssl::SSL_free_(ssl);
							break;
						case SSL_ERROR_ZERO_RETURN:
							printf("SSL_accept SSL_ERROR_ZERO_RETURN\n");
							break;
						case SSL_ERROR_WANT_CONNECT:
							printf("SSL_accept SSL_ERROR_WANT_CONNECT\n");
							break;
						case SSL_ERROR_WANT_ACCEPT:
							printf("SSL_accept SSL_ERROR_WANT_ACCEPT\n");
							break;
						default:
							printf("SSL_accept failed\n");
							muduo::net::ssl::SSL_free_(ssl);
							break;
						}
						return false;
					}
					printf("SSL_accept succ(version \"%s\" cipher:\"%s\")\n",
						SSL_get_version(ssl), SSL_get_cipher_name(ssl));
					//握手成功
					return true;
#else
					if (!ssl) {
						//create the SSL：SSL_new
						if ((ssl = ::SSL_new(ctx)) == NULL) {
							printf("SSL_new failed\n");
							return false;
						}
#ifdef MEMORYLEAK_DETECT
						//atomic incr
						atom_incr(s_c_);
#endif
#if 0
						//SSL_set_mode
						SSL_set_mode(ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
#endif
						//SSL_set_fd
						if (::SSL_set_fd(ssl, sockfd) == 0) {
							printf("SSL_set_fd failed\n");
							muduo::net::ssl::SSL_free_(ssl);
							return false;
						}
#if 1
						//server
						::SSL_set_accept_state(ssl);
#else
						//client
						::SSL_set_connect_state(ssl);
#endif
					}
#if 0
					//create a BIO：BIO_new_fd
					BIO* bio = ::BIO_new_fd(2, BIO_NOCLOSE);
					if (!bio) {
						printf("BIO_new_fd failed\n");
						muduo::net::ssl::SSL_free_(ssl);
						return false;
					}
#endif
					int rc;
					//accept：SSL_do_handshake
					if ((rc = ::SSL_do_handshake(ssl)) != 1) {
						saveErrno = ::SSL_get_error(ssl, rc);
						//握手失败
						switch (saveErrno) {
						case SSL_ERROR_SSL:
#ifdef LIBWEBSOCKET_DEBUG
							printf("SSL_do_handshake SSL_ERROR_SSL\n");
#endif
							muduo::net::ssl::SSL_free_(ssl);
							break;
							//SSL需要在非阻塞socket可读时读入数据
						case SSL_ERROR_WANT_READ:
#ifdef LIBWEBSOCKET_DEBUG
							printf("SSL_do_handshake SSL_ERROR_WANT_READ\n");
#endif
							break;
							//SSL需要在非阻塞socket可写时写入数据
						case SSL_ERROR_WANT_WRITE:
#ifdef LIBWEBSOCKET_DEBUG
							printf("SSL_do_handshake SSL_ERROR_WANT_WRITE\n");
#endif
							break;
						case SSL_ERROR_WANT_X509_LOOKUP:
#ifdef LIBWEBSOCKET_DEBUG
							printf("SSL_do_handshake SSL_ERROR_WANT_X509_LOOKUP\n");
#endif
							muduo::net::ssl::SSL_free_(ssl);
							break;
						case SSL_ERROR_SYSCALL:
#ifdef LIBWEBSOCKET_DEBUG
							printf("SSL_do_handshake SSL_ERROR_SYSCALL\n");
#endif
							muduo::net::ssl::SSL_free_(ssl);
							break;
						case SSL_ERROR_ZERO_RETURN:
#ifdef LIBWEBSOCKET_DEBUG
							printf("SSL_do_handshake SSL_ERROR_ZERO_RETURN\n");
#endif
							break;
						case SSL_ERROR_WANT_CONNECT:
#ifdef LIBWEBSOCKET_DEBUG
							printf("SSL_do_handshake SSL_ERROR_WANT_CONNECT\n");
#endif
							break;
						case SSL_ERROR_WANT_ACCEPT:
#ifdef LIBWEBSOCKET_DEBUG
							printf("SSL_do_handshake SSL_ERROR_WANT_ACCEPT\n");
#endif
							break;
						default:
#if 0
							::ERR_print_errors(bio);
							::BIO_free(bio);
#endif
#ifdef LIBWEBSOCKET_DEBUG
							printf("SSL_do_handshake failed\n");
#endif
							muduo::net::ssl::SSL_free_(ssl);
							break;
						}
						return false;
					}
#ifdef LIBWEBSOCKET_DEBUG
					printf("SSL_do_handshake succ(version \"%s\" cipher:\"%s\")\n",
						SSL_get_version(ssl), SSL_get_cipher_name(ssl));
#endif
					//握手成功
					return true;
#endif
				}
				assert(false);
				return false;
			}

			//SSL_handshake
			bool SSL_handshake(SSL_CTX* ctx, SSL*& ssl, int sockfd, int& saveErrno) {
				GuardLock lock(s_mutex_);
				return SSL_handshake_(ctx, ssl, sockfd, saveErrno);
			}

			static inline void* my_zeroing_malloc(size_t howmuch) {
				return calloc(1, howmuch);
			}

			static SSL_CTX* ssl_ctx_;

			//SSL_library_init
			//static inline void SSL_library_init();

			//SSL_library_free
			//static inline void SSL_library_free();

			//SSL_CTX_create
			//static inline bool SSL_CTX_create();

			//SSL_CTX_setup_certs 加载CA证书
			static inline void SSL_CTX_setup_certs(
				std::string const& cert_path,
				std::string const& private_key_path,
				std::string const& client_ca_cert_file_path,
				std::string const& client_ca_cert_dir_path);

			//SSL_CTX_Init
			void SSL_CTX_Init(
				std::string const& cert_path,
				std::string const& private_key_path,
				std::string const& client_ca_cert_file_path,
				std::string const& client_ca_cert_dir_path) {
				ssl::SSL_CTX_setup_certs(
					cert_path,
					private_key_path,
					client_ca_cert_file_path,
					client_ca_cert_dir_path);
			}

			//SSL_library_init
			static inline void SSL_library_init() {
#if OPENSSL_VERSION_NUMBER < 0x1010001fL
				CRYPTO_set_mem_functions(my_zeroing_malloc, realloc, free);
				//OPENSSL_config(NULL);
				::SSL_library_init();
				::SSL_load_error_strings();
				OpenSSL_add_all_algorithms();
				printf("Using OpenSSL version \"%s\"\n", ::SSLeay_version(SSLEAY_VERSION));
#else
				if (OPENSSL_init_ssl(
					/*OPENSSL_INIT_LOAD_CONFIG |*/
					OPENSSL_INIT_LOAD_SSL_STRINGS |
					OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL) == 0) {
					printf("OPENSSL_init_ssl error\n");
					return;
				}
				::ERR_clear_error();
				printf("OPENSSL_init_ssl\n");
#endif
			}

			//SSL_library_free
			static inline void SSL_library_free() {
#if OPENSSL_VERSION_NUMBER < 0x1010001fL
				::ERR_free_strings();
#endif
			}

			//SSL_CTX_Get
			SSL_CTX* SSL_CTX_Get() {
				//assert(ssl_ctx_);
				return ssl_ctx_;
			}

			//SSL_CTX_create
			static inline bool SSL_CTX_create() {
				if (!ssl_ctx_) {
					//openSSL库初始化 ///
					ssl::SSL_library_init();
					//////////////////////////////////////////////////////////////////////////
					//SSLv2版本 SSLv2_server_method SSLv2_client_method
					//SSL/TLS版本 SSLv23_server_method SSLv23_client_method
					//SSLv3版本 SSLv3_server_method SSLv3_client_method
					//
					//TLSv1.0版本 TLSv1_server_method TLSv1_client_method
					//TLSv1.1版本 TLSv1_1_server_method TLSv1_1_client_method
					//TLSv1.2版本 TLSv1_2_server_method TLSv1_2_client_method
					//
					//DTLSv1.0版本 DTLSv1_server_method DTLSv1_client_method
					//DTLSv1.2版本 DTLSv1_2_server_method DTLSv1_2_client_method
					//
					//DTLS1.0/1.2版本 DTLS_server_method DTLS_client_method
					//////////////////////////////////////////////////////////////////////////
					//创建SSL_CTX
					ssl_ctx_ = ::SSL_CTX_new(SSLv23_server_method());
					if (!ssl_ctx_) {
						printf("SSL_CTX_new failed\n");
						ssl::SSL_library_free();
						return false;
					}
#if 1
					//指定SSL_CTX可选项，禁止使用指定协议建链
					SSL_CTX_set_options(ssl_ctx_,
						SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
						SSL_OP_NO_COMPRESSION |
						SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
#elif 0
					SSL_CTX_set_options(ssl_ctx_,
						SSL_OP_NO_SSLv2 |
						SSL_OP_SINGLE_DH_USE | SSL_OP_SINGLE_ECDH_USE);
#elif 0
					SSL_CTX_set_options(ssl_ctx_,
						SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
						SSL_OP_NO_COMPRESSION |
						SSL_OP_SINGLE_DH_USE | SSL_OP_SINGLE_ECDH_USE |
						SSL_OP_CIPHER_SERVER_PREFERENCE);

					char* ssl_cipher_list =
						"ECDHE-ECDSA-AES256-GCM-SHA384:"
						"ECDHE-RSA-AES256-GCM-SHA384:"
						"DHE-RSA-AES256-GCM-SHA384:"
						"ECDHE-RSA-AES256-SHA384:"
						"HIGH:!aNULL:!eNULL:!EXPORT:"
						"!DES:!MD5:!PSK:!RC4:!HMAC_SHA1:"
						"!SHA1:!DHE-RSA-AES128-GCM-SHA256:"
						"!DHE-RSA-AES128-SHA256:"
						"!AES128-GCM-SHA256:"
						"!AES128-SHA256:"
						"!DHE-RSA-AES256-SHA256:"
						"!AES256-GCM-SHA384:"
						"!AES256-SHA256";
					::SSL_CTX_set_cipher_list(ssl_ctx_, ssl_cipher_list);
#endif
#if 1
					/* Cheesily pick an elliptic curve to use with elliptic curve ciphersuites.
						* We just hardcode a single curve which is reasonably decent.
						* See http://www.mail-archive.com/openssl-dev@openssl.org/msg30957.html */
					EC_KEY* ecdh = ::EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
					if (!ecdh) {
						printf("EC_KEY_new_by_curve_name error\n");
						ssl::SSL_CTX_free();
						return false;
					}
					if (SSL_CTX_set_tmp_ecdh(ssl_ctx_, ecdh) != 1) {
						printf("SSL_CTX_set_tmp_ecdh error\n");
						ssl::SSL_CTX_free();
						return false;
					}
					::EC_KEY_free(ecdh);
#endif
				}
				assert(ssl_ctx_);
				return true;
			}

			//SSL_CTX_free
			void SSL_CTX_free() {
				if (ssl_ctx_) {
					::SSL_CTX_free(ssl_ctx_);
					ssl_ctx_ = NULL;
					ssl::SSL_library_free();
				}
			}

			//SSL_CTX_setup_certs 加载CA证书
			static inline void SSL_CTX_setup_certs(
				std::string const& cert_path,
				std::string const& private_key_path,
				std::string const& client_ca_cert_file_path,
				std::string const& client_ca_cert_dir_path) {
				if (cert_path.empty() || private_key_path.empty()) {
					//printf("SSL_CTX_setup_certs failed\n");
					return;
				}
				//SSL_CTX_create ///
				if (!ssl::SSL_CTX_create()) {
					return;
				}
				printf("Loading certificate-chain from '%s'\n" \
					"and private-key from '%s'\n",
					cert_path.c_str(), private_key_path.c_str());
#if 1
				//为SSL会话加载本应用的证书所属的证书链
				if (::SSL_CTX_use_certificate_chain_file(ssl_ctx_, cert_path.c_str()) != 1) {
					printf("SSL_CTX_use_certificate_chain_file failed\n");
					ssl::SSL_CTX_free();
					return;
				}
#else
				//为SSL会话加载本应用的证书*.cer
				if (::SSL_CTX_use_certificate_file(ssl_ctx_, cert_path.c_str(), SSL_FILETYPE_PEM) != 1) {
					printf("SSL_CTX_use_certificate_file failed\n");
					ssl::SSL_CTX_free();
					return;
				}
#endif
				std::string passwd("");
				::SSL_CTX_set_default_passwd_cb_userdata(ssl_ctx_, (void*)passwd.c_str());

				//为SSL会话加载本应用的私钥
				if (::SSL_CTX_use_PrivateKey_file(ssl_ctx_, private_key_path.c_str(), SSL_FILETYPE_PEM) != 1) {
					printf("SSL_CTX_use_PrivateKey_file failed\n");
					ssl::SSL_CTX_free();
					return;
				}
				//验证所加载的私钥和证书是否相匹配
				if (::SSL_CTX_check_private_key(ssl_ctx_) != 1) {
					printf("SSL_CTX_check_private_key failed\n");
					ssl::SSL_CTX_free();
					return;
				}
				if (!client_ca_cert_file_path.empty() || !client_ca_cert_dir_path.empty()) {
#if 0
					//需要客户端验证时，服务器把client_ca_cert_file_path里面的可信任CA证书发往客户端
					if (client_ca_cert_file_path) {
						stack_st_X509_NAME list = ::SSL_load_client_CA_file(client_ca_cert_file_path);
						::SSL_CTX_set_client_CA_list(ssl_ctx_, list);
					}
#endif
					//为SSL_CTX加载本应用受信任的CA证书
					::SSL_CTX_load_verify_locations(ssl_ctx_, client_ca_cert_file_path.c_str(), client_ca_cert_dir_path.c_str());

					//指定证书验证方式，验证对方证书
					::SSL_CTX_set_verify(
						ssl_ctx_,
						SSL_VERIFY_PEER |
						SSL_VERIFY_FAIL_IF_NO_PEER_CERT, // SSL_VERIFY_CLIENT_ONCE,
						NULL);
#if 0
					std::string sid_ctx = "134123322131##@adxd!";
					assert(sid_ctx.length() <= SSL_MAX_SSL_SESSION_ID_LENGTH);
					::SSL_CTX_set_session_id_context(ssl_ctx_,
						(uint8_t const*)sid_ctx.c_str(),
						std::min<size_t>(SSL_MAX_SSL_SESSION_ID_LENGTH, sid_ctx.length()));
#endif
				}
			}
		};//namespace ssl
	};//namespace net
};//namespace muduo