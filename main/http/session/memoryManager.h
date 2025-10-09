#pragma once

#include <shared_mutex>

#include "baseManager.h"
#include "generated.h"

namespace http::session {

#define SESSION_ERROR_BASE      10000
#define NO_SLOT_SESSION_ERROR   (SESSION_ERROR_BASE + 1)
#define COLLISION_SESSION_ERROR (SESSION_ERROR_BASE + 2)
#define INVALID_SESSION_ERROR   (SESSION_ERROR_BASE + 3)

    class memoryManager: public baseManager {

        struct {
            mutable std::shared_mutex rwMutex;
            baseSession* data[SESSION_MAX_CLIENT_COUNT] = {nullptr};
            size_t length = 0;
        } storage;

        protected:

            virtual std::string generateSID(const request* context = nullptr);

            bool place(baseSession* session);
            bool remove(const std::string& sid);
            baseSession* find(const std::string& sid) const;
            bool validateSession(baseSession* session, const request* context = nullptr) const;

        public:

            static constexpr size_t total =  SESSION_MAX_CLIENT_COUNT;

            virtual ~memoryManager() = default;
            result<baseSession*> open(const request* context = nullptr) override;
            result<baseSession*> open(const std::string& sid, const request* context = nullptr) override;
            resBool close(const std::string& sid) override;
    };

}
