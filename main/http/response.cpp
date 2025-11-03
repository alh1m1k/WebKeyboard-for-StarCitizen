#include "response.h"

#include "contentType.h"
#include "esp_err.h"
#include "esp_http_server.h"

#include "exception/headers_sended.h"
#include "codes.h"
#include "generated.h"
#include "handler.h"
#include "result.h"
#include "errors.h"
#include "util.h"
#include <cstring>
#include <sys/_stdint.h>
#include <sys/types.h>


namespace http {
	
	const ssize_t response::MAX_UNCHUNKED_SIZE = 1024 * 10;
		
	//response::response(const httpd_req_t* esp_req){}
	response::response(request& req) : _request(req) {}
	
	response::response(const response&& resp) : _request(resp._request) {
		throw not_impleneted("response::move cnstr");
	}
	
	response::~response() {
		
	}
	
	response& response::operator=(const response& resp) {
		throw not_impleneted("response::move =");
		return *this;
	}
	
	void response::done() {
		if (_sended) {
			return;
		}
		if (_chunked) {
			info("request complete (chunked)", this->_request.getUriRaw());
			_chunked = false;
			writeDone();
		} else {
			if (this->_request.getMethod() == 0) {
				info("request complete (websocket)");
			} else {
				info("request complete", this->_request.getUriRaw());
			}
			
		}
		_sended = true;
	}
	
	resBool response::writeChunk(const uint8_t* buffer, ssize_t size) noexcept {
		if (auto code = httpd_resp_send_chunk((httpd_req_t*)_request.native(), (const char*)buffer, size); code == ESP_OK) {
			_headerSended 	= true;
			_chunked 		= true;
			_bytes 		   += size;
			
			return ResBoolOK;
		} else {
			return code;
		}
	}
	
	resBool response::write(const uint8_t* buffer, ssize_t size) noexcept {
		
		ssize_t 	cursor 		= 0;
		ssize_t 	chunkSize 	= std::min(size, response::MAX_UNCHUNKED_SIZE);
		
		do {
			debugIf(LOG_HTTP, "response::write", cursor, " ", chunkSize);
			if (auto code = httpd_resp_send_chunk((httpd_req_t*)_request.native(), (const char*)buffer+cursor, chunkSize); code == ESP_OK) {
				_headerSended 	= true;
				_chunked 		= true;
				_bytes 		   += size;
			} else {
				error("httpd_resp_send_chunk:code", code);
				return code;
			}
			
			cursor += chunkSize;
			chunkSize 	= std::min(size-cursor, MAX_UNCHUNKED_SIZE);
		
		} while(cursor < size);
		
		return ResBoolOK;
		
	}
	
	resBool response::write(const char* str) noexcept {
		size_t size = strlen(str);
		return write((const uint8_t*)str, size);
	}
	
	resBool response::writeDone() noexcept {
		debug("write chunkedDone");
		if (auto code = httpd_resp_send_chunk((httpd_req_t*)_request.native(), nullptr, 0); code == ESP_OK) {
			_chunked 		= true;
			_headerSended 	= true;
			_sended  		= true;
			return ResBoolOK;
		} else {
			return code;
		}
	}
	
	resBool response::writeResp(const uint8_t* buffer, ssize_t size, bool split) noexcept {
		if (split && size > MAX_UNCHUNKED_SIZE) {
			return write(buffer, size);
		}
		if (auto code = httpd_resp_send((httpd_req_t*)_request.native(), (const char*)buffer, size); code == ESP_OK) {
			_chunked 		= false;
			_headerSended 	= true;
			_sended  		= true;
			_bytes 		   += size;
			return ResBoolOK;
		} else {
			return code;
		}
	}
	
	resBool response::writeResp(const char* str, bool split) noexcept {
		size_t size = strlen(str);
		return writeResp((const uint8_t*)str, size, split);
	}
	
	resBool response::status(const codes code) noexcept {
		 if (_headerSended || _sended) {
			 return (esp_err_t)HTTP_ERR_HEADERS_ARE_SENDED;
		 }
		 return httpd_resp_set_status((httpd_req_t*)_request.native(), codes2Symbols(code));
	}
	
	resBool response::contentType(const enum contentType ct) noexcept {
		 return contentType(contentType2Symbols(ct));
	}
	
	resBool response::contentType(const char* ct) noexcept {
		 if (_headerSended || _sended) {
			 return (esp_err_t)HTTP_ERR_HEADERS_ARE_SENDED;
		 }
		 return httpd_resp_set_type((httpd_req_t*)_request.native(), ct);
	}

    headers& response::getHeaders() {
        if (_headers == nullptr) {
            _headers = std::make_unique<headers>(_request.native());
        }
        return *_headers;
    }

    cookies& response::getCookies() {
        return getHeaders().getCookies();
    }

    std::shared_ptr<session::iSession> response::getSession() const {
        return _request.getSession();
    }

	const network& response::getRemote() const {
		return _request.getRemote();
	}

	const network& response::getLocal() const {
		return _request.getLocal();
	}

}

http::response& operator<<(http::response& resp, std::string_view str)	{
    if (auto result = resp.write((const uint8_t*)str.data(), str.size()); !result) {
		throw bad_api_call("httpd_resp_send_chunk", result.code());
	}
    return resp;
}

http::response& operator<<(http::response& resp, const char* str)	{
    if (auto result = resp.write(str); !result) {
		throw bad_api_call("httpd_resp_send_chunk", result.code());
	}
    return resp;
}

http::response& operator<<(http::response& resp, const http::codes code)	{
    if (auto result = resp.status(code); !result) {
		if (auto code = result.code(); code == HTTP_ERR_HEADERS_ARE_SENDED) {
			throw headers_sended("setting status code");
		} else {
			throw bad_api_call("httpd_resp_set_status", code);
		}
	}
    return resp;
}

http::response& operator<<(http::response& resp, const http::contentType ct)	{
    if (auto result = resp.contentType(ct); !result) {
		if (auto code = result.code(); code == HTTP_ERR_HEADERS_ARE_SENDED) {
			throw headers_sended("setting content type");
		} else {
			throw bad_api_call("httpd_resp_set_type", code);
		}
	}
    return resp;
}


