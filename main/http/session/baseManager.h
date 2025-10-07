#pragma once

#include <string>

#include "result.h"
#include "baseSession.h"


namespace http::session {

    class baseManager {
        public:
            virtual ~baseManager();
            virtual result<baseSession*> open();
            virtual result<baseSession*> open(const std::string_view id);
            virtual resBool close(const std::string_view id);
    };

}
