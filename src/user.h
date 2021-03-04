#ifndef USER_H_
#define USER_H_

#include "HttpConn.h"

class user{
	private:
		HttpConn* http_ptr; 
		int max_conn_num; 
		int curr_conn_sum; 
		unordered_map<int, int> fd_to_index; 
	public:
		user(): max_conn_num(MAXCONN), curr_conn_sum(0){
			http_ptr = new HttpConn[max_conn_sum]; 
		}

		~user():{
			delete [] http_ptr; 
		}

		int addUser(int connfd); 
		int getIndex(); 
}

#endif
