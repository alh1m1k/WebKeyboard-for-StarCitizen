#pragma once

#include "define.h"

//WIFI CONFIGURATION STUFF

//WIFI_MODE_AP is default wifi working mode
//#define WIFI_MODE 						WIFI_MODE_AP
//it is *NOT* recommended to use WIFI_MODE_STA mode, use at own risk.
//#define WIFI_MODE 						WIFI_MODE_STA

//ap ssid minimum 7 symbol, if invalid host may be trapped
#define WIFI_SSID 							"WKB(change)"

//ap password minimum 7 symbol, if invalid host may be trapped
#define WIFI_PWD  							"spaceAdventure42"


//Auth mode,
//WIFI_AUTH_WPA2_WPA3_PSK is generally better but not any device can afford,
//also modes without graceful degradation (WIFI_AUTH_WPA3_PSK) even better see docs.
//different modes may have different requirements for ssid or password.
//So a ssid/password that is valid in one case may cause the host to enter trap state in another

/*
 * 	List of valid WIFI_AP_AUTH values
	 WIFI_AUTH_WPA2_PSK
	 WIFI_AUTH_WPA_WPA2_PSK
	 WIFI_AUTH_ENTERPRISE
	 WIFI_AUTH_WPA2_ENTERPRISE
	 WIFI_AUTH_WPA3_PSK
	 WIFI_AUTH_WPA2_WPA3_PSK
	 WIFI_AUTH_WAPI_PSK
	 WIFI_AUTH_WPA3_ENT_192
	 WIFI_AUTH_WPA3_EXT_PSK
	 WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE
	 WIFI_AUTH_WPA3_ENTERPRISE
	 WIFI_AUTH_WPA2_WPA3_ENTERPRISE
	 WIFI_AUTH_WPA_ENTERPRISE
 */

//WIFI_AUTH_WPA2_WPA3_PSK is for new device
//#define WIFI_AP_AUTH 						WIFI_AUTH_WPA2_WPA3_PSK
//WIFI_AUTH_WPA_WPA2_PSK is for older device
//#define WIFI_AP_AUTH 						WIFI_AUTH_WPA_WPA2_PSK




//predefined ip of host and subnet
//WIFI_AP_DHCP_USE_STATIC_IP == true is not default but set in order to be compatible with
//prev releases
#define WIFI_AP_DHCP_USE_STATIC_IP 			true
//#define WIFI_AP_DHCP_STATIC_IP 			"192.168.5.1"
//#define WIFI_AP_DHCP_STATIC_MASK 			"255.255.255.0"

//wifi access point dns server, make sense only if WIFI_MODE == WIFI_MODE_AP
//#define WIFI_AP_DNS 						true //if WIFI_MODE == WIFI_MODE_AP
//#define WIFI_AP_DNS_DOMAIN				"wkb.local"
//force dns to capture all dns query and direct client to "this" server captive portal
//make sense if (HTTP_USE_HTTPS == false) and  -DEMBED_CAPTIVE=ON compile flag is set
//#define WIFI_AP_DNS_CAPTIVE 				false

//factory reset trigger btn pin and led pin (if not addressable)
//values included from define.h and board depended, change it there if other pin needed.
#define RESET_TRIGGER_BTN					BOARD_BOOT
#define RESET_LED_INDICATOR					BOARD_LEAD

//SOCKET MANAGEMENT
//Exact socket count controlled by menuconfig via CONFIG_LWIP_MAX_SOCKETS define
//httpd server socket poolSize = CONFIG_LWIP_MAX_SOCKETS - SYSTEM_SOCKET_RESERVED;
//WARNING: https socket is extremely ram hungry(40kb ram per socket), server
//maintain operational only within small configuration opt. window
//IF you chose to use https server better use it with default configuration,
//also check for right version of ASSERT_IF_SOCKET_COUNT_LESS

//reserve socket(from system pool) for other needs (other consume webserver)
#define SYSTEM_SOCKET_RESERVED 3

//when socket pool became empty system will stall ~30s until some of the socket became available.
//SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER reduce chance of such event by closing every non essential connection
//SOCKET_RECYCLE_USE_LRU_COUNTER allow to close some old connection to free socket if such event occur
//peek socket consumption = SESSION_MAX_CLIENT_COUNT + (number of pending files * number of concurrent clients)
//in general, SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER work better when most of socket pool already utilized, but increase
//pressure on net layer due to continues open/close op.
//SOCKET_RECYCLE_USE_LRU_COUNTER work better with large pool of sockets, but it can ruin app if socket pool not big enough to handle persistent connections + new one
//as it began to drop persistent connection and force client to reconnect which will lead to the need to open new connections.

//every static file request will processed with "connection: close" header applied.
//instructing the client to close the connection immediately after receiving the file, without waiting for the socket timeout
//freeing up the socket for subsequent requests
//#define SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER true

//use lwip "last recently used" counter for every socket. when socket pool depleted least recently socket will-be used
//to process incoming connection
//WARNING: this may close persistent ws connection in some condition.
//#define SOCKET_RECYCLE_USE_LRU_COUNTER true


//SESSION AND COOKIE MANAGEMENT

//cookie time to live in seconds
//#define SID_COOKIE_TTL                    3600*24

//session inactivity timeout(before it close) in seconds
//#define SESSION_TIMEOUT                   60*30

//timeout before inactive socket will be closed, 0 - newer
//there is tradeoff: without this feature server unable to detect when client go away
//without close connection, but on mobile device this feature can be annoying
//as sleep mode prevent keep_alive packet to be sent, forcing the server to break
//the connection and send notification for every device entering sleep
//on every device that awake.
//this default value is subject to change
//#define SOCKET_KEEP_ALIVE_TIMEOUT			0

//interval in seconds between ssid was regenerated and cookies was updated
//#define SESSION_SID_REFRESH_INTERVAL      3600

//maximum active sessions in system, every session consume at least one socket via
//persistent connection
//#define SESSION_MAX_CLIENT_COUNT 			10


//HTTP(s) modes

//use secure connection, make sense only if -DEMBED_CERT=ON compile flag is set.
//2 or 3 file certificates must be provided in cert directory see ../cert/readme.md
//WARNING: use https version of ASSERT_IF_SOCKET_COUNT_LESS for https
//WARNING: idf.py menuconfig set CONFIG_LWIP_MAX_SOCKETS=10 for https or system will-be out of resources
//#define HTTP_USE_HTTPS false

//enable ETAG cache, resource will-be cached using they compile time checksum
//that require -RESOURCE_CHECKSUM=ON compile flag is set
//preventing expensive content steaming and improving load time.
//there is no real reason to disable etag except debugging.
//#define HTTP_CACHE_USE_ETAG true

//maximum response headers per request
//#define HTTP_MAX_RESPONSE_HEADERS 10

//server port for incoming connections
//#define HTTP_PORT 		80
//#define HTTP_HTTPS_PORT 	443


//USB STUFF

//this is override of default values,
//comment string to use provided via menuconfig (idf.py menuconfig)
#define DEVICE_KB_VENDORID		0x09da
#define DEVICE_KB_PRODUCTID     0xF6CC

#define DEVICE_KB_MANUFACTURER	"RZI"
#define DEVICE_KB_PRODUCT		"DSProbe"
#define DEVICE_KB_HID 			"WebKeyboard for StarCitizen"



//http version, it kill compilation is configuration invalid
#define HTTP_ASSERT_IF_SOCKET_COUNT_LESS (SESSION_MAX_CLIENT_COUNT + 1 * 7)

//https version, it kill compilation is configuration invalid
#define HTTPS_ASSERT_IF_SOCKET_COUNT_LESS 6

//kill insecure https server
#define ASSERT_IF_INSECURE_HTTPS 	true


//JTAG DEBUG, for developer

//use menuconfig Component config > ESP System Settings :: CONFIG_ESP_CONSOLE_UART = CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
//to redirect app logs to JTAG uart

//uncomment to allow any non trivial JTAG debug, but this will entirely disable app USB stack
//as CDC and JTAG are compete for resources
//#define DEBUG_ALLOW_JTAG_VIA_SUPPRESSED_CDC


//different subsytem log

//#define LOG_JOYSTICK 		false

//#define LOG_KEYBOARD 		false

//#define LOG_ENTROPY 		false

//#define LOG_SOCKET 		false

//#define LOG_HTTP 			false

//#define LOG_LED 			false

//#define LOG_MESSAGES 		false

//#define LOG_SCHEDULER 	false

//#define LOG_WIFI 			false

//#define LOG_SESSION      	false

//#define LOG_SESSION_EVT  	false

//#define LOG_HEADERS false

//#define LOG_COOKIES      	false

//#define LOG_COOKIE_BUILD 	false

//#define LOG_SERVER_GC    	false

//#define LOG_HTTPD_TASK_STACK 	false

//#define LOG_HTTPD_HEAP 		false

//#define LOG_SERVER_SOCK_RECYCLE false







