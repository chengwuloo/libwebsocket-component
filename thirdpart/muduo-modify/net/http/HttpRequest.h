// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_HTTP_HTTPREQUEST_H
#define MUDUO_NET_HTTP_HTTPREQUEST_H

#include "muduo/base/copyable.h"
#include "muduo/base/Timestamp.h"
#include "muduo/base/Types.h"

#include "muduo/net/libwebsocket/IHttpContext.h"

#include <map>
#include <assert.h>
#include <stdio.h>

namespace muduo
{
namespace net
{

class HttpRequest : public muduo::copyable, public http::IRequest
{
 public:
//   enum Method
//   {
//     kInvalid, kGet, kPost, kHead, kPut, kDelete
//   };
//   enum Version
//   {
//     kUnknown, kHttp10, kHttp11
//   };
     typedef http::IRequest::Method Method;
     typedef http::IRequest::Version Version;
  HttpRequest()
    : method_(http::IRequest::kInvalid),
      version_(http::IRequest::kUnknown)
  {
  }

  void setVersion(http::IRequest::Version v)
  {
    version_ = v;
  }
//   void setVersion(http::IRequest::Version v) {
//       setVersion((Version)v);
//   }

  http::IRequest::Version& getVersionRef() {
      return version_;
  }

  http::IRequest::Version getVersion() const
  { return version_; }

//   http::IRequest::Version getVersion() const {
//       return (http::IRequest::VersionE)getVersion();
//   }

  bool setMethod(const char* start, const char* end)
  {
    assert(method_ == http::IRequest::kInvalid);
    string m(start, end);
    if (m == "GET")
    {
      method_ = http::IRequest::kGet;
    }
    else if (m == "POST")
    {
      method_ = http::IRequest::kPost;
    }
    else if (m == "HEAD")
    {
      method_ = http::IRequest::kHead;
    }
    else if (m == "PUT")
    {
      method_ = http::IRequest::kPut;
    }
    else if (m == "DELETE")
    {
      method_ = http::IRequest::kDelete;
    }
    else
    {
      method_ = http::IRequest::kInvalid;
    }
    return method_ != http::IRequest::kInvalid;
  }

  http::IRequest::Method& methodRef() {
      return method_;
  }

  http::IRequest::Method method() const
  { return method_; }

//   http::IRequest::MethodE req_method() const {
//       return (http::IRequest::MethodE)method();
//   }
  const char* methodString() const
  {
    const char* result = "UNKNOWN";
    switch(method_)
    {
      case kGet:
        result = "GET";
        break;
      case kPost:
        result = "POST";
        break;
      case kHead:
        result = "HEAD";
        break;
      case kPut:
        result = "PUT";
        break;
      case kDelete:
        result = "DELETE";
        break;
      default:
        break;
    }
    return result;
  }

  void setPath(const char* start, const char* end)
  {
    path_.assign(start, end);
  }

  std::string& pathRef() {
      return path_;
  }
  const string& path() const
  { return path_; }

  void setQuery(const char* start, const char* end)
  {
    query_.assign(start, end);
  }

  std::string& queryRef() {
      return query_;
  }
  const string& query() const
  { return query_; }

  void setReceiveTime(Timestamp t)
  { receiveTime_ = t; }
  
  void setReceiveTimePtr(ITimestamp* t) {
      setReceiveTime(Timestamp(t->microSecondsSinceEpoch()));
  }

  Timestamp receiveTime() const
  { return receiveTime_; }


  ITimestamp* receiveTimePtr() {
      return &receiveTime_;
  }
  ITimestamp const* receiveTimeConstPtr() const {
      return &receiveTime_;
  }

  void addHeader(const char* start, const char* colon, const char* end)
  {
    string field(start, colon);
    ++colon;
    while (colon < end && isspace(*colon))
    {
      ++colon;
    }
    string value(colon, end);
    while (!value.empty() && isspace(value[value.size()-1]))
    {
      value.resize(value.size()-1);
    }
    headers_[field] = value;
  }

  string getHeader(const string& field) const
  {
    string result;
    std::map<string, string>::const_iterator it = headers_.find(field);
    if (it != headers_.end())
    {
      result = it->second;
    }
    return result;
  }

  std::map<string, string>* headersPtr() {
      return &headers_;
  }
  const std::map<string, string>& headers() const
  { return headers_; }

  void swap(HttpRequest& that)
  {
    std::swap(method_, that.method_);
    std::swap(version_, that.version_);
    path_.swap(that.path_);
    query_.swap(that.query_);
    receiveTime_.swap(that.receiveTime_);
    headers_.swap(that.headers_);
  }

  void req_swap(http::IRequest* that) {
	  std::swap(method_, that->methodRef());
	  std::swap(version_, that->getVersionRef());
	  path_.swap(that->pathRef());
	  query_.swap(that->queryRef());
	  receiveTime_.swapPtr(that->receiveTimePtr());
	  headers_.swap(*that->headersPtr());
  }

 private:
  Method method_;
  Version version_;
  string path_;
  string query_;
  Timestamp receiveTime_;
  std::map<string, string> headers_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_HTTP_HTTPREQUEST_H
