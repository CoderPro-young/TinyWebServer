#ifndef EVENTLOOPTHRAED_H_
#define EVENTLOOPTHRAED_H_

#include <pthread.h>
#include <sys/socket.h>
#include <web_function.h>

template<typename T>
class EventLoopThread{
	private:
		pthread_t tid; 
		void loop(); 
		bool stop; 
		int epollfd; 
		int conn_sum; 
		int pipefd[2]; // used to received new connection 
	public:
		EventLoopThread(): conn_sum(0), stop(false){
			epollfd = epoll_create(EVENT_TABLE_SIZE);  
			assert( epollfd != -1); 

			int ret = socketpair( PF_UNIX, SOCK_STREAM, 0, pipefd); 
			assert(ret != -1); 

		}

		~EventLoopThread(){
			
		}
		
		void* start_thread(void* arg); 
		int start(); 
		void run(); 
}

#endif
