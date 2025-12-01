#pragma once

#include "file.h"
#include "result.h"
#include "cacheControl.h"

namespace http::resource::cache {

	class noCache : cacheControl {
		public:
			using cacheControl::operator();
			noCache() noexcept : cacheControl("no-cache")  {};
	};

}