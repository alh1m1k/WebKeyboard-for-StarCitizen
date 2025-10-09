#pragma once

#include "baseSession.h"

namespace http::session {

    class web: public baseSession {
        protected:
            std::string _sid;
        public:

            web(const std::string& id);

            web(std::string&& id);

            virtual ~web() = default;

            const std::string& sid()     override;

            int32_t testValue = 0;
    };

}