#pragma once

#include <string>

#include "traits.h"
#include "iCast.h"

namespace http::session {

    class iTest : virtual public iCast {
        protected:

            virtual uint32_t incSocketCounter(int32_t delta)  = 0;

        public:

            static constexpr uint32_t TRAIT_ID = (uint32_t)traits::TEST;

            iTest(uint32_t& traits) { traits |= TRAIT_ID; };   //used only as delivered constructor
            virtual ~iTest() {}; //virtual destructor must present even for interface classes

            inline bool socketCounter() {
                return true;
            }

	};

}