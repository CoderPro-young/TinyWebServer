#include "user.h"
#include <string.h>
#include <stdio.h> 
#include <fcntl.h> 
#include <assert.h> 
#include <sys/socket.h>

typedef struct sockaddr SA; 

static void unix_error(const char* err){
	perror(err); 
}

static int setnonblocking(int fd){
	int old_option = fcntl(fd, F_GETFL); 
	int new_option = old_option | O_NONBLOCK; 
	fcntl(fd, F_SETFL, new_option); 
	return old_option; 
}

static void modfd(int epollfd, int fd, int ev){
	epoll_event event; 
	event.data.fd = fd; 

	event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP; 
	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event); 
}

static void addfd(int epollfd, int fd){
	struct epoll_event ev; 
	ev.data.fd = fd; 
	ev.events = EPOLLIN | EPOLLET; 
	int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev); 
	assert( ret != -1); 
	setnonblocking(fd); 
}


int user::addUser(int epollfd, int listenfd)
{
	//curr_conn_sum++; 
	struct sockaddr_in client_address; 
	socklen_t client_addrlen = sizeof(client_address); 
	int connfd = accept(listenfd, (SA*)&client_address, &client_addrlen); 
	printf("connfd is %d \n", connfd); 
	if(connfd < 0){
		unix_error("accept"); 
	}

	while(connfd > 0){
		int index = getIndex(); // select a http_conn object to deal request  
		if(index < 0){
			serverBusy(); 
		}
		curr_conn_sum++; 
		fd_to_index[connfd] = index; 
		http_ptr[index].init(epollfd, connfd, client_address); 
		addfd(epollfd, connfd); 
		
		bzero(&client_address, sizeof(client_address)); 
		connfd = accept(listenfd, (SA*)&client_address, &client_addrlen); 
	}
	modfd(epollfd, listenfd, EPOLLIN); 
	return connfd; 
}

int user::getIndex()
{
	return 0; 
}

void user::serverBusy()
{

}

void user::handler(int connfd, const epoll_event& event)
{
	int index = fd_to_index[connfd]; 
	printf("handler\n"); 
	if(event.events & EPOLLIN){
		
		http_ptr[index].read(); 
		http_ptr[index].process(); 
		//http_ptr[index].write(); 
	}
	else if(event.events & EPOLLOUT){
		printf("write to socket"); 
		http_ptr[index].write(); 
	}
}
