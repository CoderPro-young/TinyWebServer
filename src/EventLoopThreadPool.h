#ifndef EVENTLOOPTHREADPOOL_H_
#define EVENTLOOPTHREADPOOL_H_

class EventLoopThreadPool{
	private:
		int max_thread_num;
	        int curr_ptr_id; 	
		EventLoopThread<HttpConn>* eventloop_ptr;
	public:
		EventLoopThreadPool(int num); 
		~EventLoopThreadPool(); 
		void addNewConn(int fd); 
		void startPool(); 		
}


#endif
