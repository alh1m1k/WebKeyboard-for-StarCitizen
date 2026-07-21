#pragma once

#include "generated.h"

#if defined(SYNC_WRITE_OP_WITH_MUTEX)
#include <mutex>
#endif

#include "esp_err.h"
#include "esp_http_server.h"

#include "invalid_descriptor.h"

#include "asyncSocket.h"
#include "result.h"
#include "wscodes.h"

namespace http::socket {
	
	#if defined(SYNC_WRITE_OP_WITH_MUTEX) 
	extern std::mutex m;
	#endif
	
	class socket {
		
		httpd_req_t* req;
		
		public:
			
			typedef std::string message;
		
			explicit socket(httpd_req_t* req);
			
			size_t available() noexcept;
			
			resBool read(const uint8_t* buffer, size_t size) noexcept;
								
			result<message> read() noexcept;
			
			resBool write(const uint8_t* buffer, size_t size, 	httpd_ws_type_t type = httpd_ws_type_t::HTTPD_WS_TYPE_BINARY) noexcept;
			
			resBool write(const message& msg, 					httpd_ws_type_t type = httpd_ws_type_t::HTTPD_WS_TYPE_TEXT	) noexcept;
			
			resBool write(const char* msg, 						httpd_ws_type_t type = httpd_ws_type_t::HTTPD_WS_TYPE_TEXT	) noexcept;

			resBool writeClose(wscodes code = wscodes::NORMAL_CLOSE, const uint8_t* buffer = nullptr, size_t size = 0) noexcept;

			inline resBool writeClose(const wscodes code = wscodes::NORMAL_CLOSE, const char* reason = nullptr) noexcept {
				//at this point we not transfer NIL and end of string
				return writeClose(code, (const uint8_t*)reason, reason == nullptr ? 0 : strlen(reason));
			}

			inline resBool writeClose(const wscodes code = wscodes::NORMAL_CLOSE, const std::string& reason = "") noexcept {
				//at this point we not transfer NIL and end of string
				const auto size = reason.size();
				return writeClose(code, size ? (const uint8_t*)reason.data() : nullptr, size);
			}

			inline bool operator==(const socket& sock) const {
				return req->handle == sock.req->handle && httpd_req_to_sockfd(req) == httpd_req_to_sockfd(sock.req);
			}
						
			inline httpd_handle_t serverHandle() const noexcept {
				return req->handle;
			}
			
			inline int fileDescriptor() const noexcept {
				return httpd_req_to_sockfd(req);
			}		
			
			inline httpd_req_t* native() const noexcept {
				return req;
			}
			
			asyncSocket keep()  {
				if (int fd = fileDescriptor(); fd == -1) {
					throw invalid_descriptor("keeping socket");
				} else {
					return asyncSocket(req->handle, fd);
				}
			}	
	};

	static auto noSocket = socket(nullptr);
}