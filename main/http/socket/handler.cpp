#include "handler.h"

#include "util.h"


namespace http::socket {
					
	handler::handler(handler_type& 	    hndl): hndl(hndl){ debugIf(LOG_SOCKET, "socks handler ref(copy)"); };
	
	handler::handler(handler_type&& 	hndl): hndl(std::move(hndl)) { debugIf(LOG_SOCKET, "socks handler move"); };
							
	handlerRes handler::operator()(request& req, response& resp, server& serv) {
	    if (req.isGet()) {
            if (auto wsSessionPtr = session::pointer_cast<session::iWebSocketSession>(req.getSession()); wsSessionPtr != nullptr) {
				wsSessionPtr->setWebSocket(socket(req.native()).keep());
                debugIf(LOG_SESSION || LOG_SOCKET, "websocket handshake", httpd_req_to_sockfd(req.native()));
            }
	        return ESP_OK;
	    }
	    debugIf(LOG_SOCKET, "socket::operator()");
	    auto res = handle(req, resp, serv); //do not ret directly resBool != esp_err_t != handlerRes, !resBool(handlerRes)
	    return res.code();
	}

}