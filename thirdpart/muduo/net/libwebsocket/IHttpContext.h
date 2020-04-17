/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.03.2020                                           */
/************************************************************************/
#ifndef _MUDUO_NET_IHTTPCONTEXT_H_
#define _MUDUO_NET_IHTTPCONTEXT_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>  // memset
#include <string>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>
#include <map>
#include <memory>

#include <muduo/net/libwebsocket/base.h>
#include <muduo/net/libwebsocket/ITimestamp.h>
#include <muduo/net/libwebsocket/IBytesBuffer.h>

namespace muduo {
	namespace net {
		namespace http {

			//@@ IRequest
			class IRequest {
			public:
				//Method
				enum Method {
					kInvalid,
					kGet,
					kPost,
					kHead,
					kPut,
					kDelete
				};
				//Version
				enum Version {
					kUnknown,
					kHttp10,
					kHttp11
				};
			public:
				//@overide
				virtual void setVersion(Version v) = 0;
				//@overide
				virtual Version& getVersionRef() = 0;
				virtual Version getVersion() const = 0;
				//@overide
				virtual bool setMethod(const char* start, const char* end) = 0;
				//@overide
				virtual Method& methodRef() = 0;
				virtual Method method() const = 0;
				//@overide
				virtual const char* methodString() const = 0;
				virtual void setPath(const char* start, const char* end) = 0;
				//@overide
				virtual std::string& pathRef() = 0;
				virtual const std::string& path() const = 0;
				//@overide
				virtual void setQuery(const char* start, const char* end) = 0;
				//@overide
				virtual std::string& queryRef() = 0;
				virtual const std::string& query() const = 0;
				//@overide
				virtual void setReceiveTimePtr(ITimestamp* t) = 0;
				//@overide
				virtual ITimestamp* receiveTimePtr() = 0;
				virtual ITimestamp const* receiveTimeConstPtr() const = 0;
				//@overide
				virtual void addHeader(const char* start, const char* colon, const char* end) = 0;
				virtual std::string getHeader(const std::string& field) const = 0;
				//@overide
				virtual std::map<std::string, std::string>* headersPtr() = 0;
				virtual const std::map<std::string, std::string>& headers() const = 0;
				//@overide
				virtual void req_swap(IRequest* that) = 0;
			};
			
			//@@ IResponse
			class IResponse {
			public:
				//StatusCode
				enum StatusCode {
					kUnknown,
					k200Ok = 200,
					k301MovedPermanently = 301,
					k400BadRequest = 400,
					k404NotFound = 404,
				};
			public:	
				virtual void setStatusCode(StatusCode code) = 0;
				virtual void setStatusMessage(const std::string& message) = 0;
				virtual void setCloseConnection(bool on) = 0;
				virtual bool closeConnection() const = 0;
				virtual void setContentType(const std::string& contentType) = 0;
				virtual void addHeader(const std::string& key, const std::string& value) = 0;
				virtual void setBody(const std::string& body) = 0;
				virtual void appendToBufferPtr(IBytesBuffer* output) const = 0;
			};

			//@@ IContext_
			class IContext_ {
			public:
				//ParseState
				enum ParseState {
					kExpectRequestLine,
					kExpectHeaders,
					kExpectBody,
					kGotAll,
				};
			public:
				virtual bool parseRequestPtr(IBytesBuffer* buf, ITimestamp* receiveTime) = 0;
				virtual bool gotAll() const = 0;
				virtual void reset() = 0;
				virtual IRequest const* requestConstPtr() const = 0;
				virtual IRequest* requestPtr() = 0;
				virtual ~IContext_() {}
			};
			
		}//namespace http

		//@@
		typedef http::IContext_ IHttpContext;
		typedef std::unique_ptr<IHttpContext> IHttpContextPtr;

	}//namespace net
}//namespace muduo

#endif