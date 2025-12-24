# WebKeyboard for StarCitizen v0.3
This is the first major release after initial, focused primarily on technical fixes, server and security features.

<details>
 <summary>Subsystems version on release</summary>
 PROTOCOL_VERSION 	 0.1 <br>
 STORAGE_VERSION  	 0.4 <br>
 SERVER_VERSION 	 0.3 <br>
 CLIENT_VERSION 	 0.2 <br>
 INCREMENTAL_VERSION 3   <br>
</details>

> [!WARNING]
> Changed default behavior:<br>
> config.h before v0.3 WIFI_AP_AUTH was explicit set to WIFI_AUTH_WPA_WPA2_PSK, now by default WIFI_AP_AUTH is unset
> and default value is such case: WIFI_AUTH_WPA2_WPA3_PSK
> This change may result in older devices not being able to connect or see the hotspot. In such case, 
> it is worth rollback the value and making a custom build.

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
This is a step ahead necessary for further improvements of the application.
Server now able to identify client acrose multiple protocols and connections, and manage
connection live time.

#### Cookie management.
For session needs

#### Page cache management.
Implemented etag page cache based on compile time file checksum.
Controlled by `HTTP_CACHE_USE_ETAG` config option (default: true).
Also require `-RESOURCE_CHECKSUM=ON` compile flag is set (default: enabled).
> [!Note]
> Since page cache rely on compile time checksum
> if `-DRESOURCE_COMPRESSION=ON` page etag value will change every build due to the reasons described in the section 
> "Content compression"

#### Content compression.
Implemented compile time web content compression (gzip). 
Enabled with `-DRESOURCE_COMPRESSION=ON` compile flag (default: enabled).
Runtime decompression not implemented yet and there is a chance that it will never be implemented, as all major browser 
in minimum req spec. a support compressed content on flight. This one reduce flash req for this build on ~10% (of total app partition).
> [!Note]
> For some reason, probably due timestamp included in to header, CMake file(ARCHIVE_CREATE) produce different output (checksum)
> even if source file not change between compression. Also, mtime setting of file(ARCHIVE_CREATE) is not work for at least gzip format.

#### DNS and captive portal
DNS server based on captive dns server example. 
Controlled by `WIFI_AP_DNS` conf. opt. (default: true), 
Captive portal controlled by `WIFI_AP_DNS_CAPTIVE` conf. opt. (default: false).
This is single record dns server where dns.record = app.host or dns.record = "*".
For captive portal conf `HTTP_USE_HTTPS = false` must be, and compilation flag `-DEMBED_CAPTIVE=ON` must be set.
Captive portal is special page whe all wrong host inquiry go.

> [!Note]
> Captive portal logs can be extensive. It may contain reference to internal error
> `[error] fixme -> runtime decompression` depending of what happening on your system
> as captured host request may not accept compression

> [!Note]
> Not any inquiry can be captured

Additional compile flag exist `-DCAPTIVE_PORTAL_BACK_URL="wkb.local"` (default: "wkb.local")
setting "go back" button href on captive portal

#### Dynamic ip
With dns enabled it is now possible to use dynamic ip for server.
controlled by `WIFI_AP_DHCP_USE_STATIC_IP` (default: true)
Use with caution, client dns cache still exist.

#### Optional HTTPS server.
Secure ssl http server. <br>
Enabled with opt: `HTTP_USE_HTTPS = true` (default: false). <br>
The application must be compiled with flags `-DEMBED_CERT=ON` <br>
The cert's must be placed inside cert directory.
- cert.pem - Server certificate in pem format
- privkey.pem - Private key in pem format
- cacert.pem - (optional, detect presence) CA certificate ((CA used to sign clients, or client cert itself)

> [!NOTE]
> The certificate file descriptions are taken from the documentation
> https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_https_server.html as is.
> And at the moment, I don't understand what espressif meant. Usually, when the API provides separate cert sections,
> "cert" contains the server certificate, "cacert" contains the concat of trust chain certificates.
> That not work anymore. <br>
> The HTTPS server in release 0.3 uses a two-file configuration where cert.pem contains the entire chain of trust from
> the server certificate to (but not including) the root of trust. privkey.pem contains the private key.

About https sockets: Every ssl socket consume ~40kb of ram.
esp32s3 have 500kb of ram, half of them already allocated on app startup. so server will operate
in extremely strict condition. main hint there is to use default config as possible.

sdkconfig(menuconfig):
`CONFIG_LWIP_MAX_SOCKETS=10` (by default 32, but it will keep esp out of resources)

config.h
`SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER` true (default: true)
`SOCKET_RECYCLE_USE_LRU_COUNTER` true (default: true)

https::server and main.cpp have build-in static assert that may help, by preventing app with invalid configuration from build
as it probably won't work anyway.

> [!Note]
> Server configured to be secure or not secure at compile time. Secured server does not support unsecured connection 
> at port 80 and vice versa. If connection error appear, check protocol with what you're connecting to server `http` or `https`
> it can be implicitly changed by the browser

#### Ability to build with optimization flags (-0s, -02).
Side effect of server refactoring. controlled by menuconfig.

#### Ability to build with default partition table and stock esp32s3 module with 2mb flash.
With compression enabled and https disabled application binary can fit inside stock 2mb "large single partition (no OTA)".
This one not change min requirement for 4mb flash, as application may continue to grow. Controlled by menuconfig.

#### Client connection flow refactoring.
Refactoring of one of the oldest client component in order to make connection establishing robust and fast.

#### Code cleanup + drop 3d party library
Refactoring + dropped mdns library and http_parser library.
Added dns_server library.

#### Fixed socket pool depletion problem for some configuration
For http with default config it seems to be fixed.
For https... due to the larger socket size, there is not enough memory on the esp32s3 to accommodate a sufficient number of sockets to handle the worst-case scenario.
But real live test's show that with `SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER` and `SOCKET_RECYCLE_USE_LRU_COUNTER`
it able to handle concurrent connection of 2 devices.

#### Bugfixes
fixed 'undefined' client name appeared in some cases
fixed  server early startup crash

#### Bump idf and dep version
now minimum require (and recommended) 
- idf version 5.5.1 (up from 5.1.4)
- espressif/tinyusb: 0.19.0~2 (up from 0.17.0~2)

<br>

> [!WARNING]
> For domain users, browser security policy restrict localStorage (app setting storage) as domain specific, 
> ie `192.168.5.1` and `wkb.local` is different domain and have different data in. Moving from `192.168.5.1` to `wkb.local` 
> or vise versa does not transfer your setting. There is no easy solution for now.

> [!NOTE]
> In some cases server not able to detect client that "go away" see: config.h SOCKET_KEEP_ALIVE_TIMEOUT for more information.