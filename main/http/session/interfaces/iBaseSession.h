#pragma once

#include <string>

#include "iCast.h"
#include "../traits.h"

namespace http::session {

    class iBaseSession { //this one not cast'able

        public:

            typedef std::string sid_type;
            typedef uint32_t    index_type;

            iBaseSession() {};   //used only as delivered constructor
            virtual ~iBaseSession() {}; //virtual destructor must present even for interface classes

            virtual const sid_type&     sid()                       const = 0;
            virtual       bool          valid()                     const = 0; //this session is garbage, there is no turned back
            virtual       bool          expired(int64_t timestamp)  const = 0; //this session is expired, it will-be ::invalidate at next open or at gc cycle
            virtual       index_type    index()                     const = 0;

	};

}