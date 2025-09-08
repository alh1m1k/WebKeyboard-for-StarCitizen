#include "handler.h"
#include "util.h"


namespace http::socket {
					
	handler::handler(handler_type& 	hndl): hndl(hndl){ debugIf(LOG_SOCKET, "socks handler ref(copy)"); };
	
	handler::handler(handler_type&& 	hndl): hndl(hndl){ debugIf(LOG_SOCKET, "socks handler move"); };
							
	handlerRes handler::operator()(request& req, response& resp) {
	    if (req.isGet()) {
	        debugIf(LOG_SOCKET, "socket::operator() handshake");
	        return ESP_OK;
	    }
	    debugIf(LOG_SOCKET, "socket::operator()");
	    auto res = handle(req, resp); //do not ret directly resBool != esp_err_t != handlerRes, !resBool(handlerRes)
	    return res.code();
	}

}