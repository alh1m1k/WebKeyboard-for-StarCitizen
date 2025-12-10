#pragma once

#include "cacheControl.h"

namespace http::resource::cache {

	class noCache : cacheControl {
		public:
			using cacheControl::operator();
			constexpr noCache() noexcept : cacheControl("no-cache") {};
	};

}