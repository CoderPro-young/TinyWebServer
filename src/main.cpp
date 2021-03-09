#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "EventLoopThreadPool.h"
#include "web_function.h"

#define DEFPORT 2021 
#define EVENTLOOPNUM 16
#define MAX_EVENT_NUM 16

int port = DEFPORT; 

int main(int argc, char* argv[])
{

	if(argc == 2){
		port = atoi(argv[1]);    // may overflow
	}

	EventLoopThreadPool eventLoopThreadPool_(EVENTLOOPNUM); 
	eventLoopThreadPool_.startPool(); 

	int listenfd = init_listen(); 
	printf("listenfd is %d \n", listenfd); 
	int epollfd = epoll_create(5); 
	assert(epollfd != -1); 

	struct epoll_event events[MAX_EVENT_NUM]; 

	setnonblocking(listenfd); 
	addfd(epollfd, listenfd); 

	int sig_pipefd[2]; 
	int ret = socketpair( PF_UNIX, SOCK_STREAM, 0, sig_pipefd ); 
	assert(ret != -1); 

	bool stop = false; 

	while(!stop){
		int num; 
		if((num = epoll_wait(epollfd, events, MAX_EVENT_NUM, -1)) < 0){
			if(errno == EINTR){
				continue; 
			}
			// others 	
			break; 
		}
		printf("should accept connect \n"); 
		for(int i = 0; i < num; i++){
			int fd = events[i].data.fd; 
			if(fd == listenfd){
				eventLoopThreadPool_.addNewConn(fd); 
				
			}
			else if(fd == sig_pipefd[0] && (events[i].events & EPOLLIN)){
				
			}

		}	
	}	

}
