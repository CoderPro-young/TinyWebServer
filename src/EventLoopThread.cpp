#include "EventLoopThread.h" 

template<typename T>
int EventLoopThread<T>::start()
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

template<typename T>
void* EventLoopThread<T>::start_thread(void* arg)
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

template<typename T>
void EventLoopThread<T>::loop()
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
				int connfd = recv(); 
				user_.addUser(connfd); 
			}
			else if(){
				user_.handler(connfd, events[i].events); 
			}

		}	
	}

}
