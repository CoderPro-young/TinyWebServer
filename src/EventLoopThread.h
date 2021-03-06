#ifndef EVENTLOOPTHRAED_H_
#define EVENTLOOPTHRAED_H_

#include <pthread.h>
#include <sys/socket.h>
#include "user.h"
#include "web_function.h"

#define NEWCONN 1
#define DEALTIMEOUTCONN -1
#define EVENT_TABLE_SIZE 1024


template<typename T>
class EventLoopThread{
	private:
		pthread_t tid; 
		void loop(); 
		bool stop; 
		int epollfd; 
		int conn_sum; 
		user user_;
	public:
		EventLoopThread(): conn_sum(0), stop(false){
			epollfd = epoll_create(EVENT_TABLE_SIZE);  
			assert( epollfd != -1); 

			int ret = socketpair( PF_UNIX, SOCK_STREAM, 0, pipefd); 
			assert(ret != -1); 

		}

		~EventLoopThread(){
			
		}
		
		int pipefd[2]; // used to received new connection 
		int fun(int info){
			int ret = info;  
			if(info >= 0){
				ret = 1; 
			}
			
			return ret; 
		}
		void* start_thread(void* arg); 
		int start(); 
		void run(); 
}; 

#endif
