#include "action.h"

#include "bad_api_call.h"
#include "esp_err.h"
#include "util.h"

namespace http {
		
	action::action(handler_type&& callback, server* owner): callback(std::move(callback)), owner(owner) {};
	
	action::action(const action& copy): callback(copy.callback), owner(copy.owner) {};
	
	action::action(action&& move): callback(std::move(move.callback)), owner(move.owner) {};
	
	action::~action() {}

    action::handler_res_type action::operator()(request& req, response& resp) {
		if (this->callback == nullptr) {
			throw bad_api_call("route::operator() callback is nullptr", ESP_FAIL);
		}
        if (this->owner == nullptr) {
            throw bad_api_call("route::operator() _owner is nullptr", ESP_FAIL);
        }
		return this->callback(req, resp, *this->owner); //todo change sig to ptr
	}
}

