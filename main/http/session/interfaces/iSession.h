#pragma once

#include <string>

#include "iCast.h"
#include "iBaseSession.h"
#include "../traits.h"

namespace http::session {

    class iSession : virtual public iCast, virtual public iBaseSession {
        protected:

            //must be called only by manager, group{ update, expired, invalidate }
            virtual void invalidate(int reason = 0) noexcept = 0;

            //must be called only by manager, group{ update, expired, invalidate }
            virtual void update(int64_t timestamp) noexcept = 0;

            //must be called only by manager, pair{ renew, outdated }
			virtual void renew(const sid_type& sid, int64_t timestamp) noexcept = 0;

            //must be called only by manager
            explicit iSession(std::string&& sid, uint32_t index, int64_t timestamp) noexcept {};

            //inheritance sugar, to not call iSession(std::string&& sid, uint32_t index, int64_t timestamp)
            iSession() noexcept = default;
        public:

            static constexpr uint32_t TRAIT_ID = (uint32_t)traits::I_SESSION;

            //virtual destructor must present even for interface classes
            virtual ~iSession() = default;

            //not safe to call outside manager, group{ update, expired, invalidate }
            virtual bool expired( int64_t timestamp) const = 0;
	};

}