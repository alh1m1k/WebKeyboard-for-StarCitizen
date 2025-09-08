#pragma once

#include "generated.h"

#include "esp_http_server.h"
#include <cstddef>
#include <string_view>
#include <sys/_stdint.h>

#include "codes.h"
#include "cookies.h"
#include "request.h"
#include "../exception/bad_api_call.h"
#include "../exception/not_implemented.h"
#include "server.h"
#include "../result.h"
#include "contentType.h"

namespace http {
	
	class server;
	class cookies;
	
	using _cookies = cookies;
	//using _headers = headers;
	
	class response {

		friend server;
				
		const request& req;
		
		bool _chunked 		= false;
		
		bool _sended  		= false;
		
		bool _headerSended  = false;
		
		size_t _bytes = 0;
				
		//explicit response(const httpd_req_t* esp_req);
		
		public:
		
			static const ssize_t MAX_UNCHUNKED_SIZE;
		
			explicit response(request& req);
						
			response(const response&& resp);
			
			virtual ~response();
			
			response& operator=(const response& resp);
			
			void done();
									
			resBool writeChunk(const uint8_t* buffer, ssize_t size) noexcept; //most low lvl write
			
			resBool write(const uint8_t* buffer, ssize_t size) noexcept; //generic write (write or cycle write)
			
			resBool write(const char* str) noexcept; //proxy to write
			
			resBool writeDone() noexcept; //complete chunk tx
						
			resBool writeResp(const uint8_t* buffer, ssize_t size, bool split = true) noexcept; //single write, split - allow to split tx on chunks (write used as backend)
			
			resBool writeResp(const char* buffer, bool split = true) noexcept; //proxy to writeResp
			
			resBool status(const codes code) noexcept;
			
			inline resBool status404() noexcept {
				return status(codes::NOT_FOUND);
			}
			
			inline resBool status500() noexcept {
				return status(codes::INTERNAL_SERVER_ERROR);
			}
			
			inline resBool encoding(const codes code) noexcept {
				throw not_impleneted();
			}
			
			resBool contentType(const contentType ct) noexcept;
			
			inline resBool contentType(const char* ct) noexcept;
			
			inline _cookies cookies() {
				throw not_impleneted();
			}
			
/*			inline _headers headers() {
				throw not_impleneted();
			}*/
			
			inline bool isSended() const noexcept {
				return _sended;
			}
			
			inline bool isHeaderSended() const noexcept {
				return _headerSended;
			}
			
			inline bool isChunked() const noexcept {
				return _chunked;
			}
			
			inline size_t bytesDirect() const noexcept {
				return _bytes;
			}
			
			inline size_t bytesIndirect() const noexcept {
				if (isHeaderSended()) {
					return 1; //todo fix me
				} else {
					return 0;
				}
			}
			
			inline size_t bytesTotal() const noexcept { //direct+indirectBytes
				return bytesDirect() + bytesIndirect();
			}
			
	};	
}

http::response& operator<<(http::response& resp, const std::string_view str);
http::response& operator<<(http::response& resp, const char* str);
http::response& operator<<(http::response& resp, const http::codes);
http::response& operator<<(http::response& resp, const http::contentType);


