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

	#ifndef RESPONSE_MAX_UNCHUNKED_SIZE
	#define RESPONSE_MAX_UNCHUNKED_SIZE 10*1024
    #endif

    response::bunchSend::bunchSend(httpd_req_t* handler) noexcept : handler(handler), state(0) {}

	response::bunchSend::~bunchSend() {}

    resBool response::bunchSend::write(const uint8_t *buffer, ssize_t size) noexcept {
        if (auto code = httpd_resp_send(handler, (const char*)buffer, size); code == ESP_OK) {
            state = 2;
            return ResBoolOK;
        } else {
            return code;
        }
    }

    response::chunkSend::chunkSend(httpd_req_t* handler) noexcept : handler(handler), state(0) {}

    response::chunkSend::~chunkSend() {
        if (state == 1) {
            const char nilBuff = 0;
            if (auto code = httpd_resp_send_chunk(handler, &nilBuff, 0); code == ESP_OK) {
                state = 2;
            } else {
                error("unable to finish chunkSend op");
            }
        }
    }

    resBool response::chunkSend::write(const uint8_t *buffer, ssize_t size) noexcept {
        if (state > 1) {
            return ESP_FAIL;
        }
        if (auto code = httpd_resp_send_chunk(handler, (const char*)buffer, size); code == ESP_OK) {
            state = size == 0 ? 2 : 1;
            return ResBoolOK;
        } else {
            return code;
        }
    }

	response::response(const request& req) noexcept : _request(req) {}

	response::~response() {}

	response::bunchSend& response::bunch() {
		switch (_backend.index()) {
			case 0: //monostate
				_backend = bunchSend(_request.native());
                [[fallthrough]];
			case 1:
				return std::get<bunchSend>(_backend);
			default:
				throw std::logic_error("response mode cannot be changed");
		}
	}

	response::chunkSend& response::chunk() {
		switch (_backend.index()) {
			case 0: //monostate
				_backend = chunkSend(_request.native());
                [[fallthrough]];
			case 2:
				return std::get<chunkSend>(_backend);
			default:
				throw std::logic_error("response mode cannot be changed");
		}
	}

	resBool response::writeChunks(chunkSend& backend, const uint8_t* buffer, ssize_t size) noexcept {
		for (
				ssize_t cursor = 0, chunkSize = std::min(size, RESPONSE_MAX_UNCHUNKED_SIZE);
				cursor < size;
				cursor += chunkSize, chunkSize = std::min(size-cursor, RESPONSE_MAX_UNCHUNKED_SIZE)
		) {
			debugIf(LOG_HTTP, "response::write", cursor, " ", chunkSize);
			backend.write(buffer+cursor, chunkSize);
		}
		return ResBoolOK;
	}
	
	resBool response::write(const char* str) noexcept {
		size_t size = strlen(str);
		return write((const uint8_t*)str, size);
	}

	resBool response::write(const uint8_t* buffer, ssize_t size) noexcept {
		if (std::holds_alternative<std::monostate>(_backend)) {
			if (size > RESPONSE_MAX_UNCHUNKED_SIZE) {
				return writeChunks(chunk(), buffer, size);
			} else {
                debugIf(LOG_HTTP, "response::write", buffer, " ", size);
				return bunch().write(buffer, size);
			}
		} else {
            debugIf(LOG_HTTP, "response::write", buffer, " ", size);
			if (std::holds_alternative<bunchSend>(_backend)) {
				return std::get<bunchSend>(_backend).write(buffer, size);
			} else {
				return std::get<chunkSend>(_backend).write(buffer, size);
			}
		}
	}

	resBool response::status(const codes code) noexcept {
		 return httpd_resp_set_status((httpd_req_t*)_request.native(), codes2Symbols(code));
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
		if (auto code = result.code(); code == HTTP_ERR_HEADERS_ARE_SENT) {
			throw headers_sended("setting status code");
		} else {
			throw bad_api_call("httpd_resp_set_status", code);
		}
	}
    return resp;
}

http::response& operator<<(http::response& resp, const http::contentType ct)	{
    if (auto result = resp.getHeaders().contentType(ct); !result) {
		if (auto code = result.code(); code == HTTP_ERR_HEADERS_ARE_SENT) {
			throw headers_sended("setting content type");
		} else {
			throw bad_api_call("httpd_resp_set_type", code);
		}
	}
    return resp;
}


