#pragma once

#include <string>

#include "result.h"
#include "baseSession.h"
#include "request.h"

namespace http::session {

    class baseManager {

        public:

            typedef baseSession session_type;

            virtual ~baseManager() {};
            virtual result<baseSession*> open(const request* context = nullptr) = 0;
            virtual result<baseSession*> open(const std::string& sid, const request* context = nullptr) = 0;
            virtual resBool close(const std::string& sid) = 0;
    };

}
