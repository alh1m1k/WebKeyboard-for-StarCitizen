#pragma once

#include "generated.h"

#if defined(SYNC_WRITE_OP_WITH_MUTEX)
#include <mutex>
#endif

#include "esp_err.h"
#include "esp_http_server.h"

#include "invalid_descriptor.h"

#include "result.h"
#include "asyncSocket.h"


namespace http::socket {
	
	#if defined(SYNC_WRITE_OP_WITH_MUTEX) 
	extern std::mutex m;
	#endif
	
	class socket {
		
		httpd_req_t* handler;
		
		public:
			
			typedef std::string message;
		
			explicit socket(httpd_req_t* req);
			
			size_t available();
			
			resBool read(const uint8_t* buffer, size_t size);
								
			result<message> read();
			
			resBool write(const uint8_t* buffer, size_t size, 	httpd_ws_type_t type = httpd_ws_type_t::HTTPD_WS_TYPE_BINARY);
			
			resBool write(const message& msg, 					httpd_ws_type_t type = httpd_ws_type_t::HTTPD_WS_TYPE_TEXT	);
			
			resBool write(const char* msg, 						httpd_ws_type_t type = httpd_ws_type_t::HTTPD_WS_TYPE_TEXT	);
			
						
			inline httpd_handle_t serverHandler() {
				return handler->handle;
			}
			
			inline int fileDescriptor() {
				return httpd_req_to_sockfd(handler);
			}		
			
			inline httpd_req_t* native() {
				return handler;
			}
			
			asyncSocket keep() {
				if (int fd = fileDescriptor(); fd == -1) {
					throw invalid_descriptor("keeping socket");
				} else {
					return asyncSocket(handler->handle, fd);
				}
			}	
	};
}