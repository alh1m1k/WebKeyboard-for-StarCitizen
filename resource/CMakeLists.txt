resource(${WEB_ROOT_SRC}/lib.js      TEXT)
resource(${WEB_ROOT_SRC}/widget.js   TEXT)
resource(${WEB_ROOT_SRC}/overlay.js  TEXT)
resource(${WEB_ROOT_SRC}/favicon.ico BINARY)
resource(${WEB_ROOT_SRC}/index.html  TEXT)
resource(${WEB_ROOT_SRC}/index.css   TEXT)
resource(${WEB_ROOT_SRC}/index.js    TEXT)
resource(${WEB_ROOT_SRC}/symbols.svg TEXT)

option(EMBED_CAPTIVE "Include captive portal page" OFF)
set(CAPTIVE_PORTAL_BACK_URL "wkb.local" CACHE STRING "Captive portal back button url")

MESSAGE("EMBED_CAPTIVE=${EMBED_CAPTIVE}")

if (EMBED_CAPTIVE)
    configure_file(${WEB_ROOT_SRC}/captive.html ${WEB_ROOT_BIN}/captive.html @ONLY)
    resource(${WEB_ROOT_BIN}/captive.html TEXT)
endif ()




