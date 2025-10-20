#pragma once

namespace http {

    template<typename T>
    class features {
        const T& _features;
        public:
            features(const T& f) : _features(f) {}
            inline bool has(const T& mask) const {
                return (_features & mask) == mask;
            }
    };

}