# checklist

- code freeze           (complete)
- update config.h       (complete)
- update release.md     (complete)
- update README.md      (complete)
- testing
  - test http           (complete)
  - test https          (complete)
  - test page compression
  - test page cache
  - test dns
  - test captive portal
  - test session
    - test session invalidation
    - try to hijack it
    - test login/relogin
  - test fix: undefined client name
  - test ability to change client name 
  - test 2mb flash (complete)
  - test dynamic ip
  - test socket recycle
  - test optimization flags
  - create server ver define (complete)
  - bump client versions     (complete)
  - form reference app build
  - push, create release and tag it
  - media shitposting
  - fire and forget.

## Key features:
- More secure, fast and robust connection flow
- fast page load/reload
- significantly reduced flash memory requirements
- https server with user certs
- embedded dns server and app domain
- dynamic ip support
- captive portal

## Details
#### Session-based authorization has been implemented, replacing socket-based authentication.
this is a step ahead necessary for further improvements of the application.
server now able to identify client acrose multiple protocols and connections, and manage
connection live time.

#### Cookie management.
for session needs

#### Page cache management.
implemented etag page cache based on static content checksum

#### Content compression.
implemented static web content compression (deflate). 
enabled with `-DRESOURCE_COMPRESSION=ON` (default: enabled) 
runtime decompression not implemented yet and there is a chance that it will never be implemented.
as all major browser in minimum req spec. a support compressed content on flight.
this one reduce flash req for this build on ~10% (of total app partition)

#### DNS and captive portal
dns server based on captive dns server example.
this is one record dns server where dns.record = app.host or dns.record = "*".
for captive portal conf `HTTP_USE_HTTPS = false` must be,
and compilation flag enabled `EMBED_CAPTIVE=ON` must be set.
captive portal is special page whe all wrong host inquiry go.

> [!Note]
> Captive portal logs can be extensive. It may contain reference to internal error <br>
> `[error] fixme -> runtime decompression` depending of what happening on your system
> as captive host request may not accept compression

> [!Note]
> Not any inquiry can be captured

Additional compile flag exist `-DCAPTIVE_PORTAL_BACK_URL="wkb.local"` (default: "wkb.local")
setting "go back" button href on captive portal

#### Dynamic ip
with dns enabled it is now possible to use dynamic ip for server, but with caution as
client dns cache still exist

#### Optional HTTPS server.
secure ssl http server. <br>
enabled with opt: HTTP_USE_HTTPS = true (default: false). <br>
the application must be compiled with flags `-DEMBED_CERT=ON` or static assert appear. <br>
the cert's must be placed inside cert directory.
- cert.pem - server certificate in pem format
- privkey.pem - server private key in pem format
- cacert.pem - (optional, presence will be detected) server authority chain in pem format

about https sockets: every ssl socket consume ~40kb of ram.
esp32s3 have 500kb of ram, half of them already allocated on app startup. so server will operate
in extremely strict condition. main hint there is to use default config as possible.

sdkconfig(menuconfig):
CONFIG_LWIP_MAX_SOCKETS=10 (by default 32, but it will keep esp out of resources)

config.h
SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER true (default: true)
SOCKET_RECYCLE_USE_LRU_COUNTER true (default: true)

https::server and main.cpp have build-in static assert that may help, by preventing app with invalid configuration from build
as it probably won't work anyway.

> [!Note]
> Server configured to be secure or not secure at compile time. Secured server does not support unsecured connection 
> at po 80 and vice versa. If connection error appear, check protocol with what you're connecting to server `http` or `https`
> it can be implicitly changed by the browser

#### Ability to build with optimization flags (-0s, -02).
side effect of server refactoring

#### Ability to build with default partition table and stock esp32s3 module with 2mb flash.
with compression enabled and https disabled application binary can fit inside stock 2mb "large single partition (no OTA)".
this one not change min requirement for 4mb flash, as application may continue to grow.

#### Client connection flow refactoring.
refactoring of one of the oldest client component in order to make connection establishing robust and fast

#### Code cleanup + drop 3d party library
refactoring + dropped mdns library and http_parser library
added dns_server library

#### Fixed socket pool depletion problem for some configuration
for http with default config it seems to be fixed.
for https... due to the larger socket size, there is not enough memory on the esp32s3 to accommodate a sufficient number of sockets to handle the worst-case scenario.
but real live test's show that with `SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER` and `SOCKET_RECYCLE_USE_LRU_COUNTER`
it able to handle concurrent connection of 2 devices

#### Bump idf and dep version
now minimum require (and recommended) 
- idf version 5.5.1 (up from 5.1.4)
- espressif/tinyusb: 0.19.0~2 (up from 0.17.0~2)

#### Bugfixes
fixed 'undefined' client name appeared in some cases

<br>

> [!WARNING]
> For domain users, browser security policy restrict localStorage (app setting storage) as domain specific, 
> ie `192.168.5.1` and `wkb.local` is different domain and have different data in. Moving from `192.168.5.1` to `wkb.local` 
> or vise versa does not transfer your setting. There is no easy solution for now.