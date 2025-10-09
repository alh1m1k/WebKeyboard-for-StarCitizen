#include "memoryManager.h"

#include "web.h"
#include "util.h"

namespace http::session {

    std::string memoryManager::generateSID(const request* context) {
        return genGandom(10);
    }

    bool memoryManager::place(baseSession* session) {

        auto guardian = std::unique_lock<std::shared_mutex>(storage.rwMutex);

        if (storage.length >= total) {
            return false;
        }

        baseSession** slot = nullptr;
        for (size_t i = 0; i < total; ++i) {
            if (storage.data[i] != nullptr) {
                if (storage.data[i]->sid() == session->sid()) {
                    return false;
                }
            } else if (slot == nullptr) {
                slot = &storage.data[i];
            }
        }

        if (slot == nullptr) {
            return false;
        }

        *slot = session;
        storage.length++;

        return true;

    }

    bool memoryManager::remove(const std::string& sid) {

        auto guardian = std::unique_lock<std::shared_mutex>(storage.rwMutex);

        if (storage.length == 0) {
            return false;
        }

        for (size_t i = 0; i < total; ++i) {
            if (storage.data[i] != nullptr) {
                if (storage.data[i]->sid() == sid) {
                    delete storage.data[i];
                    storage.length--;
                    return true;
                }
            }
        }

        return false;
    }

    baseSession* memoryManager::find(const std::string& sid) const {

        auto guardian = std::shared_lock(storage.rwMutex);

        if (storage.length == 0) {
            return nullptr;
        }

        for (size_t i = 0; i < total; ++i) {
            if (storage.data[i] != nullptr) {
                if (storage.data[i]->sid() == sid) {
                    return storage.data[i];
                }
            }
        }

        return nullptr;
    }

    bool memoryManager::validateSession(baseSession* session, const request* context) const {
        return true;
    }

    result<baseSession*> memoryManager::open(const request* context) {

        for (int i = 0; i < 3; ++i) {
            if (storage.length >= total) {
                return (esp_err_t)NO_SLOT_SESSION_ERROR;
            }
            baseSession* sess = new web(generateSID(context));
            if (place(sess)) {
                return sess;
            } else {
                errorIf(LOG_SESSION, "session collision", sess->sid().c_str(), " ", i);
                delete sess;
            }
        }

        return (esp_err_t)COLLISION_SESSION_ERROR;
    }

    result<baseSession*> memoryManager::open(const std::string& sid, const request* context) {

        if (auto sess = find(sid); sess != nullptr) {
            if (validateSession(sess, context)) {
                return sess;
            } else {
                errorIf(LOG_SESSION, "invalid session", sess->sid().c_str());
                return INVALID_SESSION_ERROR;
            }
        } else {
            return ESP_ERR_NOT_FOUND;
        }

    }

    resBool memoryManager::close(const std::string& sid) {

        if (storage.length > 0) {
            if (remove(sid)) {
                return ESP_OK;
            }
        }

        return ESP_ERR_NOT_FOUND;
    }

}