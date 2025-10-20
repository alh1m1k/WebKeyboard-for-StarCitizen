#pragma once

#include <string>

#include "iCast.h"
#include "../traits.h"
#include "socket/asyncSocket.h"

namespace http::session {

    class iWebSocketSession : virtual public iCast {
        protected:

            socket::asyncSocket _socket;

        public:

            static constexpr uint32_t TRAIT_ID = (uint32_t)traits::WEBSOCKET;

            iWebSocketSession(uint32_t& traits) { traits |= (1 << TRAIT_ID); };   //used only as delivered constructor
            virtual ~iWebSocketSession() {}; //virtual destructor must present even for interface classes

            void setSocket(const socket::asyncSocket& socket) {
                _socket = socket;
            }

            socket::asyncSocket& getSocket() {
                return _socket;
            }

	};

}