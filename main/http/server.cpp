#include "server.h"

#include <variant>
#include <exception>
#include <stdexcept>
#include <lwip/sockets.h>


#include "esp_err.h"


#include "bad_api_call.h"
#include "headers_sended.h"

#include "codes.h"
#include "util.h"

#include "session/interfaces/iManager.h"
#include "session/interfaces/iSocksCntSession.h"
#include "session/interfaces/iWebSocketSession.h"
#include "session/pointer.h"
#include "network.h"


namespace http {
    
    static http::codes err2code(esp_err_t err) {
        switch (err) {
            case HTTP_SESSION_RECOVERY:
                return http::codes::UNAUTHORIZED;
            case NO_SLOT_SESSION_ERROR:
                return http::codes::TOO_MANY_REQUESTS;
            default:
                return http::codes::INTERNAL_SERVER_ERROR;
        }
    }

	static void serverError(request& req, response& resp, http::codes code = http::codes::INTERNAL_SERVER_ERROR) {
		if (resp.isHeaderSended()) {
			//nothing we can do as this moment
			info("handler callback return error but headers already send do nothing ",  HTTP_ERR_HEADERS_ARE_SENDED);
			return;
		} else {
            code = code < http::codes::BAD_REQUEST ? http::codes::INTERNAL_SERVER_ERROR : code;
            debug("serverError", (int)code);
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

    static esp_err_t socketOpen(httpd_handle_t hd, int sockfd) {
        return ESP_OK;
    }

    static void socketClose(httpd_handle_t hd, int sockfd) {
        if (auto weakSessPtr = httpd_sess_get_ctx(hd, sockfd); weakSessPtr != nullptr) {
            if (auto absSessPtr = static_cast<session::pointer*>(weakSessPtr)->lock(); absSessPtr != nullptr) {
                infoIf(LOG_SESSION, "session ", absSessPtr->sid().c_str(), " socket closing: ", sockfd);
                if (auto socketAwareSessionPtr = pointer_cast<session::iSocksCntSession>(absSessPtr); socketAwareSessionPtr != nullptr) {
                    socketAwareSessionPtr->socksCntDecr();
                    infoIf(LOG_SESSION, "     pendingSockets: ", socketAwareSessionPtr->socksCnt());
                }
                if (auto wsSessionPtr = pointer_cast<session::iWebSocketSession>(absSessPtr); wsSessionPtr != nullptr) {
                    if (wsSessionPtr->updateWebSocketIfEq(socket::asyncSocket(hd, sockfd), socket::noAsyncSocket)) {
                        infoIf(LOG_SESSION, "     websocket is closed");
                    }
                }
            }
        }
        close(sockfd);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
    static void freeSessionPtr(void* ptr) {
        debug("freeSessionPtr", ptr);
        if (ptr != nullptr) {
            delete static_cast<session::pointer*>(ptr);
        }
    }
#pragma GCC diagnostic pop


    static esp_err_t sessionOpen(request& req, response& resp, action& path) {

        using session_ptr_type = session::iManager::session_ptr_type;

        session::iManager::result_type sessionResult = ESP_FAIL;
        //try reuse if cookie exist
        infoIf(LOG_SESSION, "try session open(reuse)", req.native()->uri, " t ", esp_timer_get_time());
        if (auto cookieReadResult = req.getCookies().get("wkb-id"); cookieReadResult) {
            if (sessionResult = path.owner->getSessions()->open(std::get<const cookie>(cookieReadResult).value, &req); sessionResult) {
                infoIf(LOG_SESSION, "session open(reuse)", req.native()->uri, " t ", esp_timer_get_time(), std::get<const cookie>(cookieReadResult).value);
                if (auto absSess = std::get<session_ptr_type>(sessionResult); absSess->outdated(esp_timer_get_time())) {
                    infoIf(LOG_SESSION, "session is outdated", req.native()->uri, " t ", esp_timer_get_time());
                    if (sessionResult = path.owner->getSessions()->renew(absSess, &req); sessionResult) {
                        infoIf(LOG_SESSION, "session renew", req.native()->uri, " t ", esp_timer_get_time(), " ", absSess->sid().c_str());
                        auto sidCookie = cookie("wkb-id", absSess->sid(), true);
                        sidCookie.maxAge = SID_COOKIE_TTL;
                        if (auto cookieWriteResult = resp.getCookies().set(sidCookie); cookieWriteResult) {
                            infoIf(LOG_SESSION, "new cookie write successful", req.native()->uri, " t ", esp_timer_get_time());
                        } else {
                            error("cookie write", req.native()->uri, " t ", esp_timer_get_time(), " code ", cookieWriteResult.code());
                            if (auto closeResult = path.owner->getSessions()->close(absSess->sid()); closeResult) {
                                infoIf(LOG_SESSION, "outdated session closed", req.native()->uri, " t ", esp_timer_get_time());
                            } else {
                                error("session close", req.native()->uri, " t ", esp_timer_get_time(), " code ", closeResult.code());
                            }
                            sessionResult = ESP_FAIL;
                        }
                    } else {
                        error("fail to renew outdated session", req.native()->uri, " t ", esp_timer_get_time(), " ", sessionResult.code());
                        if (auto closeResult = path.owner->getSessions()->close(absSess->sid()); closeResult) {
                            infoIf(LOG_SESSION, "outdated session closed", req.native()->uri, " t ", esp_timer_get_time());
                        } else {
                            error("session close", req.native()->uri, " t ", esp_timer_get_time(), " code ", closeResult.code());
                        }
                    }
                }
            } else {
                infoIf(LOG_SESSION, "session open(reuse) fail", req.native()->uri, " t ", esp_timer_get_time(), " code ", sessionResult.code());
            }
        } else {
            error("cookie get", req.native()->uri, " t ", esp_timer_get_time(), " ", cookieReadResult.code());
        }

        if (!sessionResult) {
            //try open new, cookie not exist or invalid
            if (!path.isWebSocket()) {
                infoIf(LOG_SESSION, "try session open(new)");
                if (sessionResult = path.owner->getSessions()->open(&req); sessionResult) {
                    infoIf(LOG_SESSION, "session open(new)", req.native()->uri, " t ", esp_timer_get_time(), " ",
                           std::get<session_ptr_type>(sessionResult)->sid().c_str());
                    auto sidCookie = cookie("wkb-id", std::get<session_ptr_type>(sessionResult)->sid(), true);
                    sidCookie.maxAge = SID_COOKIE_TTL;
                    if (auto cookieWriteResult = resp.getCookies().set(sidCookie); cookieWriteResult) {
                        infoIf(LOG_SESSION, "new cookie write successful", req.native()->uri, " t ", esp_timer_get_time());
                    } else {
                        error("cookie write", req.native()->uri, " t ", esp_timer_get_time(), " code ", cookieWriteResult.code());
                        if (auto closeResult = path.owner->getSessions()->close(
                                    std::get<session_ptr_type>(sessionResult)->sid()); closeResult) {
                            infoIf(LOG_SESSION, "malformed session closed", req.native()->uri, " t ", esp_timer_get_time());
                        } else {
                            error("session close", req.native()->uri, " t ", esp_timer_get_time(), " code ", closeResult.code());
                        }
                        sessionResult = ESP_FAIL;
                    }
                }
            } else {
                sessionResult = (esp_err_t )HTTP_SESSION_RECOVERY;
                //session is missing or closed and recover is not applied or fail.
                //when ws send reconnect attempt(Get Upgrade) it contain cookie sid that can be outdated or invalid
                //(ie unexpected reboot) on such request no custom headers can be send to client
                //ie no way start new session, so such request must use session recovery mode and reOpen new session with
                //predefined ssid (mod 2)
                //also esp_server backend not allow to generate server error for upgrade request (it always response with 101 but then close connection
                //if error happened), so session recovery needed
            }
        }

        //general session fail drop request
        if (sessionResult) {
            auto absSess = std::get<session_ptr_type>(sessionResult);
            if (req.native()->sess_ctx == nullptr || static_cast<http::session::pointer*>(req.native()->sess_ctx)->lock() != absSess) {
                req.native()->sess_ctx = (void*)(new http::session::pointer{absSess});
                req.native()->free_ctx = freeSessionPtr;
            }
            if (auto sessionSocks = pointer_cast<session::iSocksCntSession>(req.getSession()); sessionSocks != nullptr) {
                sessionSocks->socksCntInc();
                infoIf(LOG_SESSION, "session", sessionSocks->sid().c_str(), " new connection, pendingSockets: ", sessionSocks->socksCnt());
            }
            //if (path.)
            if (auto sessionWS = pointer_cast<session::iWebSocketSession>(req.getSession()); sessionWS != nullptr) {
                infoIf(LOG_SESSION, "session", sessionWS->sid().c_str(), " new connection");
            }
        } else {
            return sessionResult.code();
        }

        return ESP_OK;
    }

    static esp_err_t noop(httpd_req_t *esp_req) { return ESP_OK; };

    //this method must process any exception and hold them inside fn border;
	static esp_err_t staticRouter(httpd_req_t *esp_req) {

        action&  path   = *static_cast<action*>(esp_req->user_ctx);
		request  req 	= http::request(esp_req, path);
		response resp 	= http::response(req);

		if (esp_req->method == 0) {
			info("handle request (websocket)");
		} else {
			info("handle request", esp_req->uri, " m: ", esp_req->method);
		}

        if (req.getRemote().version() == 6) {
            std::cout << "[info] clientData -> " << req.getRemote().ipv6() << " " << req.getRemote().mac() << std::endl;
        } else if (req.getRemote().version() == 4) {
            std::cout << "[info] clientData -> " << req.getRemote().ipv4() << " " << req.getRemote().mac() << std::endl;
        }
		
		debugIf(LOG_HTTP, "---> static routing begin", esp_req->uri, " t ", esp_timer_get_time());

		try {

            if (!(esp_req->method == 0 && path.isWebSocket())) {
                //auto start session only if it not ongoing websocket com
                //because ws not have cookies, but initial Get have, so socket will rcv it session on protocol upgrade stage until it closes
                if (auto code = sessionOpen(req, resp, path); code != ESP_OK) {
                    error("session open", req.native()->uri, " t ", esp_timer_get_time(), " code ", code);
                    serverError(req, resp, err2code(code));
                    debugIf(LOG_HTTP, "<--- static routing complete", req.native()->uri, " t ", esp_timer_get_time());
                    return ESP_FAIL;
                }
            }

			auto result = path(req, resp);
			if (!result) {
				serverError(req, resp);
				error("internal error", esp_req->uri, " t ", esp_timer_get_time(), " code ", result.code());
				debugIf(LOG_HTTP, "<--- static routing complete", esp_req->uri, " t ", esp_timer_get_time());
				return ESP_FAIL;
			} else {
				if (std::holds_alternative<codes>(result)) {
					if (resp.isHeaderSended()) {
						resp.done();
						info("handler callback return http-code but headers already send, do nothing",  HTTP_ERR_HEADERS_ARE_SENDED);
						debugIf(LOG_HTTP, "<--- static routing complete", esp_req->uri, " t ", esp_timer_get_time());
						return ESP_OK;
					} else {
						resp.status(std::get<codes>(result));
						resp.done();
						debugIf(LOG_HTTP, "<--- static routing complete", esp_req->uri, " t ", esp_timer_get_time(), " with status code ", result);
						return ESP_OK;
					}
				} else if (std::holds_alternative<esp_err_t>(result)) {
					if (esp_err_t code = std::get<esp_err_t>(result); code == ESP_OK) {
						resp.done();
						debugIf(LOG_HTTP, "<--- static routing complete", esp_req->uri, " t ", esp_timer_get_time());
						return ESP_OK;
					} else {
						serverError(req, resp);
						error("handler return error code", esp_req->uri, " t ", esp_timer_get_time(), " code ", code);
						debugIf(LOG_HTTP, "<--- static routing complete", esp_req->uri, " t ", esp_timer_get_time());
						return ESP_FAIL;
					}
				} else {
					serverError(req, resp);
					error("handler undefined behavior", esp_req->uri, " t ", esp_timer_get_time());
					debugIf(LOG_HTTP, "<--- static routing complete", esp_req->uri, " t ", esp_timer_get_time());
					return ESP_FAIL;
				}
			}
		
		} catch (headers_sended& e) {
			error("handler throw error", e.what(), e.code());
			debugIf(LOG_HTTP, "<--- static routing complete", esp_req->uri, " t ", esp_timer_get_time());
		} catch (std::exception& e) {
			serverException(req, resp, &e);
			error("server exception", e.what(), ESP_FAIL);
			debugIf(LOG_HTTP, "<--- static routing complete", esp_req->uri, " t ", esp_timer_get_time());
			return ESP_FAIL;
		} catch (...) {
            error("server exception",ESP_FAIL);
            return ESP_FAIL;
        }
				
		debugIf(LOG_HTTP, "<--- static routing complete (nothing)", esp_req->uri, " t ", esp_timer_get_time());
		return ESP_OK;
	}

    struct jobWrapper {
        const server::job_type job;
    };
    static void staticJobProcessor(void *arg) {
        auto wrapper = static_cast<jobWrapper*>(arg);
        try {
            wrapper->job();
        } catch (const std::exception& e) {
            error("staticJobProcessorEx", e.what());
        } catch (...) {
            error("staticJobProcessorEx", "undefined exception");
        };
        delete wrapper;
    }

	resBool server::begin(uint16_t port) {
		httpd_config_t config = HTTPD_DEFAULT_CONFIG();
		config.max_uri_handlers 	= 20;
		config.max_open_sockets 	= 42;
		config.stack_size 		   += 1024; //temporaly fix of sovf
        config.open_fn  = &socketOpen;
        config.close_fn = &socketClose;
        return  httpd_start(&handler, &config);
	}

	resBool server::end() {
		return httpd_stop(handler);
	}
	
	//todo move sem
	resBool server::addHandler(const char * url, httpd_method_t mode, handler_type&& callback) {
		
		if (callback == nullptr) {
            throw std::invalid_argument("callback must be provided");
        }

		action* wrapper = new action(std::move(callback), this);

        httpd_uri routeHandler = {
            .uri                    = url,
            .method                 = mode,
            .handler                = &staticRouter,
            .user_ctx               = (void*)wrapper,
            .is_websocket           = wrapper->isWebSocket(),
            .handle_ws_control_frames = false,
            .supported_subprotocol    = nullptr
        };

        if (routeHandler.is_websocket) {
            debugIf(LOG_HTTP, "setup new websocket connection");
        }

        info("register route", (void*)wrapper);

        if (auto code =  httpd_register_uri_handler(handler, &routeHandler); code != ESP_OK) {
            delete wrapper;
            routeHandler.user_ctx = nullptr;
            return code;
        } else {
            return code;
        }
	}
		
	resBool server::removeHandler(const char * url, httpd_method_t mode) {

        /**
         * after refactoring there is a problem
         * esp_server do not expose free_user_ctx fn and do not deallocate user_ctx (or we just use alloc)
         * so there is a memory leak in theory as we allocate new action for it
         *
         * to fix it we must track url for every action we make, but in any other cases the URL is not used
         * or access server private storage to associate uri with it action
         * or patch esp_server and implement free_user_ctx as many esp api do
         *
         * but all this is not important since in reality we do not turn off handlers
         */

        throw not_impleneted();

        return httpd_unregister_uri_handler(handler, url, mode);
	}
	
	bool server::hasHandler(const char * url, httpd_method_t mode) {
        httpd_uri routeHandler = {};
        routeHandler.uri    = url;
        routeHandler.method = mode;
        routeHandler.handler = &noop;

        if (auto code = httpd_register_uri_handler(handler, &routeHandler); code == ESP_OK) {
            httpd_unregister_uri_handler(handler, routeHandler.uri, routeHandler.method);
            return false;
        } else {
            return true;
        }
	}

    void server::collect() {
        if (_sessions != nullptr) {
            _sessions->collect();
        }
    }

    void server::setSessions(server::sessions_ptr_type&& manager) {
        auto guardian = std::lock_guard(_m);
        if (_sessions != nullptr) {
            throw std::invalid_argument("session cannot be changed after init");
        }
        _sessions = std::move(manager);
    }

    const server::sessions_ptr_type& server::getSessions() const {
        if (_sessions == nullptr) {
            auto guardian = std::lock_guard(_m);
            if (_sessions == nullptr) {
                _sessions = std::make_unique<session::manager<session::session<>>>();
            }
        }
        return _sessions;
    }

    resBool server::scheduleJob(job_type&& job) {
        if (handler == nullptr) {
            throw std::logic_error("server not start");
        }
        auto wrapper = new jobWrapper{std::move(job)};
        if (auto code = httpd_queue_work(handler, staticJobProcessor, (void*)wrapper); code != ESP_OK) {
            delete wrapper;
            return code;
        } else {
            return code;
        }
    }

}