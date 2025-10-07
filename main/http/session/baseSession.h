#pragma once

#include <string>

//#include "baseManager.h"


namespace http::session {

	class baseSession {
        protected:
            //baseSession();
            //virtual ~baseSession();
        public:
            virtual const std::string   id()         = 0;
            //other general data there
            bool     authorized                      = false;
            uint32_t incremental                     = 0;
            void*    otherData                       = nullptr;
	};

}