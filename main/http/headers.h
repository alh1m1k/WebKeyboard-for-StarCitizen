#pragma once

#include "generated.h"

#include <sys/_stdint.h>
#include <cstring>

#include "esp_http_server.h"
#include "esp_err.h"

#include "parsing_error.h"

#include "contentType.h"
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

			explicit headers(httpd_req_t* handler);
			
			bool has(const char* headerId) const noexcept;
								
			result<std::string> getResult(const char* headerId) const noexcept;
			
			std::string get(const char* headerId) const noexcept;

			resBool set(const char* headerId, const char* str) noexcept;
			
			uri& getUri() const;
			
			cookies& getCookies() const;
			
			inline std::string accept() const noexcept {
				return get("Accept");
			}
			
			inline std::string acceptEncoding() const noexcept {
				return get("Accept-Encoding");
			}
			
			inline std::string acceptLanguage() const noexcept {
				return get("Accept-Language");
			}
			
			inline std::string cacheControl() const noexcept {
				return get("Cache-Control");
			}
			
			inline std::string connection() const noexcept {
				return get("connection");
			}
			
			inline std::string dht() const noexcept {
				return get("DHT");
			}
			
			inline std::string host() const noexcept {
				return get("Host");
			}
			
			inline std::string cookies_v() const noexcept {
				return get("Cookies");
			}
			
			inline std::string pragma() const noexcept {
				return get("Pragma");
			}
			
			inline std::string referer() const noexcept {
				return get("Referer");
			}
			
			inline std::string userAgent() const noexcept {
				return get("User-Agent");
			}

            inline resBool contentEncoding(const std::string& str) noexcept {
                return set("Content-Encoding", str.c_str());
            }

            inline resBool contentLength(const std::string& str) noexcept {
                return set("Content-Length", str.c_str());
            }

			inline resBool contentLength(size_t size) noexcept {
				return contentLength(std::to_string(size));
			}

            inline resBool contentType(const enum contentType& ct) noexcept {
                return contentType(contentType2Symbols(ct));
            }

            inline resBool contentType(const std::string& ct) noexcept {
                return contentType(ct.c_str());
            }

            inline resBool contentType(const char* ct) noexcept {
                return httpd_resp_set_type((httpd_req_t*)handler, ct);
            }

            inline resBool server(const std::string& str) noexcept {
                return set("Server", str.c_str());
            }

            inline resBool setCookie(const std::string& str) noexcept {
                return set("Set-Cookie", str.c_str());
            }

            inline resBool connection(const std::string& str) noexcept {
                return set("Connection", str.c_str());
            }
	};
	
}