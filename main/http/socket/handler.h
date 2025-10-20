#pragma once

#include "generated.h"

#include <memory>

#include "esp_err.h"

#include "result.h"
#include "request.h"
#include "response.h"
#include "socket.h"
#include "server.h"
#include "session/interfaces/iWebSocketSession.h"
#include "session/session.h"
#include "session/pointer.h"
#include "context.h"

namespace http {
    class server;
    class response;
}

namespace http::socket {

    typedef result<codes> handlerRes;

	class handler {
				
		protected:
		
			std::function<resBool(socket& remote, const socket::message msg, const context& ctx)> hndl;
						
			inline resBool handle(request& req, response& resp, server& serv) {
				
				socket socks = socket(req.native());

				if (auto msg = socks.read(); !msg) {
					debug("handler::handle unable 2 read", msg.code());
					return msg.code();
				} else if (hndl != nullptr) {
					return hndl(socks, std::get<socket::message>(msg), context(req.native()));
				} else {
					debug("http::socket::handler callback not defined");
				}
				
				return ESP_OK;
			}
		
		public:
			
			typedef std::function<resBool(socket& remote, const socket::message& rawMessage, const context& ctx)> handler_type;
				
			friend socket& operator<<(socket& stream, const socket::message& result);
						
			handler(handler_type& 	hndl);
			
			handler(handler_type&& 	hndl);
									
			handlerRes operator()(request& req, response& resp, server& serv);
	};
}