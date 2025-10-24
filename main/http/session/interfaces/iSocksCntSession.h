#pragma once

#include <string>

#include "iCast.h"
#include "iBaseSession.h"
#include "../traits.h"

namespace http::session {

    class iSocksCntSession : virtual public iCast, virtual public iBaseSession {

        public:

            static constexpr uint32_t TRAIT_ID = (uint32_t)traits::I_SOCKS_CNT_SESSION;

            iSocksCntSession() {};          //used only as delivered constructor
            virtual ~iSocksCntSession() {}; //virtual destructor must present even for interface classes

            virtual void    socksCntInc()       = 0;
            virtual void    socksCntDecr()      = 0;
            virtual size_t  socksCnt() const    = 0;
	};

}