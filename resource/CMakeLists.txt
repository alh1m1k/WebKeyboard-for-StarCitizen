resource(lib.js      TEXT)
resource(widget.js   TEXT)
resource(overlay.js  TEXT)
resource(favicon.ico BINARY)
resource(index.html  TEXT)
resource(index.css   TEXT)
resource(index.js    TEXT)
resource(symbols.svg TEXT)

option(EMBED_CAPTIVE "Include captive portal page" ON)
set(CAPTIVE_PORTAL_BACK_URL "wkb.local" CACHE STRING "Captive portal back button url")

MESSAGE("EMBED_CAPTIVE=${EMBED_CAPTIVE}")

if (EMBED_CAPTIVE OR FALSE)
    set(_BUILD_STAGE TRUE)
else ()
    set(_BUILD_STAGE FALSE)
endif ()
if (_BUILD_STAGE)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/build")

    message(STATUS "Building resources")

    if (EMBED_CAPTIVE)
        configure_file("${CMAKE_CURRENT_LIST_DIR}/captive.html" "${CMAKE_CURRENT_LIST_DIR}/build/captive.html" @ONLY)
        resource("build/captive.html" TEXT)
    endif ()

    file(REMOVE_RECURSE "${CMAKE_CURRENT_LIST_DIR}/build")
endif ()




