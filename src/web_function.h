#ifndef WEB_FUNCTION_H_
#define WEB_FUNCTION_H_

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
extern int port; 
#define LISTENQ 4096 
#define debug

int init_listen(){
	int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	int on = 1; 
        assert(listenSocket != -1); 
	printf("init_listen\n"); 		
	struct sockaddr_in server_address; 
	socklen_t socklen = sizeof(server_address); 

	bzero(&server_address, sizeof(server_address)); 
	server_address.sin_family = AF_INET; 
	server_address.sin_port = htons(port); 
	inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);

	if((setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0){
		perror("sockopt"); 
		exit(1); 
	}
	
	int ret = bind(listenSocket, (struct sockaddr*)&server_address, socklen); 
	assert( ret != -1); 	

	ret = listen(listenSocket, LISTENQ); 
	assert(ret != -1); 

	#ifdef debug
	printf("listen !\n"); 
	#endif
	
	return listenSocket; 	
}

void unix_error(const char* err){
	fprintf(stderr, "%s", err);	
}

int setnonblocking(int fd){
	int old_option = fcntl(fd, F_GETFL); 
	int new_option = old_option | O_NONBLOCK; 
	fcntl(fd, F_SETFL, new_option); 
	return old_option; 
}

void addfd(int epollfd, int fd){
	struct epoll_event ev; 
	ev.data.fd = fd; 
	ev.events = EPOLLIN | EPOLLET; 
	int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev); 
	assert( ret != -1); 
	setnonblocking(fd); 
}

int addsig(int sig, void (handler)(int), bool restart = true){
	struct sigaction sa; 
	sa.sa_handler = handler; 
	if(restart){
		sa.sa_flags |= SA_RESTART; 
	}
	sigfillset(&sa.sa_mask); 
	assert(sigaction(sig, &sa, NULL) != -1); 
}



#endif 
