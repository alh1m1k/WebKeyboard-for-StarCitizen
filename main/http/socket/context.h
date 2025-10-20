#pragma once

#include "session/interfaces/iSession.h"

namespace http::socket {

    struct context {
        httpd_req_t* const _handler;
        public:

            typedef std::shared_ptr<session::iSession> session_ptr;

            context(httpd_req_t* handler) : _handler(handler) {}

            inline session_ptr getSession() const {
                if (_handler->sess_ctx != nullptr) {
                    return static_cast<http::session::pointer*>(_handler->sess_ctx)->lock();
                } else {
                    return nullptr;
                }
            }
    };

}