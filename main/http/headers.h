#pragma once

#include "generated.h"

#include <sys/_stdint.h>
#include <cstring>

#include "esp_http_server.h"
#include "esp_err.h"

#include "parsing_error.h"

#include "result.h"
#include "util.h"
#include "lib/httpparser/httprequestparser.h"
#include "lib/httpparser/request.h"
#include "uri.h"
#include "cookies.h"


namespace http {
	
	class headers {
		
		httpd_req_t* handler;
		
		mutable std::unique_ptr<uri> 		_uri;  //todo make defered object mutable and const
		mutable std::unique_ptr<cookies> 	_cookies;

		bool populated = false;

		public: 
		
			headers();
			
			headers(httpd_req_t* handler);
			
			bool has(const char* headerId) const;
								
			result<std::string> getResult(const char* headerId) const;
			
			std::string get(const char* headerId) const;
									
			//no way to implement fast buffer, size API
									
			resBool set(const char* headerId, const char* str) const;
			
			uri& getUri() const;
			
			cookies& getCookies() const;
			
			inline std::string accept() const {
				return get("Accept");
			}
			
			inline std::string acceptEncoding() const {
				return get("Accept-Encoding");
			}
			
			inline std::string acceptLanguage() const {
				return get("Accept-Language");
			}
			
			inline std::string cacheControl() const {
				return get("Cache-Control");
			}
			
			inline std::string connection() const {
				return get("connection");
			}
			
			inline std::string dht() const {
				return get("DHT");
			}
			
			inline std::string host() const {
				return get("Host");
			}
			
			inline std::string cookies_v() const {
				return get("Cookies");
			}
			
			inline std::string pragma() const {
				return get("Pragma");
			}
			
			inline std::string referer() const {
				return get("Referer");
			}
			
			inline std::string userAgent() const {
				return get("User-Agent");
			}
			
			resBool contentEncoding(std::string& str) const;
			
			resBool contentEncoding(std::string&& str) const;
			
			resBool contentLength(std::string& str) const;
			
			resBool contentLength(std::string&& str) const;
			
			resBool contentType(std::string& str) const;
			
			resBool contentType(std::string&& str) const;
			
			resBool server(std::string& str) const;
			
			resBool server(std::string&& str) const;
			
			resBool setCookies(std::string& str) const;
			
			resBool setCookies(std::string&& str) const;
	};
	
}