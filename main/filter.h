#pragma once

#include "generated.h"

#include "http/request.h"
#include "http/response.h"
#include "http/server.h"

#include "http/resource/file.h"
#include "http/resource/cache/noCache.h"

#include <functional>

result<http::codes> filterCaptiveAcceptHtmlHandler(http::request& req, http::response& resp, http::server& serv, const http::resource::file& file) {
	if constexpr (SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER) {
		resp.getHeaders().connection("close");
	}
	if (req.getHeaders().accept().contains(http::contentType2Symbols(http::contentType::HTML))) {
		auto impl = http::resource::cache::noCache();
		return impl(req, resp, serv);
	} else {
		return http::codes::BAD_REQUEST;
	}
}

