#pragma once

#include "generated.h"

#include <cstddef>
#include <string_view>
#include <sys/_stdint.h>

#include "esp_http_server.h"

#include "../exception/bad_api_call.h"
#include "../exception/not_implemented.h"


#include "result.h"
#include "codes.h"
#include "cookies.h"
#include "request.h"


namespace http {

	class cookies;
    class headers;

    namespace session {
        class iSession;
        class iManager;
    }

	class response {

        class bunchSend {
            httpd_req_t* handler;
            public:
                uint8_t     state = 0; //0 init, 1 begin(header sent), 2 done (sent)

                bunchSend(httpd_req_t* handler) noexcept;
                ~bunchSend();

                resBool write(const uint8_t* buffer, ssize_t size) noexcept;
        };

        class chunkSend {
            httpd_req_t* handler;
            public:
                uint8_t state = 0; //0 init, 1 begin(header sent), 2 done (sent)

                chunkSend(httpd_req_t* handler) noexcept;
                ~chunkSend();

                resBool write(const uint8_t* buffer, ssize_t size) noexcept;
        };

		const request& _request;

        mutable std::unique_ptr<headers> _headers;

        std::variant<std::monostate, bunchSend, chunkSend> _backend;

        resBool writeChunks(chunkSend& backend, const uint8_t* buffer, ssize_t size) noexcept;

		public:

			response(const request& req) noexcept;

			virtual ~response();

			response(const response&) = delete;
			
			response& operator=(const response& resp) = delete;

            bunchSend& bunch();

            chunkSend& chunk();

			resBool write(const uint8_t* buffer, ssize_t size) noexcept;
			
			resBool write(const char* str) noexcept; //proxy to write

			resBool status(const codes code) noexcept;
			
			inline resBool status404() noexcept {
				return status(codes::NOT_FOUND);
			}
			
			inline resBool status500() noexcept {
				return status(codes::INTERNAL_SERVER_ERROR);
			}
			
			inline resBool encoding(const codes code) {
				throw not_impleneted();
			}

            headers& getHeaders();
			
			cookies& getCookies();

			inline const request& getRequest() const {
				return _request;
			}

            std::shared_ptr<session::iSession> getSession() const;

			const network& getRemote() const;

			const network& getLocal() const;

			inline bool isSent() const noexcept {
                //std::variant<std::monostate, bunchSend, chunkSend> _backend;
                switch (_backend.index()) {
                    case 1:
                        return std::get<bunchSend>(_backend).state == 2;
                    case 2:
                        return std::get<chunkSend>(_backend).state == 2;
                    default:
                        return false;
                }
			}

			inline bool isHeaderSent() const noexcept {
                //std::variant<std::monostate, bunchSend, chunkSend> _backend;
                switch (_backend.index()) {
                    case 1:
                        return std::get<bunchSend>(_backend).state >= 1;
                    case 2:
                        return std::get<chunkSend>(_backend).state >= 1;
                    default:
                        return false;
                }
			}

			inline bool isChunked() const noexcept {
				return std::holds_alternative<chunkSend>(_backend);
			}
	};	
}

http::response& operator<<(http::response& resp, const std::string_view str);
http::response& operator<<(http::response& resp, const char* str);
http::response& operator<<(http::response& resp, const http::codes);
http::response& operator<<(http::response& resp, const http::contentType);


