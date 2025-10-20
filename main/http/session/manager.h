#pragma once

#include "generated.h"

#include <shared_mutex>
#include <array>

#include "esp_timer.h"

#include "http/session/interfaces/iManager.h"
#include "http/session/session.h"
#include "util.h"

namespace http::session {

#define SESSION_ERROR_BASE      10000
#define NO_SLOT_SESSION_ERROR   (SESSION_ERROR_BASE + 1)
#define COLLISION_SESSION_ERROR (SESSION_ERROR_BASE + 2)
#define INVALID_SESSION_ERROR   (SESSION_ERROR_BASE + 3)


    template<class TSession>
    class manager: public iManager {

        struct {
            mutable std::shared_mutex rwMutex;
            std::array<std::shared_ptr<iSession>, SESSION_MAX_CLIENT_COUNT> data = {nullptr};
            size_t size = 0;
        } storage;

        generator<uint32_t, false> _generator = {};

        protected:

            virtual std::string generateSID(const request* context = nullptr) {
                return genGandom(10);
            }

            virtual uint32_t index() {
                return _generator();
            }

            bool place(std::shared_ptr<iSession>& session) {
                auto guardian = std::unique_lock<std::shared_mutex>(storage.rwMutex);

                if (storage.size >= total) {
                    return false;
                }

                auto timestamp = esp_timer_get_time();

                std::shared_ptr<iSession>* slot = nullptr;
                std::shared_ptr<iSession> expireSession;
                for (size_t i = 0; i < storage.data.size(); ++i) {
                    if (storage.data[i] != nullptr) {
                        if (!storage.data[i]->expired(timestamp)) {
                            if (storage.data[i]->sid() == session->sid()) {
                                return false;
                            }
                        } else if (slot == nullptr) {
                            slot            = &storage.data[i];
                            expireSession   = storage.data[i];
                        }
                    } else if (slot == nullptr) {
                        slot = &storage.data[i];
                    }
                }

                if (slot == nullptr) {
                    return false;
                } else {
                    if (expireSession == nullptr) {
                        storage.size++;
                    }
                    *slot = session;
                }

                if (expireSession != nullptr) {
                    guardian.unlock();
                    invalidateSessionPtr(expireSession);
                }

                return true;
            }

            bool remove(const session_ptr_type& session) {
                auto guardian = std::unique_lock<std::shared_mutex>(storage.rwMutex);

                if (storage.size == 0) {
                    return false;
                }

                for (size_t i = 0; i < total; ++i) {
                    if (storage.data[i] == session) {
                        storage.data[i] = nullptr;
                        return true;
                    }
                }

                return false;
            }

            std::shared_ptr<iSession> find(const std::string& sid) const {
                auto guardian = std::shared_lock(storage.rwMutex);

                if (storage.size == 0) {
                    return nullptr;
                }

                for (size_t i = 0; i < total; ++i) {
                    if (storage.data[i] != nullptr) {
                        debug("check", storage.data[i]->sid().c_str(), " ", sid.c_str(), " ", storage.data[i]->sid().size(), " ", sid.size(), " ",  storage.data[i]->sid() == sid);
                        if (storage.data[i]->sid() == sid) {
                            return storage.data[i];
                        }
                    }
                }

                return nullptr;
            }

            virtual bool validateSession(std::shared_ptr<iSession>& session, const request* context = nullptr) const {
                return true;
            }

            virtual void invalidateSessionPtr(session_ptr_type& sessionPtr) const {
                std::static_pointer_cast<TSession>(sessionPtr)->invalidate();
            }

            virtual void updateSessionPtr(session_ptr_type& sessionPtr) const {
                std::static_pointer_cast<TSession>(sessionPtr)->update(esp_timer_get_time());
            }

        public:

            static constexpr size_t total =  SESSION_MAX_CLIENT_COUNT;

            auto operator=(manager&) = delete;

            virtual ~manager() {
                auto guardian = std::unique_lock(storage.rwMutex);
                for (int i = 0; i < storage.data.size(); ++i) {
                    if (storage.data[i] != nullptr) {
                        invalidateSessionPtr(storage.data[i]);
                        storage.data[i] = nullptr;
                    }
                }
                storage.size = 0;
            }

            result_type open(const request* context = nullptr) override {
                for (int i = 0; i < 3; ++i) {
                    if (storage.size >= total) {
                        return (esp_err_t)NO_SLOT_SESSION_ERROR;
                    }
                    //this one is for allowing to call private/protected constructor via frendship
                    manager::session_ptr_type sess(new TSession(generateSID(context), index()));
                    if (place(sess)) {
                        updateSessionPtr(sess);
                        return sess;
                    } else {
                        errorIf(LOG_SESSION, "session collision", sess->sid().c_str(), " ", i);
                    }
                }
                return (esp_err_t)COLLISION_SESSION_ERROR;
            }

            result_type open(const std::string& sid, const request* context = nullptr) override {
                if (auto sess = find(sid); sess != nullptr) {
                    debug("session founded!");
                    if (validateSession(sess, context)) {
                        if (!sess->expired(esp_timer_get_time())) {
                            updateSessionPtr(sess);
                            return sess;
                        } else {
                            invalidateSessionPtr(sess);
                            remove(sess); //do no block both mutex
                            infoIf(LOG_SESSION, "session expired", sess->sid().c_str());
                            return ESP_ERR_NOT_FOUND;
                        }
                    } else {
                        errorIf(LOG_SESSION, "invalid session", sess->sid().c_str());
                        return INVALID_SESSION_ERROR;
                    }
                } else {
                    return ESP_ERR_NOT_FOUND;
                }
            }

            resBool close(const std::string& sid) override {
                if (storage.size > 0) { //suppose that read and write are atomic
                    if (auto sess = find(sid); sess != nullptr) {
                        invalidateSessionPtr(sess);
                        remove(sess);
                        return ESP_OK;
                    }
                }
                return ESP_ERR_NOT_FOUND;
            }

            inline size_t size() const override {
                //suppose that read and write are atomic
                return storage.size;
            }

            //suppose that collect called not in current request
            //so it some kind save to lock both manager and session
            virtual void collect() override {
                if (storage.size > 0) {
                    auto guardian = std::unique_lock(storage.rwMutex);
                    auto timestamp = esp_timer_get_time();
                    for (size_t i = 0; i < total; ++i) {
                        if (storage.data[i] != nullptr && storage.data[i]->expired(timestamp)) {
                            invalidateSessionPtr(storage.data[i]);
                            storage.data[i] = nullptr;
                            storage.size--;
                        }
                    }
                }
            }
    };

}
