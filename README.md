# libwebsocket-component websocket协议栈实现(遵循RFC6455规范)：
		* 采用组件化开发，接口实现简单好用，隐藏底层实现细节
		* 模块化设计实现，高内聚低耦合，协议独立，不依赖第三方网络库
		* 可以方便接入第三方网络库
		* 支持消息分片，支持分片消息插入PING/PONG心跳探测帧
		* 支持大数量收发时的chunk分片
		* 支持WSS(SSL认证的加密websocket)
		* 打印日志信息详细，方便学习调试



## 因为公司用的是陈硕的muduo网络库，所以本套协议实现也是为了无缝对接muduo库，步骤如下：
		
		* 先编译 libwebsocket 库
		* 网上下载陈硕的muduo网络库
		* 用thirdpart/muduo修改过的文件替换新下载的muduo库
		* 重新编译muduo库，记得链接libwebsocket库
		* 创建测试demo工程demo/下，然后编译(记得链接libwebsocket库)，运行
		* 打开测试网站 http://www.websocket.org/echo.html
		* 将测试地址换成 wss://192.168.2.93:10000 或SSL认证支持的 wss://192.168.2.93:10000 

