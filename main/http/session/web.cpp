#include "web.h"

namespace http::session {
    web::web(const std::string& id) noexcept : _id(id)  {};
    const std::string web::id() noexcept { return _id; };
}