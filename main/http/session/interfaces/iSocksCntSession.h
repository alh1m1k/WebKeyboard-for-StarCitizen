#pragma once

#include <string>

#include "iCast.h"
#include "../traits.h"

namespace http::session {

    class iSocksCntSession : virtual public iCast {
        protected:

            class attributeHandler {
                iSocksCntSession* const owner;
                public:
                    attributeHandler(iSocksCntSession* owner) : owner(owner) {}
                    inline attributeHandler& operator++() {
                        owner->incSocketCounter(1);
                        return *this;
                    };
                    inline attributeHandler& operator--() {
                        owner->incSocketCounter(-1);
                        return *this;
                    };
                    inline attributeHandler& operator=(uint32_t& value) {
                        owner->setSocketCounter(value);
                        return *this;
                    }
                    inline operator uint32_t() const {
                        return owner->getSocketCounter();
                    }
                    inline attributeHandler& operator=(const attributeHandler& value) {
                        owner->setSocketCounter((uint32_t)value);
                        return *this;
                    }
            };

            virtual void     setSocketCounter(uint32_t value) = 0;
            virtual uint32_t getSocketCounter()               = 0;
            virtual uint32_t incSocketCounter(int32_t delta)  = 0;

        public:

            typedef attributeHandler attribute;

            static constexpr uint32_t TRAIT_ID = (uint32_t)traits::I_SOCKS_CNT_SESSION;

            iSocksCntSession() {};   //used only as delivered constructor
            virtual ~iSocksCntSession() {}; //virtual destructor must present even for interface classes

            virtual attribute socketCounter() {
                return attribute(this);
            }

	};

}