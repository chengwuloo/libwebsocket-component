/************************************************************************/
/*    @author create by andy_ro@qq.com/Qwuloo@qq.com                    */
/*    @Date		   03.03.2020                                           */
/************************************************************************/

#ifndef _MUDUO_ITIMESTAMP_H_
#define _MUDUO_ITIMESTAMP_H_

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
#include <websocket/base.h>

namespace muduo {

	//@@ ITimestamp
	class ITimestamp : public std::enable_shared_from_this<ITimestamp> {
	public:
		virtual void swapPtr(ITimestamp* that) = 0;
		virtual std::string toString() const = 0;
		virtual std::string toFormattedString(bool showMicroseconds = true) const = 0;
		virtual bool valid() const = 0;
		virtual int64_t microSecondsSinceEpoch() const = 0;
		virtual time_t secondsSinceEpoch() const = 0;
	};
	//@@
	typedef std::shared_ptr<ITimestamp> ITimestampPtr;
	typedef std::weak_ptr<ITimestamp> WeakITimestampPtr;

}//namespace muduo

#endif