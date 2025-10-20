#pragma once

#include <string>
#include <memory>

#include "result.h"
#include "iSession.h"
#include "request.h"

namespace http {
    class request;
}

namespace http::session {

    class iSession;
    class request;

    class iManager {

        public:

            typedef iSession                          session_type;
            typedef std::shared_ptr<session_type>        session_ptr_type;
            typedef result<session_ptr_type>             result_type;

            virtual ~iManager() {};

            virtual result_type open(const request* context = nullptr) = 0;
            virtual result_type open(const std::string& sid, const request* context = nullptr) = 0;
            virtual resBool     close(const std::string& sid) = 0;

            virtual size_t size() const  = 0;

            virtual void collect() = 0;
    };

}
