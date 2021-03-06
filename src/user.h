#ifndef USER_H_
#define USER_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <sys/epoll.h>
#include "HttpConn.h"

#define MAXCONNPERTHREAD 1024 

class user{
	private:
		HttpConn* http_ptr; 
		int max_conn_num; 
		int curr_conn_sum; 
		std::unordered_map<int, int> fd_to_index; 
	public:
		user(): max_conn_num(MAXCONNPERTHREAD), curr_conn_sum(0){
			http_ptr = new HttpConn[MAXCONNPERTHREAD]; 
		}

		~user(){
			delete [] http_ptr; 
		}

		int addUser(int epollfd, int connfd); 
		int getIndex(); 
		void serverBusy(); 
		void handler(int connfd, const epoll_event& event); 
}; 

#endif
