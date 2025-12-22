#pragma once

#include "generated.h"

#include <vector>

#include "esp_err.h"
#include "esp_http_server.h"

#include "util.h"
#include "not_found.h"
#include "result.h"
#include "session.h"

using namespace std::literals;

template<class TSessions>
class notificationManager {

    httpd_handle_t  handler;
    TSessions&      pool;

    struct message {
        const std::string			                 data;
        std::vector<typename TSessions::index_type>  to;
        notificationManager* 		                 manager;

        bool binary = false;

        message(const std::string& data, std::vector<typename TSessions::index_type>&& to, bool binary, notificationManager* manager)
                : data(data), to(std::move(to)), manager(manager), binary(binary) {
            debugIf(LOG_MESSAGES, "message::message", this->data.c_str(), " size: ", this->data.size(), " address cnt: ", to.size());
        };

    };
	
	uint32_t messageCounter = 0;
	
	void static staticHandler(void* arg) {
		auto msg = static_cast<message*>(arg);
		debugIf(LOG_MESSAGES, "send message(async)", (void*)arg, " str:data ",  msg->data.c_str(), " address cnt: ", msg->to.size());
        for (auto i = msg->to.size() - 1; i < msg->to.size(); --i) {
            if (auto sessResult = msg->manager->resolve(msg->to[i]); sessResult) {
                if (auto& socket = std::get<std::shared_ptr<session>>(sessResult)->getWebSocket(); socket.valid()) {
/*                  This code works with garbage sessions that can be created by the client during reconnection when a
                    concurrent socket opening and worker loading occurs, which in the current reality produces two
                    sessions, one of which never opens its socket and will-be collected later by gc*/
                    resBool result = ESP_OK;
                    if (msg->binary) {
                        result = socket.write(msg->data, HTTPD_WS_TYPE_BINARY);
                    } else {
                        //keep msg->data.c_str() owervice header willbe damaged fixme
                        result = socket.write(msg->data.c_str(), HTTPD_WS_TYPE_TEXT);
                    }
                    if (result) {
                        eraseByReplace(msg->to, msg->to.begin() + i);
                    }
                } else {
                    if (socket != http::socket::noAsyncSocket) {
                        auto sessInfo = std::get<std::shared_ptr<session>>(sessResult);
                        error("notificationManager::staticHandler session", sessInfo->sid().c_str(), " have not valid address i: ", sessInfo->index(), " sock:", socket.native());
                    }
                }
            } else {
                error("notificationManager::staticHandler unable to resolve client session");
            }
        }
        delete msg;
	}
	
	resBool queue(message* msg) {
		 debugIf(LOG_MESSAGES, "msg counter", messageCounter++);
		 return httpd_queue_work(handler, &staticHandler, (void*)msg);
	}

    result<std::shared_ptr<session>> resolve(TSessions::index_type& address) {
        if (auto sessResult = pool.fromIndex(address, false); sessResult) {
            return pointer_cast<session>(std::get<typename TSessions::session_ptr_type>(sessResult));
        }
        return ESP_ERR_NOT_FOUND;
    }

    template<typename T>
    resBool notifyC(const T& msgText, bool binary = false) {
        auto guardian = pool.sharedGuardian();
        std::vector<typename TSessions::index_type> aBook = {};
        aBook.reserve(pool.size());
        for (auto it = pool.begin(), end = pool.end(); it != end; ++it) {
            aBook.push_back((*it)->index());
        }
        if (!aBook.empty()) {
            auto msg = new message(msgText, std::move(aBook), binary, this);
            if (auto result = queue(msg); !result) {
                error("notificationManager::notifyC fail2queue msg", result.code());
            }
        }
        return ESP_OK;
    }

    template<typename T>
	resBool notifyC(const T& msgText, const TSessions::index_type& idv, bool binary = false) {
        try {
            auto msg = new message(msgText, {idv}, binary, this);
            if (auto result = queue(msg); !result) {
                error("notificationManager::notifyC fail2queue msg", result.code());
                return result;
            }
        } catch (not_found& e) {
            error("notificationManager::notifyC socket error",idv, e.what());
            return ESP_FAIL;
        }
		return ESP_OK;
	}
	
	template<typename T>
	resBool notifyExceptC(const T& msgText, const TSessions::index_type& idv, bool binary) {
        auto guardian = pool.sharedGuardian();
        std::vector<typename TSessions::index_type> aBook = {};
        aBook.reserve(pool.size());
        for (auto it = pool.begin(), end = pool.end(); it != end; ++it) {
            auto index = (*it)->index();
            if (index != idv) {
                aBook.push_back(index);
            }
        }
        if (!aBook.empty()) {
            auto msg = new message(msgText, std::move(aBook), binary, this);
            if (auto result = queue(msg); !result) {
                error("notificationManager::notifyExceptC fail2queue msg", result.code());
            }
        }
        return ESP_OK;
	}
	
	public:

        typedef typename TSessions::index_type id;

        notificationManager(httpd_handle_t handler, TSessions& socsPool): handler(handler), pool(socsPool) {}
		
		//todo moveto concepts && constrains

		inline resBool notify(const std::string& msgText, const id& idv, bool binary = false) {
			return notifyC(msgText, idv, binary);
		}

        inline resBool notify(const std::string& msgText, bool binary = false) {
            return notifyC(msgText, binary);
        }

		inline resBool notifyExcept(const std::string& msgText, const id& idv, bool binary = false) {
			return notifyExceptC(msgText, idv, binary);
		}

};