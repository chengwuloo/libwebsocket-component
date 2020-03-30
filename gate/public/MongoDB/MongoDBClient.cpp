#include "MongoDBClient.h"

namespace MongoDBClient {

	__thread mongocxx::client* ThreadLocalSingleton::s_value_ = 0;
	typename ThreadLocalSingleton::Deleter ThreadLocalSingleton::s_deleter_;
	std::string ThreadLocalSingleton::s_mongoDBConntionString_ = "";
	mongocxx::options::client ThreadLocalSingleton::s_options_;

}//namespace MongoDBClient
