#ifndef HTTPCONN_H_
#define HTTPCONN_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <queue>
//#include "user.h"

const int READ_BUFFER_SIZE = 4096; 
const int WRITE_BUFFER_SIZE = 4096; 
const int FILENAME_LEN = 128;

class util_timer; 

class HttpConn{
	public:
		enum METHOD
		{
			GET = 0,
			Post
		};

	    enum CHECK_STATE
	    {
			CHECK_STATE_REQUESTLINE = 0,
			CHECK_STATE_HEADER,
			CHECK_STATE_CONTENT
	    };

	    enum HTTP_CODE
	    {
			NO_REQUEST,
			GET_REQUEST,
			BAD_REQUEST,
			NO_RESOURCE,
			FORBIDDEN_REQUEST,
			FILE_REQUEST,
			INTERNAL_ERROR,
			CLOSED_CONNECTION
	    };

	    enum LINE_STATUS
	    {
			LINE_OK = 0,
			LINE_BAD,
			LINE_OPEN
	    };

	public:
		HttpConn(){}
		~HttpConn(){}
	public:
		void init(int, int , const struct sockaddr_in&, std::queue<int>* free_queue, int, util_timer* util);
	    void close_conn( bool real_close = true );//关闭连接
		bool read();  // read data from socket to user buffer
		void process();  // parse data 
		bool write();   // write data to socket

	private:
	    int m_sockfd;
		int m_epollfd; 
		int m_index; 
		std::queue<int>* m_free_queue; 
		util_timer* m_util; 
	    struct sockaddr_in m_client_address;
	    int m_client_addrlen; 
	    char m_read_buf[READ_BUFFER_SIZE];
	    int m_read_idx;
	    int m_checked_idx;
	    int m_start_line;
	    char m_write_buf[WRITE_BUFFER_SIZE];
	    int m_write_idx;
	    CHECK_STATE m_check_state;
	    METHOD m_method;
	    char m_real_file[FILENAME_LEN];
		char doc_root[FILENAME_LEN]; 
	    char *m_url;
	    char *m_version;
	    char *m_host;
	    int m_content_length;
	    bool m_linger;
	    char *m_file_address;
	    struct stat m_file_stat;
	    struct iovec m_iv[2];
	    int m_iv_count;
	    int cgi;        //是否启用的POST
	    char *m_string; //存储请求头数据
	    int bytes_to_send;
	    int bytes_have_send;
	    //char *doc_root;
	private:
		void init(); 
		HTTP_CODE process_read();
	    	bool process_write(HTTP_CODE ret);
	    	HTTP_CODE parse_request_line(char *text);
	    	HTTP_CODE parse_headers(char *text);
	    	HTTP_CODE parse_content(char *text);
	    	HTTP_CODE do_request();
	    	char *get_line() { return m_read_buf + m_start_line; };
	    	LINE_STATUS parse_line();
	    	void unmap();
	    	bool add_response(const char *format, ...);
	    	bool add_content(const char *content);
	    	bool add_status_line(int status, const char *title);
	    	bool add_headers(int content_length);
	    	bool add_content_type();
	    	bool add_content_length(int content_length);
	    	bool add_linger();
	    	bool add_blank_line();
}; 

#endif 
