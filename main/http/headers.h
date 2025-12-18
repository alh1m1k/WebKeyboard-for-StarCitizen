#pragma once

#include "generated.h"

#include <forward_list>

#include "esp_http_server.h"
#include "esp_err.h"

#include "parsing_error.h"

#include "contentType.h"
#include "result.h"
#include "util.h"
#include "uri.h"
#include "cookies.h"


namespace http {

	class headers {
		
		httpd_req_t* handler;
		
		mutable std::unique_ptr<uri> 		_uri;
		mutable std::unique_ptr<cookies> 	_cookies;

		//unable to use vector as short string optimization
		//and vector expand will break c_str() pointer to header value
		//ie value.c_str() will change after vector::expand() if value.is_short_str()
		//todo make it prt ie std::unique_prt<std::forward_list<std::string>> = nullptr;
		std::forward_list<std::string> preserve = {};

		public:

			explicit headers(httpd_req_t* handler) noexcept;

			~headers() noexcept {}
			
			bool has(const char* headerId) const noexcept;
								
			result<std::string> getResult(const char* headerId) const noexcept;

            result<std::string> getResult(const char* headerId) noexcept;
			
			std::string get(const char* headerId) const noexcept;

            std::string get(const char* headerId);

			resBool c_set(const char* headerId, const char* str) noexcept;

/*			template<std_string_concept TValue>
			resBool set(const char* header_c_str, TValue&& value) noexcept {
				//for some reason std::is_rvalue_reference<TValue> deduces
				//type incorrectly while std::is_rvalue_reference<TValue&&> is valid
				auto value_c_str = value.c_str();
				if constexpr (std::is_rvalue_reference<TValue&&>::value) {
					preserve.push_back(std::move(value));
				}
				return set(header_c_str, value_c_str);
			}*/

			inline resBool set(const char* header_c_str, std::string&& value) noexcept {
				debugIf(LOG_HEADERS, "set by move", header_c_str);
				preserve.push_front(std::move(value));
				return c_set(header_c_str, preserve.front().c_str());
			}

			inline resBool set(const char* header_c_str, const std::string& value) noexcept {
				debugIf(LOG_HEADERS, "set as ref", header_c_str);
				return c_set(header_c_str, value.c_str());
			}

            const uri& getUri() const;

            uri& getUri();
			
			const cookies& getCookies() const;

            cookies& getCookies();
			
			inline std::string accept() const noexcept {
				return get("Accept");
			}

            inline std::string accept() noexcept {
                return get("Accept");
            }
			
			inline std::string acceptEncoding() const noexcept {
				return get("Accept-Encoding");
			}

            inline std::string acceptEncoding() noexcept {
                return get("Accept-Encoding");
            }
			
			inline std::string acceptLanguage() const noexcept {
				return get("Accept-Language");
			}

            inline std::string acceptLanguage() noexcept {
                return get("Accept-Language");
            }
			
			inline std::string cacheControl() const noexcept {
				return get("Cache-Control");
			}

            inline std::string cacheControl() noexcept {
                return get("Cache-Control");
            }
			
			inline std::string connection() const noexcept {
				return get("connection");
			}

            inline std::string connection() noexcept {
                return get("connection");
            }
			
			inline std::string dht() const noexcept {
				return get("DHT");
			}

            inline std::string dht() noexcept {
                return get("DHT");
            }
			
			inline std::string host() const noexcept {
				return get("Host");
			}

            inline std::string host() noexcept {
                return get("Host");
            }
			
			inline std::string cookies_v() const noexcept {
				return get("Cookies");
			}

            inline std::string cookies_v() noexcept {
                return get("Cookies");
            }
			
			inline std::string pragma() const noexcept {
				return get("Pragma");
			}

            inline std::string pragma() noexcept {
                return get("Pragma");
            }
			
			inline std::string referer() const noexcept {
				return get("Referer");
			}

            inline std::string referer() noexcept {
                return get("Referer");
            }
			
			inline std::string userAgent() const noexcept {
				return get("User-Agent");
			}

            inline std::string userAgent() noexcept {
                return get("User-Agent");
            }

			inline std::string ifNoneMatch() const noexcept {
				return get("If-None-Match");
			}

			inline std::string ifNoneMatch() noexcept {
				return get("If-None-Match");
			}

			inline resBool contentEncoding(std::string&& str) noexcept {
                return set("Content-Encoding", std::move(str));
            }

            inline resBool contentLength(std::string&& str) noexcept {
                return set("Content-Length", std::move(str));
            }

			inline resBool contentLength(size_t size) noexcept {
				return contentLength(std::to_string(size));
			}

			inline resBool c_contentType(const char* ct) noexcept {
				return httpd_resp_set_type((httpd_req_t*)handler, ct);
			}

            inline resBool contentType(const enum contentType& ct) noexcept {
                return c_contentType(contentType2Symbols(ct));
            }

            inline resBool contentType(std::string&& str) noexcept {
				preserve.push_front(std::move(str));
                return c_contentType(preserve.front().c_str());
            }

            inline resBool server(std::string&& str) noexcept {
                return set("Server", std::move(str));
            }

            inline resBool setCookie(std::string&& str) noexcept {
                return set("Set-Cookie", std::move(str));
            }

            inline resBool connection(std::string&& str) noexcept {
                return set("Connection", std::move(str));
            }

			inline resBool cacheControl(std::string&& str) noexcept {
				return set("CacheControl", std::move(str));
			}

			//handle str live time
			inline resBool eTag(std::string&& str) noexcept {
				return set("ETag", std::move(str));
			}

			//not handle str live time
			inline resBool eTag(const std::string& str) noexcept {
				return set("ETag", str);
			}

			//not handle str live time
			inline resBool c_eTag(const char* str) noexcept {
				return c_set("ETag", str);
			}
	};
	
}