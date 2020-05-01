// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_HTTP_HTTPRESPONSE_H
#define MUDUO_NET_HTTP_HTTPRESPONSE_H

#include "muduo/base/copyable.h"
#include "muduo/base/Types.h"

#include "muduo/net/libwebsocket/IHttpContext.h"

#include <map>

namespace muduo
{
namespace net
{

class Buffer;
class HttpResponse : public muduo::copyable, public http::IResponse
{
 public:
//   enum HttpStatusCode
//   {
//     kUnknown,
//     k200Ok = 200,
//     k301MovedPermanently = 301,
//     k400BadRequest = 400,
//     k404NotFound = 404,
//   };
     typedef http::IResponse::StatusCode HttpStatusCode;

  explicit HttpResponse(bool close)
    : statusCode_(http::IResponse::kUnknown),
      closeConnection_(close)
  {
  }

  void setStatusCode(http::IResponse::StatusCode code)
  { statusCode_ = code; }

  void setStatusMessage(const string& message)
  { statusMessage_ = message; }

  void setCloseConnection(bool on)
  { closeConnection_ = on; }

  bool closeConnection() const
  { return closeConnection_; }

  void setContentType(const string& contentType)
  { addHeader("Content-Type", contentType); }

  // FIXME: replace string with StringPiece
  void addHeader(const string& key, const string& value)
  { headers_[key] = value; }

  void setBody(const string& body)
  { body_ = body; }

  void appendToBuffer(Buffer* output) const;
  
  void appendToBufferPtr(IBytesBuffer* output) const {
      assert(output);
      Buffer* buf = reinterpret_cast<Buffer*>(output);
      assert(buf);
      appendToBuffer(buf);
  }
 private:
  std::map<string, string> headers_;
  http::IResponse::StatusCode statusCode_;
  // FIXME: add http version
  string statusMessage_;
  bool closeConnection_;
  string body_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_HTTP_HTTPRESPONSE_H
