### libwebsocket协议栈实现(遵循RFC6455规范)：
		* 采用组件化开发，接口实现简单好用，隐藏底层实现细节
		* 模块化设计实现，强内聚低耦合，协议独立，不依赖第三方网络库
		* 可以方便接入第三方网络库
		* 支持消息分片，支持分片消息插入PING/PONG心跳探测帧
		* 支持大数量收发时的chunk分片，标准协议发送大数据包都是做了分片的
		* 支持WSS(SSL认证的加密websocket)
		* 打印日志信息详细，方便学习调试

		enum MessageT {
			TyTextMessage = 0, //文本消息
			TyBinaryMessage = 1, //二进制消息
		};

		//create
		IContext* create(
			ICallback* handler,
			IHttpContextPtr& context,
			IBytesBufferPtr& dataBuffer,
			IBytesBufferPtr& controlBuffer);

		//free
		void free(IContext* context);

		//parse_message_frame
		int parse_message_frame(
			IContext* context,
			IBytesBuffer* buf,
			ITimestamp* receiveTime);

		//pack_unmask_data_frame S2C
		void pack_unmask_data_frame(
			IBytesBuffer* buf,
			char const* data, size_t len,
			MessageT messageType = MessageT::TyTextMessage, bool chunk = false);

		//pack_unmask_close_frame S2C
		void pack_unmask_close_frame(
			IBytesBuffer* buf,
			char const* data, size_t len);

		//pack_unmask_ping_frame S2C
		void pack_unmask_ping_frame(
			IBytesBuffer* buf,
			char const* data, size_t len);

		//pack_unmask_pong_frame S2C
		void pack_unmask_pong_frame(
			IBytesBuffer* buf,
			char const* data, size_t len);

### 因为公司用的是陈硕的muduo网络库，所以本套协议实现也是为了无缝对接muduo库，步骤如下：
		
		* 先编译 libwebsocket 库
		* 网上下载陈硕的muduo网络库
		* 用thirdpart/muduo修改过的文件替换新下载的muduo库对应文件
		* 重新编译muduo库，需要链接libwebsocket库
		* 创建测试demo工程demo/下，然后编译运行(需要链接libwebsocket库)
		* 打开测试网站 http://www.websocket.org/echo.html 或者 http://www.websocket-test.com/
		* 将测试地址换成 ws://192.168.2.93:10000 或SSL认证支持的 wss://192.168.2.93:10000 

### 修改版本的muduo库在支持https/ws/wss同时加入了支持更高效的EPOLLET模式
   
   //readFull for EPOLLET
   
   static ssize_t readFull(int sockfd, IBytesBuffer* buf, int* savedErrno);

   //writeFull for EPOLLET
   
   static ssize_t writeFull(int sockfd, void const* data, size_t len, int* savedErrno);
   
   TcpServer 中 server_.start(bool et == false)
