#pragma once

namespace http::session {

    enum class traits : uint32_t {
        BASE_API = 0,
        SOCKET_COUNT,
        WEBSOCKET,
        SESSION,
        TEST,

        TOTAL
    };

}