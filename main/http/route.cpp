#include "route.h"
#include "bad_api_call.h"
#include "esp_err.h"
#include "generated.h"
#include "util.h"
#include <cstring>

//class represent memory slot for path, callback and esp_handler
namespace http {
		
	route::route(std::string_view& path, httpd_method_t mode, handler callback, server& owner): c_str(new char(path.size()+1)), _callback(callback), _owner(owner) {
		
		mempcpy(c_str, path.data(), path.size());
		c_str[path.size()] = 0;
		
		esp_handler.uri 		= c_str;
		esp_handler.method 		= mode;
		esp_handler.user_ctx 	= (void*)this;
		esp_handler.handler  	= nullptr;
		
		if (callback == nullptr) {
			error("route::route is null");
		}
	};
	
	route::route(route& copy): esp_handler(copy.esp_handler), _callback(copy._callback), _owner(copy._owner) {
		delete [] c_str;
		c_str = new char(strlen(copy.c_str));
		strcpy(c_str, copy.c_str);
		debugIf(LOG_HTTP, "route is copied");
	};
	
	route::route(route&& move): c_str(move.c_str), _callback(std::move(move._callback)), _owner(move._owner) {
		esp_handler.uri 		= c_str;
		esp_handler.method 		= move.esp_handler.method;
		esp_handler.user_ctx 	= (void*)this;
		esp_handler.handler  	= move.esp_handler.handler;
		esp_handler.is_websocket  	= move.esp_handler.is_websocket;
		
		move.c_str = nullptr;
		move.esp_handler.handler = nullptr;
		move.esp_handler.user_ctx = nullptr;
		
		if (_callback == nullptr) {
			debugIf(LOG_HTTP, "route::route(route&& move) is null");
		}
	};
	
	route::~route() {
		delete[] c_str;
	}
	
	route& route::operator=(const route& copy) {
		delete [] c_str;
		c_str = new char(strlen(copy.c_str));
		_callback = copy._callback;
		
		esp_handler.uri 		= c_str;
		esp_handler.method 		= copy.esp_handler.method;
		esp_handler.user_ctx 	= (void*)this;
		esp_handler.handler  	= copy.esp_handler.handler;
		
		debugIf(LOG_HTTP, "route is copied");
		
		return *this;
	}
	
	route& route::operator=(route&& move) {
		c_str 		= move.c_str;
		_callback 	= std::move(move._callback);
		
		esp_handler.uri 		= c_str;
		esp_handler.method 		= move.esp_handler.method;
		esp_handler.user_ctx 	= (void*)this;
		esp_handler.handler  	= move.esp_handler.handler;
		esp_handler.is_websocket  	= move.esp_handler.is_websocket;
		
		move.c_str = nullptr;
		move.esp_handler.handler = nullptr;
		move.esp_handler.user_ctx = nullptr;
		
		if (_callback == nullptr) {
			debugIf(LOG_HTTP, "route::operator=(route&& move) is null");
		}
		
		return *this;
	}
	
	bool route::operator==(const route& other) const {
		return (strcmp(esp_handler.uri,  other.esp_handler.uri) == 0) && (esp_handler.method == other.esp_handler.method);
	}
	
	bool route::operator!=(const route& other) const {
		return !(*this == other);
	}
	
	bool route::samePath(const route& other) const {
		return (strcmp(esp_handler.uri,  other.esp_handler.uri) == 0) && (esp_handler.method == other.esp_handler.method);
	}
	
	handlerResult route::operator()(request& req, response& resp) {
		if (this->_callback == nullptr) {
			throw bad_api_call("route::operator() callback is nullptr", ESP_FAIL);
		}
		return this->_callback(req, resp, this->_owner);
	}
}

