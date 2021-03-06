#include "EventLoopThread.h" 
#include <sys/epoll.h>
#include <errno.h>
#include <assert.h>
//#include "web_function.h"

#define MAX_EVENT_SIZE 1024 
#define INFOSIZE 4096

static void unix_error(const char* err)
{

}

static void addfd(int epollfd, int fd)
{

}

int EventLoopThread::start()
{
	if(pthread_create(&tid, nullptr, start_thread, this) < 0){
		unix_error("thread create failed"); 
		return -1; 
	}
	
	if(pthread_detach(tid) < 0){
		unix_error("thread detach failed"); 
		return -1; 
	}

	return 0; 
}

void* EventLoopThread::start_thread(void* arg)
{
	EventLoopThread* thread_ = (EventLoopThread*)arg; 
	thread_->loop(); // start up eventloop
}

/*
template<typename T>
void* EventLoopThread<T>::addConn(int fd)
{
	
}
*/



void EventLoopThread::loop()
{
	struct epoll_event events[MAX_EVENT_SIZE]; 
	while(!stop){
		int num = epoll_wait(epollfd, events, MAX_EVENT_SIZE, -1); 
		if(num < 0){
			break; 
		}
		
		for(int i = 0; i < num; i++){
			int fd = events[i].data.fd; 
			if(fd == pipefd[0] && (events[i].events & EPOLLIN)){
				int info[INFOSIZE]; 
				int info_num = recv(pipefd[0], info, INFOSIZE, 0); 
				//assert( info_num != -1); 
				if(info_num == -1){
					if(errno != EAGAIN){
						break; 
					}
				}
				for(int j = 0; j < info_num; j++){
					switch (fun(info[j]))
					{
						case NEWCONN:
							/*
							struct sockaddr_in client_address; 
							socklen_t client_addrlen = sizeof(client_address); 
							int listenfd = info[j]; 
							int connfd = connect(listenfd, (SA*)&client_address, client_addrlen); 
							*/
							{
								int listenfd = info[j]; 
								int ret = user_.addUser(epollfd, listenfd); 
								assert( ret != -1); 
								addfd(epollfd, ret);  
								//server_conn_sum++; // global atmoic variable 
								break; 
							}
						case DEALTIMEOUTCONN:
							{
								break; 
							}
					
					}
				}
				
			}
			else{
				user_.handler(fd, events[i]); 
			}

		}	
	}

}
