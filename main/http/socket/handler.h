#pragma once

#include "esp_err.h"

#include "generated.h"

#include "result.h"
#include "socket.h"
#include "../handler.h"


namespace http::socket {
	
	using http::request;
	using http::response;
	using http::handlerRes;
		
	class handler {
				
		protected:
		
			std::function<resBool(socket& context, const socket::message msg)> hndl;
						
			inline resBool handle(request& req, response& resp, server& serv) {
				
				socket socks = socket(req.native());
				
				if (auto msg = socks.read(); !msg) {
					debug("handler::handle unable 2 read", msg.code());
					return msg.code();
				} else if (hndl != nullptr) {
					return hndl(socks, std::get<socket::message>(msg));
				} else {
					debug("http::socket::handler callback not defined");
				}
				
				return ESP_OK;
			}
		
		public:
			
			typedef std::function<resBool(socket& context, const socket::message& rawMessage)> handler_type;
				
			friend socket& operator<<(socket& stream, const socket::message& result);
						
			handler(handler_type& 	hndl);
			
			handler(handler_type&& 	hndl);
									
			handlerRes operator()(request& req, response& resp, server& serv);
	};
}