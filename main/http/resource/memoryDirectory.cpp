#include "generated.h"
#include "memory.h"
#include "resource.h"
#include "action.h"
#include "util.h"

namespace http::resource {

			
	memory::slot::slot(std::string str): str(str), address(0), size(0) {};
	
	memory::slot::slot(std::string str, int address, size_t size): str(str), address(address), size(size) {};
	
	bool memory::slot::operator==(const slot& other) const {
		return other.str == this->str;
	}
	
	bool memory::slot::operator!=(const slot& other) const {
		return other.str != this->str;
	}
	
	bool memory::slot::operator<(const slot& other) const {
		return other.str < this->str;
	}
	
	handlerRes memory::operator()(request& req, response& resp) {

		auto route = req.getRoute();
		 
		if (route == nullptr) {
			debugIf(LOG_HTTP, "memory::handle request.route not available");
			return codes::INTERNAL_SERVER_ERROR;
		}
		
		auto z = req.getUri().path();
		auto routePath = route->path();
		
		debugIf(LOG_HTTP, "memory::handle route base", routePath, " uri ", z);
		
		if (!z.starts_with(routePath)) {
			return codes::INTERNAL_SERVER_ERROR;
		}
		
		auto slice = std::string_view(z).substr(routePath.size());
		
		debugIf(LOG_HTTP, "memory::handle requested resouce", slice);
		
		if (auto it = std::find(slots.begin(), slots.end(), slot(std::string(slice.begin(), slice.end()))); it == slots.end()) {
			return codes::NOT_FOUND;
		} else {
			resp << *it;
		}
		
		return (esp_err_t)ESP_OK;
	}
				
		
	handlerRes memory::handle(request& req, response& resp) {

/*		auto z = req.getUri().path();
		
		debug("memory::handle", z);
		
		if (!z.starts_with(basePath())) {
			return codes::INTERNAL_SERVER_ERROR;
		}
		
		auto slice = std::string_view(z).substr(basePath().size());
		
		debug("memory::handle requested resouce", slice);
		
		if (auto it = std::find(slots.begin(), slots.end(), slot(std::string(slice.begin(), slice.end()))); it == slots.end()) {
			return codes::NOT_FOUND;
		} else {
			resp << *it;
		}*/
		
		return (esp_err_t)ESP_OK;
	}
		
	memory::memory(): resource(), slots({}) {};
	memory::memory(std::initializer_list<slot> slots): resource(), slots(slots) {};
			
	int memory::type() const {
		return (int)map::MEMORY;
	}
	
	response& operator<<(response& stream, const memory::slot& slot) {
		debugIf(LOG_HTTP, "dump memory:slot", (void*)slot.address, " ", slot.size);
		stream.writeResp((const char*)slot.address);
		return stream;
	}
}
