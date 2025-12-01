#include "action.h"

#include "bad_api_call.h"
#include "esp_err.h"
#include "util.h"

namespace http {
		
	action::action(handler_type&& callback) noexcept : callback(std::move(callback))  {};
	
	action::action(const action& copy) noexcept : callback(copy.callback) {};
	
	action::action(action&& move) noexcept : callback(std::move(move.callback)) {};

	action& action::operator=(action&& move) noexcept {
		callback = std::move(move.callback);
		return *this;
	}
	
	action::~action() noexcept {}

    action::handler_res_type action::operator()(request& req, response& resp, server& serv) {
		if (this->callback == nullptr) {
			throw bad_api_call("route::operator() callback is nullptr", ESP_FAIL);
		}
		return this->callback(req, resp, serv);
	}
}

