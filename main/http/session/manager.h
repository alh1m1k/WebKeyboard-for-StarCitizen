#pragma once

#include "generated.h"

#include <shared_mutex>
#include <array>
#include <type_traits>

#include "esp_timer.h"

#include "http/session/interfaces/iManager.h"
#include "http/session/session.h"
#include "util.h"

#include "../../hwrandom.h"
#include "stdlib.h"

namespace http::session {

#define SESSION_ERROR_BASE      10000
#define NO_SLOT_SESSION_ERROR   (SESSION_ERROR_BASE + 1)
#define COLLISION_SESSION_ERROR (SESSION_ERROR_BASE + 2)
#define INVALID_SESSION_ERROR   (SESSION_ERROR_BASE + 3)


    template<class TSession>
    class manager: public iManager {

        struct {
            mutable std::shared_mutex   rwMutex;
            std::array<std::shared_ptr<iSession>, SESSION_MAX_CLIENT_COUNT> data = {nullptr};
            size_t  count = 0;
        } storage;

        generator<uint32_t, false> _generator = {};

        protected:

        template<bool constType>
        class manager_iterator {
            typedef std::array<std::shared_ptr<iSession>, SESSION_MAX_CLIENT_COUNT> container_type;
            typedef std::conditional<constType, container_type::const_iterator, container_type::iterator>::type  internal_it_type;

            internal_it_type    it;
            internal_it_type    begin;
            internal_it_type    end;
            int64_t             timestamp;

            public:
                using difference_type 	= container_type::difference_type;
                using value_type 		= container_type::value_type;
                using pointer 			= std::conditional<constType, container_type::const_pointer, container_type::pointer>::type;
                using reference 		= std::conditional<constType, container_type::const_reference, container_type::reference>::type;
                using iterator_category = std::bidirectional_iterator_tag;

                explicit manager_iterator(internal_it_type begin, internal_it_type end, int64_t timestamp) : it(begin), begin(begin), end(end), timestamp(timestamp) {};
                manager_iterator(const manager_iterator& copy)                  = default;
                manager_iterator(manager_iterator &&move)                       = default;
                    manager_iterator& operator=(const manager_iterator& copy)   = default;
                    value_type& operator*() const {
                        return *it;
                    }
                    auto operator++() {
                        for (++it; it != end; ++it) {
                            if (*it != nullptr && !(*it)->expired(timestamp)) {
                                break;
                            }
                        }
                    }
                    auto operator--() {
                        for (--it; it != begin; --it) {
                            if (*it != nullptr && !(*it)->expired(timestamp)) {
                                break;
                            }
                        }
                    }
                    bool operator==(const manager_iterator& other) const {
                        return other.it == this->it;
                    }
                    bool operator!=(const manager_iterator& other) const {
                        return other.it != this->it;
                    }
                    bool operator>(const manager_iterator& other) const {
                        return other.it > this->it;
                    }
            };

            virtual std::string generateSID(const request* context = nullptr) {
                auto proxy = hwrandom<unsigned>();
                srand(proxy());
                return genGandom(10);
            }

            virtual uint32_t index() {
                return _generator();
            }

            esp_err_t place(std::shared_ptr<iSession>& session, int64_t timestamp) {
                auto guardian = std::unique_lock(storage.rwMutex);

                std::shared_ptr<iSession>* slot = nullptr;
                std::shared_ptr<iSession> expireSession;
                for (size_t i = 0; i < storage.data.size(); ++i) {
                    if (storage.data[i] != nullptr) {
                        if (!storage.data[i]->expired(timestamp)) {
                            if (storage.data[i]->sid() == session->sid()) {
                                return COLLISION_SESSION_ERROR;
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
                    return NO_SLOT_SESSION_ERROR;
                } else {
                    if (expireSession == nullptr) {
                        storage.count++;
                    }
                    *slot = session;
                }

                if (expireSession != nullptr) {
                    guardian.unlock();
                    invalidateSessionPtr(expireSession);
                }

                return ESP_OK;
            }

            bool remove(const session_ptr_type& session) {
                auto guardian = std::unique_lock(storage.rwMutex);

                if (storage.count == 0) {
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

                if (storage.count == 0) {
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

            std::shared_ptr<iSession> find(const index_type index) const {
                auto guardian = std::shared_lock(storage.rwMutex);

                if (storage.count == 0) {
                    return nullptr;
                }

                for (size_t i = 0; i < total; ++i) {
                    if (storage.data[i] != nullptr) {
                        if (storage.data[i]->index() == index) {
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

            virtual void updateSessionPtr(session_ptr_type& sessionPtr, int64_t timestamp) const {
                std::static_pointer_cast<TSession>(sessionPtr)->update(timestamp);
            }

        public:

            static constexpr uint32_t TRAIT_ID = (uint32_t)traits::MANAGER;

            static constexpr size_t total =  SESSION_MAX_CLIENT_COUNT;

            typedef manager_iterator<false> iterator;
            typedef manager_iterator<true>  const_iterator;

            auto operator=(manager&) = delete;

            virtual ~manager() {
                auto guardian = std::unique_lock(storage.rwMutex);
                for (int i = 0; i < storage.data.size(); ++i) {
                    if (storage.data[i] != nullptr) {
                        invalidateSessionPtr(storage.data[i]);
                        storage.data[i] = nullptr;
                    }
                }
                storage.count = 0;
            }

            result_type open(const request* context = nullptr) override {
                auto timestamp = esp_timer_get_time();
                for (int i = 0; i < 3; ++i) {
                    //this one is for allowing to call private/protected constructor via frendship
                    manager::session_ptr_type sess(new TSession(generateSID(context), index()));
                    if (auto code = place(sess, timestamp); code == ESP_OK) {
                        updateSessionPtr(sess, timestamp);
                        return sess;
                    } else if (code == NO_SLOT_SESSION_ERROR) {
                        error("sessions: no slots");
                        return code;
                    } else {
                        errorIf(LOG_SESSION, "session placement error", code, " ", sess->sid().c_str(), " ", i);
                    }
                }
                return (esp_err_t)COLLISION_SESSION_ERROR;
            }

            result_type open(const std::string& sid, const request* context = nullptr) override {
                auto timestamp = esp_timer_get_time();
                if (auto sess = find(sid); sess != nullptr) {
                    if (validateSession(sess, context)) {
                        if (!sess->expired(timestamp)) {
                            updateSessionPtr(sess, timestamp);
                            return sess;
                        } else {
                            invalidateSessionPtr(sess);
                            remove(sess); //do no block both rwMutex
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
                if (storage.count > 0) { //suppose that read and write are atomic
                    if (auto sess = find(sid); sess != nullptr) {
                        invalidateSessionPtr(sess);
                        remove(sess);
                        return ESP_OK;
                    }
                }
                return ESP_ERR_NOT_FOUND;
            }

            result_type fromIndex(index_type index, bool markAlive = false) override {
                auto timestamp = esp_timer_get_time();
                if (auto sess = find(index); sess != nullptr) {
                    if (!sess->expired(timestamp)) {
                        if (markAlive) {
                            updateSessionPtr(sess, timestamp);
                        }
                        return sess;
                    } else {
                        invalidateSessionPtr(sess);
                        remove(sess); //do no block both rwMutex
                        infoIf(LOG_SESSION, "session expired", sess->sid().c_str());
                        return ESP_ERR_NOT_FOUND;
                    }
                } else {
                    return ESP_ERR_NOT_FOUND;
                }
            }

            size_t count() const final {
                if (storage.count > 0) {
                    auto timestamp = esp_timer_get_time();
                    size_t count = 0;
                    auto guardian = std::unique_lock(storage.rwMutex);
                    for (size_t i = 0; i < total; ++i) {
                        if (storage.data[i] != nullptr && !storage.data[i]->expired(timestamp)) {
                            ++count;
                        }
                    }
                    return count;
                }
                return 0;
            }

            size_t size() const  {
                return storage.count;
            }

            //suppose that collect called not in current request
            //so it some kind save to lock both manager and session
            void collect() override {
                if (storage.count > 0) {
                    auto timestamp = esp_timer_get_time();
                    auto guardian = std::unique_lock(storage.rwMutex);
                    for (size_t i = 0; i < total; ++i) {
                        if (storage.data[i] != nullptr && storage.data[i]->expired(timestamp)) {
                            invalidateSessionPtr(storage.data[i]);
                            storage.data[i] = nullptr;
                            storage.count--;
                        }
                    }
                }
            }

            void* neighbour(uint32_t traitId) override {
                switch (traitId) {
                    case iManager::TRAIT_ID:
                        return static_cast<iManager*>(static_cast<manager<TSession>*>(this));
                    case TRAIT_ID:
                        return static_cast<manager<TSession>*>(this);
                    default:
                        return nullptr;
                }
            }

            void* downcast() override {
                return static_cast<manager<TSession>*>(this);
            }

            auto sharedGuardian() const {
                auto guardian = std::shared_lock(storage.rwMutex);
                return guardian;
            }

            iterator begin() {
                return iterator(storage.data.begin(), storage.data.end(), esp_timer_get_time());
            }

            iterator end() {
                return iterator(storage.data.end(), storage.data.end(), esp_timer_get_time());
            }

            const_iterator begin() const {
                return const_iterator(storage.data.begin(), storage.data.end(), esp_timer_get_time());
            }

            const_iterator end() const {
                return const_iterator(storage.data.end(), storage.data.end(), esp_timer_get_time());
            }

    };

}
