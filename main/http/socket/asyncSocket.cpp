#include "asyncSocket.h"
#include "esp_http_server.h"

namespace http::socket {
	
	#if defined(SYNC_WRITE_OP_WITH_MUTEX) 
	std::mutex m {};	
	#endif
		
	asyncSocket::asyncSocket(httpd_handle_t hd, int fd): hd(hd), fd(fd) {};
	
	bool asyncSocket::valid() {
		return (hd != nullptr && fd != 0) && httpd_ws_get_fd_info(hd, fd) == HTTPD_WS_CLIENT_WEBSOCKET;
	}
		
	resBool asyncSocket::write(const uint8_t* buffer, size_t size, httpd_ws_type_t type) {
		#if defined(SYNC_WRITE_OP_WITH_MUTEX) 
		std::unique_lock guardian {m};	
		#endif
		httpd_ws_frame_t 	ws_pkt = {};
		ws_pkt.type 	= type;
		ws_pkt.payload  = (uint8_t*)buffer; //dangerous?
		ws_pkt.len 		= size;
		return httpd_ws_send_frame_async(hd, fd, &ws_pkt);
	}
	
	resBool asyncSocket::write(const message& msg, httpd_ws_type_t type) {
		debugIf(LOG_SOCKET, "socket::write", msg.size());
		return write((const uint8_t*)msg.data(), msg.size(), type);
	}
	
	resBool asyncSocket::write(const char* msg, httpd_ws_type_t type) {
		return write((const uint8_t*)msg, strlen(msg), type);
	}
}