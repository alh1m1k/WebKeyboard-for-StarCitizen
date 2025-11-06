#pragma once

namespace http::session {

    enum class traits : uint32_t {
        I_SESSION = 0,
        I_SOCKS_CNT_SESSION,
        I_WEB_SOCKET_SESSION,
        I_RENEW_SESSION,
        SESSION,
        I_TEST,

        I_MANAGER,
        MANAGER,

        TOTAL
    };


}