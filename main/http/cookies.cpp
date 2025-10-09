#include "cookies.h"

#include "esp_http_server.h"

#include "not_implemented.h"
#include "generated.h"

namespace http {
		
	cookies::cookies(): _handler(nullptr) {};
	
	//cookies::cookies(const char* str) {};
		
	cookies::cookies(const httpd_req_t* esp_req): _handler(esp_req) {};

    //this one used to read data that already write by .set
    //ie used in response.getCookies().get*(
    result<const cookie> cookies::get(const std::string& key, uint16_t expectedSize) {
        for (auto it = preserve.begin(); it != preserve.end(); ++it) {
            if (auto cook = cookie(*it); cook.name == key) {
                debugIf(LOG_SESSION, "retrieve cookie", cook.string().c_str());
                return cook;
            }
        }
        debugIf(LOG_SESSION, "retrieve cookie fail", key.c_str());
        return ESP_ERR_NOT_FOUND;
    }

    //this one used to read data from incoming request
    //ie used in request.getCookies().get*(
    result<const cookie> cookies::get(const std::string& key, uint16_t expectedSize) const {

        std::string buffer = {};
        buffer.resize(expectedSize);
        size_t size = buffer.capacity();

        auto result = httpd_req_get_cookie_val((httpd_req_t*)_handler, key.c_str(), buffer.data(), &size);
        if (result == ESP_ERR_HTTPD_RESULT_TRUNC) {
            buffer.resize(size);
            result = httpd_req_get_cookie_val((httpd_req_t*)_handler, key.c_str(), buffer.data(), &size);
        }

        if (result == ESP_OK) {
            debugIf(LOG_SESSION, "read cookie", buffer);
            return cookie(std::move(buffer));
        } else {
            debugIf(LOG_SESSION, "read cookie fail", key.c_str());
            return result;
        }

    }

    resBool cookies::set(const cookie& cook) {
        return set(cook.string());
    }

    //this one used to write data to response, and make it available to non const read
    //ie used in request.getCookies().set*(
    resBool cookies::set(const std::string&& entry) {
        if (auto result = httpd_resp_set_hdr((httpd_req_t*)_handler, "Set-Cookie", entry.c_str()); result) {
            preserve.push_back(std::move(entry));
            debugIf(LOG_SESSION, "write cookie", entry.c_str());
            return result;
        } else {
            return result;
        }
    }
}
