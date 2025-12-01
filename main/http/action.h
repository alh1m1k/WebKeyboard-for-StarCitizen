#pragma once

#include "generated.h"

#include <algorithm>
#include <functional>

#include "esp_err.h"

#include "request.h"
#include "response.h"
#include "socket/handler.h"


namespace http {

    namespace socket {
        class handler;
    }
	class server;

	class action {
        public:

            typedef result<codes> handler_res_type;
            typedef std::function<handler_res_type(request& req, response& resp, server& serv)> handler_type;

            handler_type        callback;

			action(handler_type&& _callback) noexcept;
			
			action(const action& copy) noexcept;
			
			action(action&& move) noexcept;

			action& operator=(action&& move) noexcept;
			
			virtual ~action() noexcept;

			handler_res_type operator()(request& req, response& resp, server& serv);

            inline bool isWebSocket() const noexcept {
                return callback.target<http::socket::handler>();
            }
	};
}

