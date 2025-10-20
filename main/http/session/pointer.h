#pragma once

#include <memory>

#include "generated.h"

#include "util.h"

#include "traits.h"
#include "http/session/interfaces/iSession.h"
#include "http/session/interfaces/iSocksCntSession.h"
#include "http/session/interfaces/iTest.h"


namespace http::session {

    typedef std::weak_ptr<iSession> pointer;

    template<class T, class U>
    std::shared_ptr<T> pointer_cast(const std::shared_ptr<U>& r) noexcept
    {
        if (r != nullptr) {
            if (auto p = static_cast<typename std::shared_ptr<T>::element_type*>(r.get()->neighbour(T::TRAIT_ID)); p != nullptr) {
                return std::shared_ptr<T>{r, p};
            }
        }

        return std::shared_ptr<T>{nullptr};
    }

    template<class T>
    inline std::shared_ptr<T> acquire(pointer r) noexcept {
        if (auto absSess = r.lock(); absSess != nullptr) {
            return pointer_cast<T>(absSess);
        }
        return nullptr;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
    static void freePointer(pointer* ptr) {
        if (auto absSess = ptr->lock(); absSess != nullptr) {
            if (auto socksSession = pointer_cast<iSocksCntSession>(absSess); socksSession != nullptr) {
                [[maybe_unused]] uint32_t socksCount = --socksSession->socketCounter();
                infoIf(LOG_SESSION, "session", absSess->sid(), " socket close, pendingSockets: ", socksCount);
            } else {
                infoIf(LOG_SESSION, "freePointer", absSess->sid());
            }
        }
        delete ptr;
    }
#pragma GCC diagnostic pop

}