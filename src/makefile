server:main.o HttpConn.o User.o EventLoopThread.o EventLoopThreadPool.o
	g++ $^ $@ -pthread
HttpConn.o:HttpConn.cpp HttpConn.h
	g++ -c $< -o $@ -I ./
User.o:user.cpp user.h
	g++ -c $< -o $@ 
EventLoopThread.o:EventLoopThread.cpp EventLoopThread.h
	g++ -c $< -o $@
EventLoopThreadPool.o:EventLoopThreadPool.cpp EventLoopThreadPool.h
	g++ -c $< -o $@
main.o:main.cpp
	g++ -c $< -o $@
