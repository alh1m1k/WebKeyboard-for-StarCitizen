#pragma once

namespace http::resource {
	enum class map: int {
		MEMORY 		= 1,
		FILE 		= 2,
		directory 	= 3,
	};
}