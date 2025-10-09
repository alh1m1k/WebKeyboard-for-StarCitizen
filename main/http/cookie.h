#pragma once

#include <string>
#include <unordered_map>

#include "esp_http_server.h"

#include "result.h"


namespace http {
	
	class cookie {

        protected:

            mutable std::unique_ptr<std::unordered_map<std::string, std::string>> _heap;
            bool populate(std::string&& key, std::string&& value, uint16_t index);


        public:

            cookie();
            cookie(const std::string& name, const std::string& value, bool httpOnly = false);
            cookie(const std::string&& name, const std::string& value, bool httpOnly = false);
            explicit cookie(const std::string& str);

            cookie(const cookie&  copy);
            cookie(cookie&& move);

            cookie& operator=(const cookie& copy);
            cookie& operator=(cookie&& move);

            std::string name;
            std::string value;
            std::string path;
            int32_t maxAge = -1;

            bool httpOnly = false;


            std::unordered_map<std::string, std::string>& rest() const;
            std::string string() const;
            bool valid() const;
	};
	
}
