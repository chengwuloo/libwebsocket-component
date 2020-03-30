#ifndef MONGODB_CLIENT_H
#define MONGODB_CLIENT_H

#include <muduo/base/Mutex.h>  // MCHECK

#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <memory>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/config/config.hpp>
#include <mongocxx/options/client.hpp>
#include <mongocxx/options/client_session.hpp>

namespace MongoDBClient {

	//@@ ThreadLocalSingleton 线程局部使用
	class ThreadLocalSingleton : boost::noncopyable {
	public:
		ThreadLocalSingleton() = delete;
		~ThreadLocalSingleton() = delete;
		//instance
		static mongocxx::client& instance() {
			if (!s_value_) {
				s_value_ = new mongocxx::client(mongocxx::uri{ s_mongoDBConntionString_ }, s_options_);
				s_deleter_.set(s_value_);
			}
			return *s_value_;
		}
		//reset() 抛连接断开异常时调用
		static void reset() {
			s_deleter_.reset();
			s_value_ = 0;
		}
		//pointer
		static mongocxx::client* pointer() {
			return s_value_;
		}
		//setUri
		static void setUri(std::string const& mongoDBStr) { s_mongoDBConntionString_ = mongoDBStr; }
		//setOption
		static void setOption(mongocxx::options::client options) { s_options_ = options; }
	private:
		//destructor
		static void destructor(void* obj) {
			assert(obj == s_value_);
			typedef char T_must_be_complete_type[sizeof(mongocxx::client) == 0 ? -1 : 1];
			T_must_be_complete_type dummy; (void)dummy;
			delete s_value_;
			s_value_ = 0;
		}
		//@@ Deleter
		class Deleter {
		public:
			Deleter() {
				pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);
			}
			~Deleter() {
				pthread_key_delete(pkey_);
			}
			//reset
			void reset() {
				pthread_key_delete(pkey_);
			}
			//set
			void set(mongocxx::client* newObj) {
				assert(pthread_getspecific(pkey_) == NULL);
				pthread_setspecific(pkey_, newObj);
			}

			pthread_key_t pkey_;
		};
		//ThreadLocal
		static __thread mongocxx::client* s_value_;
		static Deleter s_deleter_;
		//mongoDB
		static std::string s_mongoDBConntionString_;
		static mongocxx::options::client s_options_;
	};

}//namespace MongoDBClient

#endif