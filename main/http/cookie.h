#pragma once

#include <string>
#include <unordered_map>

#include "esp_http_server.h"

#include "result.h"


namespace http {
	
	class cookie {

        protected:

            mutable std::unique_ptr<std::unordered_map<std::string, std::string>> _heap;
            void populate(std::string&& name_, std::string&& value_, uint16_t index);


        public:

            cookie();
            cookie(std::string name, std::string value, bool httpOnly = false) noexcept;
            explicit cookie(const std::string& cookies);

            cookie(const cookie& copy)  noexcept;
            cookie(cookie&& move) noexcept;

            cookie& operator=(const cookie& copy) noexcept;
            cookie& operator=(cookie&& move) noexcept;

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
