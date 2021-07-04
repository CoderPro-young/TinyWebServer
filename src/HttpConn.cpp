#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "HttpConn.h"
#include "user.h"
#include <netinet/in.h>
//#include "web_function.h"
#include <arpa/inet.h>
#include <sys/epoll.h> 
#include <fcntl.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <queue>

const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\n";

static int setnonblocking(int fd){
	int old_option = fcntl(fd, F_GETFL); 
	int new_option = old_option | O_NONBLOCK; 
	fcntl(fd, F_SETFL, new_option); 
	return old_option; 
}

static void modfd(int epollfd, int fd, int ev){
	epoll_event event; 
	event.data.fd = fd; 

	event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP; 
	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event); 
}

static void removefd(int epollfd, int fd)
{
	epoll_ctl( epollfd, EPOLL_CTL_DEL, fd, 0 );
	close(fd); 
}

void HttpConn::close_conn(bool real_close)
{
	if(real_close){
		removefd(m_epollfd, m_sockfd); 
		m_free_queue->push(m_index); 
		m_util->isfree = true; 
	}
}

void HttpConn::init(int epollfd,int connfd, const struct sockaddr_in& client_address,std::queue<int>* free_queue,int index,util_timer* util)
{
	m_sockfd = connfd; 
	m_epollfd = epollfd; 
	m_index = index; 
	m_util = util;
	m_free_queue = free_queue; 
	m_client_address = client_address; 
	int m_client_addrlen = sizeof(client_address); 
	setnonblocking(m_sockfd); 

	init(); 

}

void HttpConn::init()
{
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;

    m_method = GET;
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

HttpConn::LINE_STATUS HttpConn::parse_line()
{
	char temp; 
	while(m_checked_idx < m_read_idx){
		temp = m_read_buf[m_checked_idx]; 
		if(temp == '\r'){
			if(m_checked_idx + 1 == m_read_idx){
				return LINE_OPEN; 	
			}
			else if(m_read_buf[m_checked_idx + 1] == '\n'){
				m_read_buf[m_checked_idx++] = '\0'; 
				m_read_buf[m_checked_idx++] = '\0'; 
				return LINE_OK; 
			}
			return LINE_BAD; 
		}		
		else if(temp == '\n'){
			if(m_checked_idx > 1 && m_read_buf[m_checked_idx - 1] == '\r'){
				m_read_buf[m_checked_idx - 1] = '\0'; 
				m_read_buf[m_checked_idx++] = '\0'; 
				return LINE_OK; 
			}
			return LINE_BAD; 
		}
		m_checked_idx++; 
	}
	return LINE_OPEN; 
}

HttpConn::HTTP_CODE HttpConn::process_read()
{
	LINE_STATUS line_status = LINE_OK;
	HTTP_CODE ret = NO_REQUEST; 
	char* text = 0; 

	while((m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || (line_status = parse_line()) == LINE_OK){
		text = get_line(); // haven't update m_start_line 
		m_start_line = m_checked_idx; 
		switch(m_check_state)
		{
			case CHECK_STATE_REQUESTLINE:
				{
					ret = parse_request_line(text); 
					if(ret == BAD_REQUEST){
						return BAD_REQUEST; 
					}
					break; 
				}	
			case CHECK_STATE_HEADER:
				{
					ret = parse_headers(text); 
					if(ret == GET_REQUEST){
						return do_request(); 		
					}
					else if(ret == BAD_REQUEST){
						return BAD_REQUEST; 
					}
					break; 
				}
				/*
			case CHECK_STATE_CONTENT:
				{
					ret = parse_content(text); 
					break; 
				}
				*/
		}
	}
	return NO_REQUEST; 
}

HttpConn::HTTP_CODE HttpConn::parse_request_line(char* text)
{
	m_url = strpbrk(text, " \t"); 
	if(!m_url){
		return BAD_REQUEST; 
	}
	*m_url++ = '\0'; 
	char* method = text; 
	if(strcasecmp(method, "GET") == 0){
		m_method = GET; 
	}
	else if(strcasecmp(method, "POST") == 0){
		m_method = POST; 
	}
	else{
		return BAD_REQUEST;
	}
	m_url += strspn(m_url, " \t"); // remove empty " " 
	m_version = strpbrk(m_url, " \t"); 
	if(!m_version)
		return BAD_REQUEST; 
	*m_version++ = '\0';  // add '\0' for m_url 
	m_version += strspn(m_version, " \t"); 
	if(strcasecmp(m_version, "HTTP/1.1") != 0){
		return BAD_REQUEST; 
	}
	if(strncasecmp(m_url, "http://", 7) == 0){
		m_url += 7; 
		m_url = strchr(m_url, '/'); 
	}
	if(strncasecmp(m_url, "https://", 8) == 0){
		m_url += 8; 
		m_url = strchr(m_url, '/'); 
	}
	if(!m_url || m_url[0] != '/'){
		return BAD_REQUEST; 
	}
	m_check_state = CHECK_STATE_HEADER; 
	return NO_REQUEST; 
}

HttpConn::HTTP_CODE HttpConn::parse_headers(char* text)
{
	if(text[0] == '\0'){
		// reach the '\r\n' under the header 
		if(m_content_length != 0){
			return NO_REQUEST; 
		}
		return GET_REQUEST; 
	}
	else if(strncasecmp(text, "Connection:", 11) == 0){
		text += 11; 
		text += strspn(text, " \t"); 
		if(strcasecmp(text, "Keep-alive") == 0){
			m_linger = true; 
		}
	}
	else if(strncasecmp(text, "Host", 5) == 0){
		text += 5; 
		text += strspn(text, " \t"); 
		m_host = text; 
	}
	return NO_REQUEST; 
}

HttpConn::HTTP_CODE HttpConn::do_request()
{
	//need xiugai 
	HttpConn::HTTP_CODE ret = FILE_REQUEST; 
	strcpy(doc_root, "../page"); 
	strcpy(m_real_file, doc_root); 
	if(strchr(m_url, '?') != NULL){
		char download_filename[FILENAME_LEN]; 
		// parse uri to get download_filename
		parse_uri(download_filename); 
		strcat(m_real_file, download_filename); 
		printf("m_real_file is %s \n", m_real_file); 
		ret = DOWNLOAD_REQUEST; 
	}
	else{
		if(strlen(m_url) == 1){
		strcat(m_real_file, "/default.html"); 
		}
		else{
			strcat(m_real_file, m_url); 
		}
		printf("real file is %s \n", m_real_file); 
	}
	
	//struct stat m_url_stat; 
	
	if(stat(m_real_file, &m_file_stat) < 0){
		printf("NO_RESOURCE\n"); 
		return NO_RESOURCE; 
	}

	if(!(m_file_stat.st_mode & S_IROTH)){
		return FORBIDDEN_REQUEST; 
	}

	if(S_ISDIR(m_file_stat.st_mode)){
		return BAD_REQUEST; 
	}

	int fd = open(m_real_file, O_RDONLY); 
	m_file_address = (char*)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0); 
	close(fd); 
	return ret ; 
}

void HttpConn::parse_uri(char download_filename[]){
	//need check file 
	memset(download_filename, '\0', FILENAME_LEN); 
	char* fileid = strchr(m_url, '?'); 
	fileid += 8; 
	// get filename with fileid 
	strcpy(download_filename, "/"); 
	strcat(download_filename, "test.txt"); 
}

void HttpConn::unmap()
{
	if(m_file_address){
		munmap(m_file_address, m_file_stat.st_size); 
		m_file_address = 0; 
	}
}

bool HttpConn::write()
{
    int temp = 0;

    if (bytes_to_send == 0)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        init();
        return true;
    }

    while (1)
    {
        temp = writev(m_sockfd, m_iv, m_iv_count);

        if (temp < 0)
        {
            if (errno == EAGAIN)
            {
                modfd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }

        bytes_have_send += temp;
        bytes_to_send -= temp;
        if (bytes_have_send >= m_iv[0].iov_len)
        {
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_file_address + (bytes_have_send - m_write_idx);
            m_iv[1].iov_len = bytes_to_send;
        }
        else
        {
            m_iv[0].iov_base = m_write_buf + bytes_have_send;
            m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
        }

        if (bytes_to_send <= 0)
        {
            unmap();
            modfd(m_epollfd, m_sockfd, EPOLLIN);

            if (m_linger)
            {
                init(); // keep-alive connection 
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}

bool HttpConn::add_response(const char *format, ...)
{
    if (m_write_idx >= WRITE_BUFFER_SIZE){
		fprintf(stderr, "too data\n"); 
		return false;
	}

    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx))
    {
		fprintf(stderr, "too len\n"); 
        va_end(arg_list);
        return false;
    }
    m_write_idx += len;
    va_end(arg_list);

    return true;
}

bool HttpConn::add_status_line(int status, const char *title)
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool HttpConn::add_headers(int content_len, bool download = false)
{
	if(download){
		return add_content_length(content_len) && add_content_type(download) &&
		add_content_disposition() && add_blank_line(); 
	}
    return add_content_length(content_len) && add_linger() &&
           add_blank_line();
}

bool HttpConn::add_content_length(int content_len)
{
    return add_response("Content-Length:%d\r\n", content_len);
}

bool HttpConn::add_content_type(bool download = false)
{
	if(download){
		return add_response("Content-Type:%s\r\n", "text/plain"); 
	}
    return add_response("Content-Type:%s\r\n", "text/html");
}

bool HttpConn::add_content_disposition()
{
	return add_response("Content-Disposition:%s;%s\r\n", "attachment", "test.txt"); 
}

bool HttpConn::add_linger()
{
    return add_response("Connection:%s\r\n", (m_linger == true) ? "keep-alive" : "close");
}

bool HttpConn::add_blank_line()
{
    return add_response("%s", "\r\n");
}

bool HttpConn::add_content(const char *content)
{
    return add_response("%s", content);
}

bool HttpConn::process_write(HTTP_CODE ret)
{
    switch (ret)
    {
    case INTERNAL_ERROR:
    {
        add_status_line(500, error_500_title);
        add_headers(strlen(error_500_form));
        if (!add_content(error_500_form))
            return false;
        break;
    }
    case BAD_REQUEST:
    {
        add_status_line(404, error_404_title);
        add_headers(strlen(error_404_form));
        if (!add_content(error_404_form))
            return false;
        break;
    }
    case FORBIDDEN_REQUEST:
    {
        add_status_line(403, error_403_title);
        add_headers(strlen(error_403_form));
        if (!add_content(error_403_form))
            return false;
        break;
    }
    case FILE_REQUEST:
    {
		printf("file_request in \n"); 
        add_status_line(200, ok_200_title);
        if (m_file_stat.st_size != 0)
        {
			printf("file len is not 0\n"); 
            add_headers(m_file_stat.st_size);
            m_iv[0].iov_base = m_write_buf;
            m_iv[0].iov_len = m_write_idx;
            m_iv[1].iov_base = m_file_address;
            m_iv[1].iov_len = m_file_stat.st_size;
            m_iv_count = 2;
            bytes_to_send = m_write_idx + m_file_stat.st_size;
            return true;
        }
        else
        {
			printf("file len is 0\n"); 
            const char *ok_string = "<html><body></body></html>";
            add_headers(strlen(ok_string));
            // if (!add_content(ok_string)){
			// 	fprintf(stderr, "add content\n"); 
			// 	return false;
			// }        
        }
		break; 
    }
	case DOWNLOAD_REQUEST:
	{
		printf("download file \n"); 
		add_status_line(200, ok_200_title); 
		if(m_file_stat.st_size != 0){
			add_headers(m_file_stat.st_size, true); 
		}
		// write(); 
		bytes_to_send = m_write_idx; 
		return true; 
	}
    default:
        return false;
    }
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    bytes_to_send = m_write_idx;
    return true;
}

ssize_t HttpConn::Send(int fd, void* userbuf, size_t n)
{
	size_t nleft = n; 
	size_t nsend; 
	char* bufp = (char*)userbuf; 

	while(nleft > 0){
		if((nsend = send(fd, bufp, n, 0)) <= 0){
			if(errno == EINTR){
				nsend = 0; 
			}
			else if(errno == EAGAIN){
				break; 
			}
			else{
				return -1; 
			}
		}

		nleft -= nsend; 
		bufp += nsend; 
	}
	return n; 
}

bool HttpConn::download()
{
	int len = Send(m_sockfd, m_write_buf, bytes_to_send); 
	if(len < 0){
		return false; 
	}
	Send(m_sockfd, m_file_address, m_file_stat.st_size); 
}

void HttpConn::process()
{
	HTTP_CODE read_ret = process_read(); 
	if(read_ret == BAD_REQUEST){
		modfd(m_epollfd, m_sockfd, EPOLLIN); 
	}

	// Debug 
	// if(read_ret == FILE_REQUEST){
	// 	printf("file request.\n"); 
	// }
	bool write_ret = process_write(read_ret); 
	if(read_ret == DOWNLOAD_REQUEST){
		printf("download request"); 
		// transmit file 
		download(); 
		return ; // 
	}

	if(!write_ret){
		printf("close conn\n"); 
		close_conn(); 
	}
	else{
		modfd(m_epollfd, m_sockfd, EPOLLOUT);
	}
}
