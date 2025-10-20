#include "handler.h"

#include "util.h"


namespace http::socket {
					
	handler::handler(handler_type& 	hndl): hndl(hndl){ debugIf(LOG_SOCKET, "socks handler ref(copy)"); };
	
	handler::handler(handler_type&& 	hndl): hndl(hndl){ debugIf(LOG_SOCKET, "socks handler move"); };
							
	handlerRes handler::operator()(request& req, response& resp, server& serv) {
	    if (req.isGet()) {
	        debugIf(LOG_SOCKET, "socket::operator() handshake");
            if (auto webSockSess = session::pointer_cast<session::iWebSocketSession>(req.getSession()); webSockSess != nullptr) {
                webSockSess->setSocket(socket(req.native()).keep());
            }
	        return ESP_OK;
	    }
	    debugIf(LOG_SOCKET, "socket::operator()");
	    auto res = handle(req, resp, serv); //do not ret directly resBool != esp_err_t != handlerRes, !resBool(handlerRes)
	    return res.code();
	}

}