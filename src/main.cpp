#include <stdio.h>
#include <stdlib.h>
#include "EventLoopThreadPool.h"

char port[6] = "2021"; 

int main(int argc, char* argv[])
{
	if(argc == 2){
		strcpy(port, argv[1]);   // may overflow
	}

	EventLoopThreadPool eventLoopThreadPool_; 
	eventLoopThreadPool_.start(); 

	int listenfd = init_listen(); 

	int epollfd = epoll_create(5); 
	assert(epollfd != -1); 

	struct epoll_event events[MAX_EVENT_NUM]; 

	setnonblocking(listenfd); 
	addfd(epollfd, listenfd); 
	while(!stop){
		int num; 
		if((num = epoll_wait(epollfd, events, MAX_EVENT_NUM, -1)) < 0){
			if(errno == EINTR){
				continue; 
			}
			// others 	
			break; 
		}
		
		for(int i = 0; i < num; i++){
			int fd = events[i].data.fd; 
			if(fd == listenfd){
				eventLoopThreadPool_.addNewConn(fd); 
				
			}
			else if(fd == sig_pipefd[0] && (events[i].event & EPOLLIN)){
				
			}

		}	
	}	

}
