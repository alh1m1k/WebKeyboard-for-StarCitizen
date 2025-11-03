#pragma once

#include "generated.h"

//#define SYNC_WRITE_OP_WITH_MUTEX

#if defined(SYNC_WRITE_OP_WITH_MUTEX)
#include <mutex>
#endif


#include "esp_err.h"
#include "esp_http_server.h"

#include "result.h"

namespace http::socket {
	
	#if defined(SYNC_WRITE_OP_WITH_MUTEX) 
	extern std::mutex m;
	#endif
	
	class asyncSocket {
		
		httpd_handle_t hd;
		int			   fd;
		
		public:
							
			typedef std::string message;

			asyncSocket(): hd(nullptr), fd(0) {}; //only use as memory allocator
			
			explicit asyncSocket(httpd_handle_t hd, int fd);
			
			bool	valid();
					
			resBool write(const uint8_t* buffer, size_t size, 	httpd_ws_type_t type = httpd_ws_type_t::HTTPD_WS_TYPE_BINARY);
			
			resBool write(const message& msg, 					httpd_ws_type_t type = httpd_ws_type_t::HTTPD_WS_TYPE_TEXT	);
			
			resBool write(const char* msg, 						httpd_ws_type_t type = httpd_ws_type_t::HTTPD_WS_TYPE_TEXT	);
			
			inline bool operator==(const asyncSocket& sock) const {
				return hd == sock.hd && fd == sock.fd;
			}
			
			inline httpd_handle_t serverHandler() {
				return hd;
			}	
			
			inline int fileDescriptor() {
				return fd;
			}	
			
			inline int native() {
				return fd;
			}
	};

	static auto noAsyncSocket = asyncSocket(nullptr, -1);
}