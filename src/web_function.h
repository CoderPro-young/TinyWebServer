#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <assert.h>
#include <stdio.h>

extern int port; 

int listenfd(){
	int listenSocket = socket(AF_INET, SOCK_STREAM, 0); 
	struct sockaddr_in server_address; 
	server_address.sin_family = AF_INET; 
	server_address.sin_port = htons(port); 
	inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);
        
	return listenSocket; 	
}

void unix_error(const char* err){
	fprintf(stderr, "%s", err);	
}

void addfd(int epollfd, int fd){
	struct epoll_event ev; 
	ev.data.fd = fd; 
	ev.events = EPOLLIN | EPOLLET; 
	int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev); 
	assert( ret != -1); 
}

int setnonblocking(int fd){
	int old_option = fcntl(fd, F_GETFL); 
	int new_option = old_option | O_NONBLOCK; 
	fcntl(fd, F_SETFL, new_option); 
	return old_option; 
}
