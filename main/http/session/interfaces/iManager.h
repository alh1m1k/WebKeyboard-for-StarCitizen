#pragma once

#include <string>
#include <memory>

#include "result.h"
#include "iSession.h"
#include "request.h"

namespace http {
    class request;
}

namespace http::session {

    class iSession;

    class iManager : virtual public iCast {

        public:

            static constexpr uint32_t TRAIT_ID = (uint32_t)traits::I_MANAGER;

            typedef iSession                             session_type;
            typedef std::shared_ptr<session_type>        session_ptr_type;
            typedef result<session_ptr_type>             result_type;
            typedef uint32_t                             index_type;
            typedef std::string                          sid_type;

            iManager(){};
            virtual ~iManager() {};

            virtual result_type open (const request* context = nullptr) = 0;
            virtual result_type open (const std::string& sid,  const request* context = nullptr) = 0;
            virtual result_type renew(session_ptr_type& sess,  const request* context = nullptr) = 0;
            virtual resBool     close(const std::string& sid) = 0;

            virtual result_type fromIndex(index_type index, bool markAlive = false) = 0;

            virtual size_t count() const  = 0;

            virtual void collect() = 0;
    };

}
