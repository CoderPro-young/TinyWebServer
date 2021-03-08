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
	int old_option = fcntl(fd, F_GETFL); 
	int new_option = old_option | O_NONBLOCK; 
	fcntl(fd, F_SETFL, new_option); 
	return old_option; 
}

static void modfd(int epollfd, int fd, int ev){
	epoll_event event; 
	event.data.fd = fd; 

	event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP; 
	epoll_ctl(epollfd, EPOLL_CTL_MOD, &event); 
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

HttpConn::LINE_STATUS HTTPCONN::parse_line()
{
	char temp; 
	while(m_ckecked_idx < m_read_idx){
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
		m_start_line = m_check_idx; 
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
	else{
		return BAD_REQUEST;
	}
	m_url += strspn(m_url, " \t"); 
	m_version = strpbrk(m_url, " \t"); 
	if(!m_version)
		return BAD_REQUEST; 
	*m_version++ = '\0'; 
	m_version += strspn(m_version, " \t"); 
	if(strcasecmp(m_version, "HTTP/1.1") != 0){
		return BAD_REQUEST; 
	}
	if(strcasecmp(m_url, "http://", 7) == 0){
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

HttpConn::HTTPCODE HttpConn::parse_headers(char* text)
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
		if(strcasemp(text, "Keep-alive") == 0){
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

HttpConn::HTTPCODE HttpConn::do_request()
{
	//need xiugai 
	if(strlen(m_url) == 1){
		strcat(m_url, "default.html"); 
	}
	struct stat m_url_stat; 
	
	if(stat(m_url, &m_url_stat) < 0){
		return NO_RESOURCE; 
	}

	if(!(m_url_stat.st_mode & S_IROTH)){
		return FORBIDDEN_REQUEST; 
	}

	if(S_ISDIR(m_url_stat.stat)){
		return BAD_REQUEST; 
	}

	int fd = open(m_url, O_RDONLY); 
	m_file_address = (char*)mmap(0, m_file_stat.st_size, PROT_READ, fd, 0); 
	close(fd); 
	return FILE_REQUEST; 
}

bool http_conn::write()
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
                init();
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}

bool http_conn::add_response(const char *format, ...)
{
    if (m_write_idx >= WRITE_BUFFER_SIZE)
        return false;
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx))
    {
        va_end(arg_list);
        return false;
    }
    m_write_idx += len;
    va_end(arg_list);

    LOG_INFO("request:%s", m_write_buf);

    return true;
}

bool http_conn::add_status_line(int status, const char *title)
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool http_conn::add_headers(int content_len)
{
    return add_content_length(content_len) && add_linger() &&
           add_blank_line();
}

bool http_conn::add_content_length(int content_len)
{
    return add_response("Content-Length:%d\r\n", content_len);
}

bool http_conn::add_content_type()
{
    return add_response("Content-Type:%s\r\n", "text/html");
}

bool http_conn::add_linger()
{
    return add_response("Connection:%s\r\n", (m_linger == true) ? "keep-alive" : "close");
}

bool http_conn::add_blank_line()
{
    return add_response("%s", "\r\n");
}

bool http_conn::add_content(const char *content)
{
    return add_response("%s", content);
}

bool http_conn::process_write(HTTP_CODE ret)
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
        add_status_line(200, ok_200_title);
        if (m_file_stat.st_size != 0)
        {
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
            const char *ok_string = "<html><body></body></html>";
            add_headers(strlen(ok_string));
            if (!add_content(ok_string))
                return false;
        }
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

void HttpConn::process()
{
	HTTP_CODE read_ret = process_read(); 
	if(read_ret == BAD_REQUEST){
		modfd(m_epolfd, m_sockfd, EPOLLIN); 
	}

	bool write_ret = process_write(read_ret); 
	if(!write_ret){
		close_conn(); 
	}
	else{
		modfd(m_epollfd, m_sockfd, EPOLLOUT);
	}
}
