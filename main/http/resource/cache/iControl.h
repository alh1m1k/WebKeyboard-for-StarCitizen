#pragma once

#include "file.h"

#include "result.h"

namespace http {
	class request;
	class response;
	class server;
}

namespace http::resource::cache {

	class iControl {
		public:
			virtual result<codes> operator()(request& req, response& resp, server& serv) = 0;
	};

}