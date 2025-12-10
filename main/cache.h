#pragma once

#include "generated.h"

#include "http/request.h"
#include "http/response.h"
#include "http/server.h"

#include "http/resource/file.h"
#include "http/resource/cache/eTag.h"
#include "http/resource/cache/noCache.h"
#include "http/resource/cache/dymmy.h"

#include <functional>

result<http::codes> noCacheHandler(http::request& req, http::response& resp, http::server& serv, const http::resource::file& file) {
	if constexpr (SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER) {
		resp.getHeaders().connection("close");
	}
	auto impl = http::resource::cache::noCache();
	return impl(req, resp, serv);
}

result<http::codes> eTagHandler(http::request& req, http::response& resp, http::server& serv, const http::resource::file& file) {
	if constexpr (SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER) {
		resp.getHeaders().connection("close");
	}
	auto impl = http::resource::cache::eTag(file.checksum);
	return impl(req, resp, serv);
}

result<http::codes> dummyHandler(http::request& req, http::response& resp, http::server& serv, const http::resource::file& file) {
	if constexpr (SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER) {
		resp.getHeaders().connection("close");
	}
	auto impl = http::resource::cache::dummy();
	return impl(req, resp, serv);
}

template<class TCache>
http::resource::file::callback_type adapter(TCache&& cache, std::function<void(TCache& cache, const http::resource::file& file)> setup = nullptr) {
	return [cache = std::forward<TCache>(cache), &setup](http::request& req, http::response& resp, http::server& serv, const http::resource::file& file) -> result<http::codes> {
		if constexpr (SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER) {
			resp.getHeaders().connection("close");
		}
		if (setup) {
			configurate(cache, file);
		}
		return cache(req, resp, serv);
	};
}