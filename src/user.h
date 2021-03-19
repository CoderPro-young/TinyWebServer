#ifndef USER_H_
#define USER_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <sys/epoll.h>
#include <queue> 
#include <vector> 
#include "HttpConn.h"

#define MAXCONNPERTHREAD 1024 

extern const int ALARMTIME; 

//class HttpConn; 

class util_timer{
	public:
		bool isfree; 
		time_t expire; 
		int fd; 
		util_timer(time_t time_, int fd_): expire(time_+ALARMTIME), isfree(false), fd(fd_){
			
		}

};

struct timer_cmp{
	bool operator () (util_timer* a, util_timer* b){
		return a->expire > b->expire; 
	}
}; 

class user{
	private:
		HttpConn* http_ptr; 
		util_timer* util_ptr[MAXCONNPERTHREAD]; 
		int max_conn_num; 
		int curr_conn_sum; 
		std::unordered_map<int, int> fd_to_index; 
		std::queue<int> free_queue; 
		std::priority_queue<util_timer*,std::vector<util_timer*>,timer_cmp> timer_queue; 
		//void handlerTimeOut(); 
	public:
		user(): max_conn_num(MAXCONNPERTHREAD), curr_conn_sum(0){
			http_ptr = new HttpConn[MAXCONNPERTHREAD]; 
			for(int i = 0; i < MAXCONNPERTHREAD;i++){
				free_queue.push(i); 
			}
		}

		~user(){

			delete [] http_ptr; 
		}

		int addUser(int epollfd, int connfd); 
		int getIndex(); 
		void serverBusy(int); 
		void handler(int connfd, const epoll_event& event); 
		void handlerTimeOut(); 
		void handlerInt(); 
}; 

#endif
