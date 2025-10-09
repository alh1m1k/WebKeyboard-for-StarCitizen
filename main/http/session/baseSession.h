#pragma once

#include <string>

namespace http::session {

	class baseSession {
        public:
            virtual ~baseSession() {}; //virtual destructor must present even for interface classes
            virtual const std::string& sid() = 0;
	};

}