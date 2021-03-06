#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "HttpConn.h"
#include <netinet/in.h>
//#include "web_function.h"
#include <arpa/inet.h>

static int setnonblocking(int fd){

}


void HttpConn::init(int connfd, const struct sockaddr_in& client_address)
{
	m_sockfd = connfd; 
	m_client_address = client_address; 
	int m_client_addrlen = sizeof(client_address); 
	setnonblocking(m_sockfd); 

	init(); 

}

void HttpConn::init()
{
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;

    m_method = Get;
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    memset( m_read_buf, '\0', READ_BUFFER_SIZE );
    memset( m_write_buf, '\0', WRITE_BUFFER_SIZE );
    memset( m_real_file, '\0', FILENAME_LEN );
}

bool HttpConn::read()
{
	if(m_read_idx > READ_BUFFER_SIZE){
		return false; 
	}

	int bytes_read = 0; 
	while(true){
		bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0); 		
		if(bytes_read == -1){
			if(errno == EAGAIN || errno == EWOULDBLOCK){
				break; 
			}
		}
		else if(bytes_read == 0){
			break; 
		}

		m_read_idx += bytes_read; 
		
	}

	return true; 
}

/*
HttpConn::HTTP_CODE HttpConn::process_read()
{
	LINE_STATUS line_status = LINE_OK;
	HTTP_CODE ret = NO_REQUEST; 
	char* text = 0; 

	while((m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || (line_status = parse_line()) == LINE_OK){
		text = get_line(); 
		m_start_line = m_check_idx; 
		switch(m_check_state)
		{
			case 
		}
	}
}
*/ 
