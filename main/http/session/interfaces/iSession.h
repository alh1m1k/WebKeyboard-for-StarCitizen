#pragma once

#include <string>

#include "iCast.h"
#include "iBaseSession.h"
#include "../traits.h"

namespace http::session {

    class iSession : virtual public iCast, virtual public iBaseSession {
        protected:
            virtual void invalidate(int reason = 0) noexcept = 0;

            virtual void update(int64_t timestamp) noexcept = 0;

			virtual void renew(const sid_type& sid, int64_t timestamp) noexcept = 0;

            explicit iSession(std::string&& sid, uint32_t index) noexcept {};   //called by manager, on open
        public:

            static constexpr uint32_t TRAIT_ID = (uint32_t)traits::I_SESSION;

            iSession() {};   //used only as delivered constructor
            virtual ~iSession() {}; //virtual destructor must present even for interface classes

	};

}