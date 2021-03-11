#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "EventLoopThreadPool.h"
#include "web_function.h"
#include <signal.h>

#define DEFPORT 2021 
#define EVENTLOOPNUM 16
#define MAX_EVENT_NUM 16
#define INFOSIZE 1024 
#define ALARMTIME 5

int port = DEFPORT; 

int sig_pipefd[2];

void sig_handler(int sig){
	int save_errno; 
	int msg = sig; 
	send(sig_pipefd[1], (char*)&msg, 1, 0); 
	save_errno = errno; 
}

void deal_rcv_sig(EventLoopThreadPool& eventLoopThreadPool_){
	char infos[INFOSIZE]; 
	int ret = recv(sig_pipefd[0], infos, INFOSIZE, 0); 
	assert(ret != -1); 
	for(int i = 0; i < ret; i++){
		switch(infos[i])
		{
			case SIGINT:
				break; 
			case SIGALRM:
				eventLoopThreadPool_.dealTimeOut(); 
				alarm(ALARMTIME); 
				break; 
		}
	}
}

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
 
	int ret = socketpair( PF_UNIX, SOCK_STREAM, 0, sig_pipefd ); 
	assert(ret != -1); 

	setnonblocking(sig_pipefd[1]); 
	addfd(epollfd, sig_pipefd[0]); 

	//signal action
	//addsig(SIGTERM, sig_handler); 
	//addsig(SIGINT, sig_handler); 
	addsig(SIGALRM, sig_handler); 

	bool stop = false; 
	alarm(ALARMTIME); 

	while(!stop){
		int num; 
		if((num = epoll_wait(epollfd, events, MAX_EVENT_NUM, -1)) < 0){
			if(errno == EINTR){
				continue; 
			}
			// others 	
			break; 
		}
		printf("%d events in main thread\n", num); 
		for(int i = 0; i < num; i++){
			printf("should accept connect \n"); 
			int fd = events[i].data.fd; 
			if(fd == listenfd){
				eventLoopThreadPool_.addNewConn(fd); 
				
			}
			else if(fd == sig_pipefd[0] && (events[i].events & EPOLLIN)){
				printf("time out\n"); 
				deal_rcv_sig(eventLoopThreadPool_); 
			}

		}	
	}	

}
