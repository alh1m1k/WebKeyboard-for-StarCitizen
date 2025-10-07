#pragma once

#include "baseSession.h"

namespace http::session {

        class web: baseSession {
            protected:
                std::string _id;

            public:
                web(const std::string& id)  noexcept;
                const std::string id()      noexcept;
        };

}