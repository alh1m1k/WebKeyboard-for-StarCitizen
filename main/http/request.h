#pragma once

#include <cstddef>
#include  <memory>

#include <esp_http_server.h>

#include "../exception/not_implemented.h"
#include "../exception/invalid_descriptor.h"
#include "parsing_error.h"

#include "uri.h"
#include "cookies.h"
#include "headers.h"
#include "network.h"
#include "http/session/interfaces/iSession.h"
#include "http/session/pointer.h"

namespace http {

	class cookies;
	class headers;
	class action;

	class request {
						
		httpd_req_t* handler;
		
		mutable std::unique_ptr<headers> _headers;

		mutable std::unique_ptr<network> _remoteNetwork;

		mutable std::unique_ptr<network> _localNetwork;
		
		action& _action;

		inline int tryFileDescriptor() const {
			if (auto sock = httpd_req_to_sockfd(handler); sock > 0) {
				return sock;
			} else {
				throw invalid_descriptor("http_request");
			}
		}

		public:
		
			//explicit request(httpd_req_t* esp_req);
			explicit request(httpd_req_t* esp_req, action& action) noexcept;
						
			const headers& getHeaders() const;

			httpd_method_t getMethod() const noexcept;

			const char* getUriRaw() const noexcept; //todo remove me after proper implementing defered object
			
			const uri& getUri() const;

			const cookies& getCookies() const;

			std::shared_ptr<session::iSession> getSession() const;

			inline const action& getRoute() const {
				return _action;
			}

			const network& getRemote() const;

			const network& getLocal() const;


            inline bool isGet() const noexcept  {
                return getMethod() == httpd_method_t::HTTP_GET;
            }

            inline bool isPost() const noexcept {
                return getMethod() == httpd_method_t::HTTP_POST;
            }

			//method needed to expose private handler in order to successfuly build sub object like sockets or headers
			//without using friend decl as it make linking impossible to manage
			inline httpd_req_t* native() const noexcept {
				return handler;
			}
			
	};

    template<class T>
    inline std::shared_ptr<T> session_acquire(request& r) noexcept {
        return session::acquire<T>(*static_cast<session::pointer*>(r.native()->sess_ctx));
    }
}

