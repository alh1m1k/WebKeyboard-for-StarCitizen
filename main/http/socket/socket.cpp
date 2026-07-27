#include "socket.h"

#include <cstring>

#include "esp_http_server.h"

#include "not_implemented.h"

#include "lwip/def.h"
#include "result.h"
#include "util.h"

namespace http::socket {
		
	socket::socket(httpd_req_t* req): req(req) {};
	
	size_t socket::available() noexcept {
		
		httpd_ws_frame_t ws_pkt = {};
	    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
	    
	    //Set max_len = 0 to get the frame len 
	    if (esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0); ret != ESP_OK) {
	        debugIf(LOG_SOCKET, "socket::available failed to get frame len with:", ret);
	        return 0;
	    }
	    
	    return ws_pkt.len;
	}
	
	resBool socket::read(const uint8_t* buffer, const size_t size) noexcept {
		
		httpd_ws_frame_t ws_pkt = {};
	    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
	    ws_pkt.payload 	= (uint8_t*)buffer; //dangerous?
	    ws_pkt.len = size;
		
        return httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        
	}
	
	result<socket::message> socket::read() noexcept {
		
		httpd_ws_frame_t ws_pkt = {};
	    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
	    
	    //Set max_len = 0 to get the frame len 
	    if (esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0); ret != ESP_OK) {
	        error("socket::read failed to get frame len with:", ret);
	        return ret;
	    } else {
			debugIf(LOG_SOCKET, "socket::read frame len:", ws_pkt.len);
		}
	    
	    if (ws_pkt.len) {
	        auto buff = message("", ws_pkt.len);
	        ws_pkt.payload = (uint8_t*)buff.data();
	        //Set max_len = ws_pkt.len to get the frame payload 
	        //c11++ garantied null at the end
	        if (esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len); ret != ESP_OK) {
				error("socket::handle rcv frame with:", ret);
				return ret;
			}

	        return buff;
			
	    } else {
			error("socket::handle got packet with zero len");
			return ResBoolFAIL;
		}
	}
	
	resBool socket::write(const uint8_t* buffer, const size_t size, const httpd_ws_type_t type) noexcept {
		#if defined(SYNC_WRITE_OP_WITH_MUTEX) 
		std::unique_lock guardian {m};	
		#endif
		httpd_ws_frame_t ws_pkt = {};
		ws_pkt.type 	= type;
		ws_pkt.payload 	= (uint8_t*)buffer;
		ws_pkt.len 		= size;
		return httpd_ws_send_frame(req, &ws_pkt);
	}
	
	resBool socket::write(const message& msg, const httpd_ws_type_t type) noexcept {
		debugIf(LOG_SOCKET, "socket::write", msg.size());
		return write((const uint8_t*)msg.data(), msg.size(), type);
	}
	
	resBool socket::write(const char* msg, const httpd_ws_type_t type) noexcept {
		return write((const uint8_t*)msg, strlen(msg), type);
	}

	resBool socket::writeClose(const wscodes code, const uint8_t* buffer, const size_t size) noexcept {
		const auto txBuffSize = size+2;
		const auto codeNetOrder = lwip_htons((int16_t)code);
		if (size != 0) {
			auto txBuff = std::make_unique_for_overwrite<uint8_t[]>(txBuffSize);
			memcpy(txBuff.get(), &codeNetOrder, 2);
			memcpy(&txBuff[2], buffer, size);
			return write(txBuff.get(), txBuffSize, httpd_ws_type_t::HTTPD_WS_TYPE_CLOSE);
		} else {
			return write((uint8_t*)&codeNetOrder, sizeof(codeNetOrder), httpd_ws_type_t::HTTPD_WS_TYPE_CLOSE);
		}
	}
}