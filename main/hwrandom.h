#pragma once

#include <limits>
#include <sys/_stdint.h>
#include "esp_random.h"

#include "unsupported.h"

template <typename R>
class hwrandom {
		
	public:
	
		typedef R result_type;

		hwrandom() {}
		
		virtual ~hwrandom() {}
		
		result_type min() {
			return std::numeric_limits<result_type>::min();
		}
		
		result_type max() {
			return std::numeric_limits<result_type>::max();
		}
		
		void seed(result_type seed) {
			throw unsupported("seed for random generator");
		}
		
		result_type operator()() {
			result_type buffer;
			esp_fill_random(&buffer, sizeof(result_type));
			return buffer;
		}

        using trait_fill_buffer = std::true_type;
		void fill(void* ptr, size_t size) {
			esp_fill_random(&ptr, size);
		}
		
		void discard(unsigned long long z) {
			throw unsupported("discard for random generator");
		}
};