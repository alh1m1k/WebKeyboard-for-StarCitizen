#pragma once

#include <string>

#include "iCast.h"
#include "iBaseSession.h"
#include "../traits.h"
#include "socket/asyncSocket.h"

namespace http::session {

    class iWebSocketSession : virtual public iCast, virtual public iBaseSession {

        public:

            static constexpr uint32_t TRAIT_ID = (uint32_t)traits::I_WEB_SOCKET_SESSION;

            typedef http::socket::asyncSocket socket_type;

            iWebSocketSession() {};   //used only as delivered constructor
            virtual ~iWebSocketSession() {}; //virtual destructor must present even for interface classes

            virtual void setWebSocket(const socket_type& socket)        = 0;
            virtual bool updateWebSocketIfEq(const socket_type& ifEqualToThis, const socket_type& setToThis) = 0;
            virtual socket_type& getWebSocket() = 0;

	};

}