/************************************************************************/
/*    @author create by andy_ro@qq.com                                  */
/*    @Date		   03.03.2020                                           */
/************************************************************************/
#ifndef _MUDUO_BASE_OBJECT_H_
#define _MUDUO_BASE_OBJECT_H_

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

namespace muduo {
	namespace base {
		
		//@@ copyable
		class copyable {
		protected:
			copyable() = default;
			~copyable() = default;
		};

		//@@ noncopyable
		class noncopyable {
		public:
			noncopyable(const noncopyable&) = delete;
			void operator=(const noncopyable&) = delete;
		protected:
			noncopyable() = default;
			~noncopyable() = default;
		};

	}//namespace base
}//namespace muduo

#endif