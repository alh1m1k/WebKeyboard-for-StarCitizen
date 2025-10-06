#pragma once

#include "generated.h"

#include "esp_http_server.h"
#include "not_found.h"
#include "result.h"
#include "util.h"
#include <math.h>
#include <sys/_stdint.h>


template<class sockets>
class notificationManager {
	
	sockets& pool;

	struct message {
		const std::string			data;
		typename sockets::member    socket;
		notificationManager* 		manager;
		
		bool binary = false;
		
		message(const char* data, typename sockets::member socket, notificationManager* manager)
			:data(data), socket(socket), manager(manager) {};
			
		message(const std::string& data, typename sockets::member socket, notificationManager* manager)
			:data(data), socket(socket), manager(manager) {
				debugIf(LOG_MESSAGES, "message::message", this->data.c_str(), this->data.size());
		};
						
	};
	
	uint32_t messageCounter = 0;
	
	void static staticHandler(void* arg) {
		message* msg = static_cast<message*>(arg);
		debugIf(LOG_MESSAGES, "send message(async)", (void*)arg, " str:data ",  msg->data.c_str(), " ", msg->data.size());
		if (msg->socket.valid()) {
			if (msg->binary) {
				msg->socket.write(msg->data, HTTPD_WS_TYPE_BINARY);
			} else {
				msg->socket.write(msg->data.c_str(), HTTPD_WS_TYPE_TEXT); //keep msg->data.c_str() owervice header willbe damaged fixme
			}
			//msg->socket.write(msg->data, msg->binary ? HTTPD_WS_TYPE_BINARY : HTTPD_WS_TYPE_TEXT);

		} else {
			error("notificationManager::staticHandler reject invalid(outdated) socket");
		}
		delete msg;
	}
	
	resBool queue(message* msg) {
		 debugIf(LOG_MESSAGES, "msg counter", messageCounter++);
		 return  httpd_queue_work(msg->socket.serverHandler(), &staticHandler, (void*)msg);
	} 
	
	template<typename T>
	resBool notifyC(const T& msgText, const sockets::id& idv = ANY_ID, bool binary = false) {
		if (idv == ANY_ID) {
            auto guardian = pool.sharedGuardian(); //moved away from for, to remove warning about different auto type;
			for (auto it = pool.begin(), end = pool.end(); it != end; ++it) {
				auto msg = new message(msgText, *it, this);
				msg->binary = binary;
				if (auto result = queue(msg); !result) {
					error("notificationManager::notifyC fail2queue msg", result.code());
				}
			}
		} else {
			try {
				auto msg = new message(msgText, pool.get(idv), this);
				msg->binary = binary;
				if (auto result = queue(msg); !result) {
                    error("notificationManager::notifyC fail2queue msg", result.code());
                    return result;
				}
			} catch (not_found& e) {
				error("notificationManager::notifyC socket error",idv, e.what());
				return ESP_FAIL;
			}
		}
		
		return ESP_OK;
	}
	
	template<typename T>
	resBool notifyExeptC(const T& msgText, const sockets::id& idv, bool binary) {
        auto guardian = pool.sharedGuardian(); //moved away from for, to remove warning about different auto type;
		for (auto it = pool.begin(), end = pool.end(); it != end; ++it) {
			if (it.unsafeId() == idv) {
				continue;
			}
			auto msg = new message(msgText, *it, this);
			msg->binary = binary;
			if (auto result = queue(msg); !result) {
				error("notificationManager::notifyExeptC fail2queue msg", result.code());
			} else {
				infoIf(LOG_MESSAGES, "queue msg", msg->socket.native(), " ", binary);
			}
		}
		
		return ESP_OK;
	}
	
	public:
	
		typedef typename sockets::id id;
		
		static constexpr id ANY_ID = sockets::ANY_ID; 
	
		notificationManager(sockets& socsPool): pool(socsPool) {}
		
		//todo moveto concepts && constrains
		
		inline resBool notify(const char* msgText, const id& idv = ANY_ID, bool binary = false) {
			return notifyC(msgText, idv, binary);
		}
		
		inline resBool notify(const std::string& msgText, const id& idv = ANY_ID, bool binary = false) {
			return notifyC(msgText, idv, binary);
		}
		
/*		inline resBool notify(const std::string&& msgText, id& idv = ANY_ID) {
			return notifyC(msgText, idv);
		}*/
				
		inline resBool notifyExept(const char* msgText, const id& idv, bool binary = false) {
			return notifyExeptC(msgText, idv, binary);
		}
		
		inline resBool notifyExept(const std::string& msgText, const id& idv, bool binary = false) {
			return notifyExeptC(msgText, idv, binary);
		}
		
/*		inline resBool notifyExept(const std::string&& msgText, id& idv) {
			debug("using move", msgText);
			return notifyExeptC(msgText, idv);
		}*/
				

};