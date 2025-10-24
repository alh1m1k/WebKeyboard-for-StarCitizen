#pragma once

#include "generated.h"

#include <algorithm>
#include <string>
#include <functional>

#include "esp_err.h"

#include "request.h"
#include "response.h"
#include "server.h"
#include "socket/handler.h"


//class represent memory slot for path, callback and esp_handler
namespace http {

    namespace socket {
        class handler;
    }
	
	class server;

	class action {
        public:

            typedef result<codes> handler_res_type;
            typedef std::function<handler_res_type(request& req, response& resp, server& serv)> handler_type;

        private:

            friend server;

		public:

            handler_type        callback;
            server* const       owner;

			action(handler_type&& _callback, server* owner);
			
			action(const action& copy);
			
			action(action&& move);
			
			virtual ~action();

			handler_res_type operator()(request& req, response& resp);

            inline bool isWebSocket() const {
                return callback.target<http::socket::handler>();
            }
	};
}

