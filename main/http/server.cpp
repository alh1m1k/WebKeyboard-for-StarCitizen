
#include "server.h"

#include <variant>
#include <exception>
#include <stdexcept>


#include "esp_err.h"


#include "bad_api_call.h"
#include "headers_sended.h"

#include "codes.h"
#include "util.h"

#include "socket/handler.h"
#include "session/interfaces/iManager.h"
#include "session/interfaces/iSocksCntSession.h"
#include "session/interfaces/iWebSocketSession.h"
#include "session/pointer.h"


namespace http {

	static void serverError(request& req, response& resp, http::codes code = http::codes::INTERNAL_SERVER_ERROR) {
		if (resp.isHeaderSended()) {
			//nothing we can do as this moment
			info("handler callback return error but headers already send do nothing ",  HTTP_ERR_HEADERS_ARE_SENDED);
			return;
		} else {
            code = code < http::codes::BAD_REQUEST ? http::codes::INTERNAL_SERVER_ERROR : code;
			auto newResp = http::response(req); //move not implemented
			if (auto res = newResp.status(code); !res) {
				error("send api unavailable ", res.code());
			} 
			if (auto res = newResp.writeResp(codes2Symbols(code)); !res) {
				error("send api unavailable ", res.code());
			}
		};
	}
	
	static void serverException(request& req, response& resp, std::exception* e) {
		serverError(req, resp);
	}

    static esp_err_t sessionOpen(request& req, response& resp, route& path) {

        using session_ptr_type = session::iManager::session_ptr_type;

        session::iManager::result_type sessionResult = ESP_FAIL;
        //try reuse if cookie exist
        infoIf(LOG_SESSION, "try session open(reuse)", req.native()->uri, " ", (void *)req.native()->user_ctx);
        if (auto cookieReadResult = req.getCookies().get("wkb-id"); cookieReadResult && std::get<const cookie>(
                cookieReadResult).value != "") {
            if (sessionResult = path.owner()->getSessions()->open(std::get<const cookie>(cookieReadResult).value); sessionResult) {
                infoIf(LOG_SESSION, "session open(reuse)", req.native()->uri, " ", (void *) req.native()->user_ctx, std::get<const cookie>(cookieReadResult).value);
            } else {
                info("session open(reuse) fail", req.native()->uri, " ", (void *) req.native()->user_ctx, " code ", sessionResult.code());
            }
        } else {
            error("cookie get", req.native()->uri, " ", (void *) req.native()->user_ctx, " ", cookieReadResult.code());
        }

        if (!sessionResult) {
            //try open new, cookie not exist
            infoIf(LOG_SESSION, "try session open(new)");
            if (sessionResult = path.owner()->getSessions()->open(); sessionResult) {
                infoIf(LOG_SESSION, "session open(new)", req.native()->uri, " ", (void *) req.native()->user_ctx, " ", std::get<session_ptr_type>(sessionResult)->sid());
                if (auto cookieWriteResult = resp.getCookies().set(cookie("wkb-id", std::get<session_ptr_type>(sessionResult)->sid(), true)); cookieWriteResult) {
                    infoIf(LOG_SESSION, "new cookie write successful", req.native()->uri, " ", (void *) req.native()->user_ctx);
                } else {
                    error("cookie write", req.native()->uri, " ", (void *) req.native()->user_ctx, " code ", cookieWriteResult.code());
                    if (auto closeResult = path.owner()->getSessions()->close(std::get<session_ptr_type>(sessionResult)->sid()); closeResult) {
                        infoIf(LOG_SESSION, "malformed session closed", req.native()->uri, " ", (void *) req.native()->user_ctx);
                    } else {
                        error("session close", req.native()->uri, " ", (void *) req.native()->user_ctx, " code ", closeResult.code());
                    }
                    sessionResult = ESP_FAIL;
                }
            }
        }

        //general session fail drop request
        if (sessionResult) {
            auto absSess = std::get<session_ptr_type>(sessionResult);
            if (req.native()->sess_ctx == nullptr || static_cast<http::session::pointer*>(req.native()->sess_ctx)->lock() != absSess) {
                req.native()->sess_ctx = (void*)(new http::session::pointer{absSess});
                req.native()->free_ctx = (httpd_free_ctx_fn_t)http::session::freePointer;
            }
            if (auto sessionSocks = pointer_cast<session::iSocksCntSession>(req.getSession()); sessionSocks != nullptr) {
                [[maybe_unused]] uint32_t pendingSockets = ++sessionSocks->socketCounter();
                infoIf(LOG_SESSION, "session", std::get<session_ptr_type>(sessionResult)->sid(), " new connection, pendingSockets: ", pendingSockets);
            }
            //if (path.)
            if (auto sessionWS = pointer_cast<session::iWebSocketSession>(req.getSession()); sessionWS != nullptr) {
                infoIf(LOG_SESSION, "session", std::get<session_ptr_type>(sessionResult)->sid(), " new connection");
            }
        } else {
            return sessionResult.code();
        }

        return ESP_OK;
    }

    //this method must process any exception and hold them inside fn border;
	static esp_err_t staticRouter(httpd_req_t *esp_req) {

        route    path   = *static_cast<route*>(esp_req->user_ctx);
		request  req 	= http::request(esp_req, path);
		response resp 	= http::response(req);
		
		if (esp_req->method == 0) {
			info("handle request (websocket)");
		} else {
			info("handle request", esp_req->uri, " m: ", esp_req->method);
		}
		
		debugIf(LOG_HTTP, "---> static routing begin", esp_req->uri, " ", (void*)esp_req->user_ctx);

		try {

            if (!(esp_req->method == 0 && path.isWebSocket())) {
                //auto start session only if it not ongoing websocket com
                //because ws not have cookies, but initial Get have, so socket will rcv it session on protocol upgrade stage until it closes
                if (auto code = sessionOpen(req, resp, path); code != ESP_OK) {
                    error("session open", req.native()->uri, " ", (void *) req.native()->user_ctx, " code ", code);
                    serverError(req, resp, code == NO_SLOT_SESSION_ERROR ? http::codes::TOO_MANY_REQUESTS : http::codes::INTERNAL_SERVER_ERROR);
                    debugIf(LOG_HTTP, "<--- static routing complete", req.native()->uri, " ", (void *) req.native()->user_ctx);
                    return ESP_FAIL;
                }
            }

			auto result = path(req, resp);
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
		} catch (...) {
            return ESP_FAIL;
        }
				
		debugIf(LOG_HTTP, "<--- static routing complete (nothing)", esp_req->uri, " ", (void*)esp_req->user_ctx);
		return ESP_OK;
	}
	
	resBool server::begin(uint16_t port) {
		httpd_config_t config = HTTPD_DEFAULT_CONFIG();
		config.max_uri_handlers 	= 20;
		config.max_open_sockets 	= 12;
		config.stack_size 		   += 1024; //temporaly fix of sovf
		return  httpd_start(&handler, &config);
	}

	resBool server::end() {
		return httpd_stop(handler);
	}
	
	//todo move sem
	resBool server::addHandler(std::string_view path, httpd_method_t mode, const handler_type& callback) {
		
		if (callback == nullptr) {
            throw std::invalid_argument("callback must be provided");
        }
				
		std::lock_guard<std::mutex> guardian = std::lock_guard(routes.m);
		
		route candidate = route(path, mode, callback, this);
		
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
            if (&routes.data[routes.data.size()-1] != routes.data[routes.data.size()-1].esp_handler.user_ctx) {
                throw std::runtime_error("implementation error");
            }
			return ResBoolOK;
		}
	}
		
	resBool server::removeHandler(std::string_view path, httpd_method_t mode) {
		
		std::lock_guard<std::mutex> guardian = std::lock_guard(routes.m);
		
		route candidate = route(path, mode, nullptr, this);
		
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
		
		route candidate = route(path, mode, nullptr, this);
		
		if (auto pos = std::find(routes.data.begin(), routes.data.end(), candidate); pos != routes.data.end()) {
			return true;
		} else {
			return false;
		}		
	}

    void server::setSessions(server::sessions_ptr_type&& manager) {
        auto guardian = std::lock_guard(routes.m);
        if (_sessions != nullptr) {
            throw std::invalid_argument("session cannot be changed after init");
        }
        _sessions = std::move(manager);
    }

    const server::sessions_ptr_type& server::getSessions() const {
        if (_sessions == nullptr) {
            auto guardian = std::lock_guard(routes.m);
            if (_sessions == nullptr) {
                _sessions = std::make_unique<session::manager<session::session<>>>();
            }
        }
        return _sessions;
    }

}