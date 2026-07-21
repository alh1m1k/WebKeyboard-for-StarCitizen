#pragma once

#include "generated.h"
#include "work.h"

// #define SYNC_WRITE_OP_WITH_MUTEX

#if defined(SYNC_WRITE_OP_WITH_MUTEX)
#include <mutex>
#endif

#include "esp_err.h"
#include "esp_http_server.h"

#include "result.h"
#include "wscodes.h"

namespace http::socket {
	
	#if defined(SYNC_WRITE_OP_WITH_MUTEX) 
	extern std::mutex m;
	#endif
	
	class asyncSocket {

		//must be mutable to =operator()
		httpd_handle_t hd;
		int			   fd;

		public:
							
			typedef std::string message;
			typedef work::handler_type callback_type;

			asyncSocket() noexcept : hd(nullptr), fd(0) {} ; //only use as memory allocator
			
			explicit asyncSocket(httpd_handle_t hd, int fd) noexcept;
			
			bool	valid() const noexcept;

			resBool write(const uint8_t* buffer, size_t size, 	httpd_ws_type_t type = httpd_ws_type_t::HTTPD_WS_TYPE_BINARY) noexcept;
			
			resBool write(const message& msg, 					httpd_ws_type_t type = httpd_ws_type_t::HTTPD_WS_TYPE_TEXT	) noexcept;
			
			resBool write(const char* msg, 						httpd_ws_type_t type = httpd_ws_type_t::HTTPD_WS_TYPE_TEXT	) noexcept;

			//this function queued it execution
			resBool close(wscodes code = wscodes::NORMAL_CLOSE, const uint8_t* buffer = nullptr, size_t size = 0, callback_type&& callback = nullptr) noexcept;

			//this function queued it execution
			inline resBool close(const wscodes code = wscodes::NORMAL_CLOSE, const char* reason = nullptr, callback_type&& callback = nullptr) noexcept {
				//at this point we not transfer NIL and end of string
				return close(code, (const uint8_t*)reason, reason == nullptr ? 0 : strlen(reason), std::move(callback));
			}

			//this function queued it execution
			inline resBool close(const wscodes code = wscodes::NORMAL_CLOSE, const std::string& reason = "", callback_type&& callback = nullptr) noexcept {
				//at this point we not transfer NIL and end of string
				const auto size = reason.size();
				return close(code, size ? (const uint8_t*)reason.data() : nullptr, size, std::move(callback));
			}

	    			//this function queued it execution
			resBool closeWait(wscodes code = wscodes::NORMAL_CLOSE, const uint8_t* buffer = nullptr, size_t size = 0) noexcept;

			//this function queued it execution
			inline resBool closeWait(const wscodes code = wscodes::NORMAL_CLOSE, const char* reason = nullptr) noexcept {
				//at this point we not transfer NIL and end of string
				return closeWait(code, (const uint8_t*)reason, reason == nullptr ? 0 : strlen(reason));
			}

			//this function queued it execution
			inline resBool closeWait(const wscodes code = wscodes::NORMAL_CLOSE, const std::string& reason = "") noexcept {
				//at this point we not transfer NIL and end of string
				const auto size = reason.size();
				return closeWait(code, size ? (const uint8_t*)reason.data() : nullptr, size);
			}

			//this function queued it execution
			inline resBool terminate() const noexcept {
				return httpd_sess_trigger_close(hd, fd);
			}

			inline bool operator==(const asyncSocket& sock) const {
				return hd == sock.hd && fd == sock.fd;
			}
			
			inline httpd_handle_t serverHandle() const noexcept {
				return hd;
			}	
			
			inline int fileDescriptor() const noexcept {
				return fd;
			}	
			
			inline int native() const noexcept {
				return fd;
			}
	};

	static auto noAsyncSocket = asyncSocket(nullptr, -1);
}