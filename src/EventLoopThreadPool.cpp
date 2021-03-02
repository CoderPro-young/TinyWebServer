#include "EventLoopThreadPool.h"

EventLoopThreadPool::EventLoopThreadPool(int num)
{
	max_thread_num = num; 
	eventloop_ptr = new EventLoopThread<HttpConn>[max_thread_num]; 
}

void EventLoopThreadPool::startPool()
{
	for(int i = 0; i < max_thread_num; i++){
		eventloop_ptr[i].start(); 
	}
}

void EventLoopThreadPool::addNewConn(int fd)
{
	int index = getNextLoop();
        int notify = fd; 	
	send(eventloop_ptr[index]->pipefd[1], &notify, sizeof(notify), 0);  // notify eventloop thread 
}

EventLoopThreadPool::~EventLoopThreadPool()
{
	delete []eventloop_ptr; 
}
