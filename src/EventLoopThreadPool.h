#ifndef EVENTLOOPTHREADPOOL_H_
#define EVENTLOOPTHREADPOOL_H_

#include "EventLoopThread.h"
#include "HttpConn.h"

class EventLoopThreadPool{
	private:
		int max_thread_num;
	        int curr_ptr_id; 	
		EventLoopThread* eventloop_ptr;
		int getNextLoop(); 
	public:
		EventLoopThreadPool(int num); 
		~EventLoopThreadPool(); 
		void addNewConn(int fd); 
		void startPool(); 	
		void dealTimeOut(); 	
};


#endif
