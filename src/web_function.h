#include <unistd.h>
#include <fctl.h>
#include <sys/socket.h>
#include <sys/epoll.h>

int listenfd(){

}

void addfd(int epollfd, int fd){
	struct epoll_event ev; 
	ev.data.fd = fd; 
	ev.events = EPOLLIN | EPOLLET; 
	int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev); 
	assert( ret != -1); 
}


