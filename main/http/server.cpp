
#include "server.h"

#include <esp_http_server.h>
#include <variant>
#include <exception>
#include <cassert>

#include "assert.h"
#include "bad_api_call.h"
#include "esp_err.h"
#include "codes.h"
#include "../util.h"
#include "generated.h"
#include "handler.h"
#include "headers_sended.h"
#include "headers.h"
#include "request.h"
#include "response.h"
#include "socket/handler.h"

namespace http {
	
	server::server() {
		//routes.data.reserve(10); //temporal fix ptr invalidation
	}
	
	server::~server() {
		
	}
	
	static void serverError(request& req, response& resp) {
		if (resp.isHeaderSended()) {
			//nothing we can do as this moment
			info("handler callback return error but headers already send do nothing ",  HTTP_ERR_HEADERS_ARE_SENDED);
			return;
		} else {
			auto newResp = http::response(req); //move not implemented
			if (auto res = newResp.status(codes::INTERNAL_SERVER_ERROR); !res) {
				error("send api unavailable ", res.code());
			} 
			if (auto res = newResp.writeResp("500 Internal Server Error"); !res) {
				error("send api unavailable ", res.code());
			}
		};
	}
	
	static void serverException(request& req, response& resp, std::exception* e) {
		serverError(req, resp);
	}
	
	static esp_err_t staticRouter(httpd_req_t *esp_req) {
		
		request  req 	= http::request(esp_req, static_cast<route*>(esp_req->user_ctx));
		response resp 	= http::response(req);
		
		if (esp_req->method == 0) {
			info("handle request (websocket)");
		} else {
			info("handle request", esp_req->uri, " m: ", esp_req->method);
		}
		
		debugIf(LOG_HTTP, "---> static routing begin", esp_req->uri, " ", (void*)esp_req->user_ctx);

		try {
			auto result = (*static_cast<route*>(esp_req->user_ctx))(req, resp);
			
			if (!result) {
				serverError(req, resp);
				error("internal error", esp_req->uri, " ", (void*)esp_req->user_ctx, " code ", result.code());
				debugIf(LOG_HTTP, "<--- static routing complete", esp_req->uri, " ", (void*)esp_req->user_ctx);
				return ESP_FAIL;
			} else {
				if (std::holds_alternative<codes>(result)) {
					if (resp.isHeaderSended()) {
						resp.done();
						info("handler callback return http-code but headers already send, do nothing",  HTTP_ERR_HEADERS_ARE_SENDED);
						debugIf(LOG_HTTP, "<--- static routing complete", esp_req->uri, " ", (void*)esp_req->user_ctx);
						return ESP_OK;
					} else {
						resp.status(std::get<codes>(result));
						resp.done();
						debugIf(LOG_HTTP, "<--- static routing complete", esp_req->uri, " ", (void*)esp_req->user_ctx, " with status code ", result);
						return ESP_OK;
					}
				} else if (std::holds_alternative<esp_err_t>(result)) {
					if (esp_err_t code = std::get<esp_err_t>(result); code == ESP_OK) {
						resp.done();
						debugIf(LOG_HTTP, "<--- static routing complete", esp_req->uri, " ", (void*)esp_req->user_ctx);
						return ESP_OK;
					} else {
						serverError(req, resp);
						error("handler return error code", esp_req->uri, " ", (void*)esp_req->user_ctx, " code ", code);
						debugIf(LOG_HTTP, "<--- static routing complete", esp_req->uri, " ", (void*)esp_req->user_ctx);
						return ESP_FAIL;
					}
				} else {
					serverError(req, resp);
					error("handler undefined behavior", esp_req->uri, " ", (void*)esp_req->user_ctx);
					debugIf(LOG_HTTP, "<--- static routing complete", esp_req->uri, " ", (void*)esp_req->user_ctx);
					return ESP_FAIL;
				}
			}
		
		} catch (headers_sended& e) {
			error("handler throw error", e.what(), e.code());
			debugIf(LOG_HTTP, "<--- static routing complete", esp_req->uri, " ", (void*)esp_req->user_ctx);
		} catch (bad_api_call& e) {
			serverException(req, resp, &e);
			error("server exception", e.what(), ESP_FAIL);
			debugIf(LOG_HTTP, "<--- static routing complete", esp_req->uri, " ", (void*)esp_req->user_ctx);
			return ESP_FAIL;
		} 
				
		debugIf(LOG_HTTP, "<--- static routing complete (nothing)", esp_req->uri, " ", (void*)esp_req->user_ctx);
		return ESP_OK;
	}
	
	resBool server::begin(uint16_t port) {
		httpd_config_t config = HTTPD_DEFAULT_CONFIG();
		config.max_uri_handlers 	= 20;
		config.max_open_sockets 	= 12;
		config.stack_size 			+= 1024; //temporaly fix of sovf
		return  httpd_start(&handler, &config);
	}

	resBool server::end() {
		return httpd_stop(handler);
	}
	
	//todo move sem
	resBool server::addHandler(std::string_view path, httpd_method_t mode, http::handler callback) {
		
		assert(callback != nullptr);
				
		std::lock_guard<std::mutex> guardian = std::lock_guard(routes.m);
		
		route candidate = route(path, mode, callback);
		
		candidate.esp_handler.handler = &staticRouter;
		
		if (callback.target<http::socket::handler>()) {
			candidate.esp_handler.is_websocket = true;
			//candidate.esp_handler.handle_ws_control_frames = false;
			debugIf(LOG_HTTP, "setup new websocket connection");
		}
		
		if (auto pos = std::find(routes.data.begin(), routes.data.end(), candidate); pos != routes.data.end()) {
			if (auto code = httpd_unregister_uri_handler(handler, pos->esp_handler.uri, pos->esp_handler.method); code != ESP_OK) {
				return code;
			}
			
			*pos = std::move(candidate);
			
			if (auto code = httpd_register_uri_handler(handler, &(*pos).esp_handler); code != ESP_OK) {
				routes.data.erase(pos);
				return code;
			}
			
			return ResBoolOK;
		} else {
			routes.data.push_back(std::move(candidate));
			if (auto code = httpd_register_uri_handler(handler, &routes.data[routes.data.size()-1].esp_handler); code != ESP_OK) {
				auto it = routes.data.end()-1;
				routes.data.erase(it);
				return code;
			}
			info("register route", (void*)&routes.data[routes.data.size()-1]);
			assert(&routes.data[routes.data.size()-1] == routes.data[routes.data.size()-1].esp_handler.user_ctx);
			return ResBoolOK;
		}
	}
		
	resBool server::removeHandler(std::string_view path, httpd_method_t mode) {
		
		std::lock_guard<std::mutex> guardian = std::lock_guard(routes.m);
		
		route candidate = route(path, mode, nullptr);
		
		if (auto pos = std::find(routes.data.begin(), routes.data.end(), candidate); pos != routes.data.end()) {
			if (auto code = httpd_unregister_uri_handler(handler, pos->esp_handler.uri, pos->esp_handler.method); code != ESP_OK) {
				return code;
			}
			routes.data.erase(pos);
			return ResBoolOK;
		} else {
			return ResBoolFAIL;
		}		
	}
	
	bool server::hasHandler(std::string_view path, httpd_method_t mode) {
		
		std::lock_guard<std::mutex> guardian = std::lock_guard(routes.m);
		
		route candidate = route(path, mode, nullptr);
		
		if (auto pos = std::find(routes.data.begin(), routes.data.end(), candidate); pos != routes.data.end()) {
			return true;
		} else {
			return false;
		}		
	}
}