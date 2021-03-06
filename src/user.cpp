#include "user.h"
#include <string.h>
#include <sys/socket.h>

typedef struct sockaddr SA; 

int user::addUser(int epollfd, int listenfd)
{
	//curr_conn_sum++; 
	struct sockaddr_in client_address; 
	socklen_t client_addrlen = sizeof(client_address); 
	int connfd = accept(listenfd, (SA*)&client_address, &client_addrlen); 

	while(connfd > 0){
		int index = getIndex(); // select a http_conn object to deal request  
		if(index < 0){
			serverBusy(); 
		}
		curr_conn_sum++; 
		fd_to_index[connfd] = index; 
		http_ptr[index].init(connfd, client_address); 
		
		bzero(&client_address, sizeof(client_address)); 
		connfd = accept(listenfd, (SA*)&client_address, &client_addrlen); 
	}
	return connfd; 
}

int user::getIndex()
{

}

void user::serverBusy()
{

}

void user::handler(int connfd, const epoll_event& event)
{

}
