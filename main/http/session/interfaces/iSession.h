#pragma once

#include <string>

#include "iCast.h"
#include "../traits.h"

namespace http::session {

    class iSession : virtual public iCast {
        protected:
            virtual void invalidate() noexcept = 0;     //this one is called by baseManager instances making this session invalid
                                                        //this is steep of session closing, before manager remove own ptr to that session
                                                        //after that point, session no longer managed, but it will exist in memory
                                                        //until last shared_ptr is moved away
                                                        //after called, valid() must return false.

            virtual void update(int64_t timestamp) noexcept = 0; //prolong session live time, called on every open. Does not restore invalid session.
                                                                 //invalidate session if (timestamp - prevTimestamp > sessionLiveTime)
                                                                 //session that valid after ::update always valid before next open or invalidate call

            explicit iSession(std::string&& sid, uint32_t index) noexcept;   //called by manager, on open
        public:

            static constexpr uint32_t TRAIT_ID = (uint32_t)traits::I_SESSION;

            typedef std::string sid_type;
            typedef uint32_t    index_type;

            iSession() {};   //used only as delivered constructor
            virtual ~iSession() {}; //virtual destructor must present even for interface classes

            virtual const sid_type&     sid()                       const = 0;
            virtual       bool          valid()                     const = 0; //this session is garbage, there is no turned back
            virtual       bool          expired(int64_t timestamp)  const = 0; //this session is expired, it will-be ::invalidate at next open or at gc cycle
            virtual       index_type    index()                     const = 0;

	};

}