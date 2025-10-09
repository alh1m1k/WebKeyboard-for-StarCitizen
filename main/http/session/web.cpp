#include "web.h"

namespace http::session {
    web::web(const std::string& id) : _sid(id)  {};
    web::web(std::string&& id)      : _sid(id)  {};
    const std::string& web::sid()   { return _sid; };
}