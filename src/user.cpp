#include "user.h"

int user::addUser(int epollfd, int listenfd)
{
	//curr_conn_sum++; 
	struct sockaddr_in client_address; 
	socklen_t client_addrlen = sizeof(client_address); 
	int connfd = accept(listenfd, (SA*)&client_address, client_addrlen); 

	while(connfd > 0){
		int index = getIndex(); 
		if(index < 0){
			serverBusy(); 
		}
		curr_conn_sum++; 
		fd_to_index[connfd] = index; 
		http_ptr[index].init(connfd, &client_address, ); 
		
		bzero(client_address, sizeof(client_address)); 
		connfd = accept(listenfd, (SA*)&client_address, client_addrlen); 
	}
	return connfd; 
}
