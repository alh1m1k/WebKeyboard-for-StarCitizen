#pragma once

#include "session.h"
#include "http/session/manager.h"

class sessionManager: public http::session::manager<class session> {
    public:

        //static constexpr uint32_t TRAIT_ID = (uint32_t)session::TRAIT_ID + 1;

        sessionManager() = default;

        virtual ~sessionManager() = default;

        void invalidateSessionPtr(session_ptr_type& sessionPtr) const override {
            std::static_pointer_cast<session>(sessionPtr)->invalidate();
        }

        void updateSessionPtr(session_ptr_type& sessionPtr, int64_t timestamp) const override {
            std::static_pointer_cast<session>(sessionPtr)->update(timestamp);
        }

        void* neighbour(uint32_t traitId) override {
            switch (traitId) {
                case sessionManager::TRAIT_ID:
                    return static_cast<sessionManager*>(this);
                default:
                    return http::session::manager<session>::neighbour(traitId);
            }
        }

        void* downcast() override {
            return static_cast<sessionManager*>(this);
        }
};