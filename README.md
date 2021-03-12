# TinyWebServer
## 简介
c++实现的轻量级web服务器，采用reactor模式加非阻塞IO实现，主线程负责监听新连接以及信号的处理，工作线程负责建立新连接以及处理客户请求。整体遵循*one loop per thread*的设计，等待在一个event loop上，工作线程由一个线程池进行管理。通过小根堆维护一个定时器，关闭不活跃的连接。 

##  类的设计

 - `EventLoopThreadPool` 管理工作线程，为主线程提供接口实现主线程与工作线程之间的通信
 - `EventLoopThread` 工作线程，开启event loop，等待的事件发生后调用`user`提供的回调函数
 - `user`为工作线程提供接口实现对于事件的处理，封装`HttpConn`
 - `HttpConn`实现对于客户请求的处理，包括读取并解析请求报文，生成相应报文以及向客户发送报文

## 环境
- linux kernel 5.4.0-66
- ubuntu 18.04
- g++ 7.5.0 
- Make 4.1 
 
 ##  使用方法
 下载源码：
 ```
 git clone https://github.com/CoderPro-young/TinyWebServer.git
 ```
 进入目录： `cd src`
 编译生成可执行文件： `make`
 运行：`./server`
 在浏览器中输入`127.0.0.1:2021`即可得到页面

## 后续工作
- 支持GET请求动态内容
- 支持POST方法
- 添加数据库
 

