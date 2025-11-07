#pragma once

namespace syncing {

    class dummy {
        public:
            inline void lock() {};
            inline bool try_lock() { return true; };
            inline void unlock() {};
            inline void lock_shared() {};
            inline bool try_lock_shared() { return true; };
            inline void unlock_shared() {};
    };

}