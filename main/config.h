#pragma once

#include "define.h"

//WIFI CONFIGURATION STUFF
#define WIFI_MODE 							WIFI_MODE_AP
//it is not recommended to use WIFI_MODE_STA mode, use at own risk.
//#define WIFI_MODE 						WIFI_MODE_STA

//ap ssid minimum 7 symbol, if invalid host may be trapped
#define WIFI_SSID 							"WKB(change)"
//ap password minimum 7 symbol, if invalid host may be trapped
#define WIFI_PWD  							"spaceAdventure42"


/*
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

//WIFI_AUTH_WPA_WPA2_PSK is generally better but not any device can afford,
//also modes without graceful degradation (WIFI_AUTH_WPA3_PSK) even better see docs.
//different modes may have different requirements for ssid or password.
//So a ssid/password that is valid in one case may cause the host to enter trap state in another

//#define WIFI_AP_AUTH 						WIFI_AUTH_WPA2_WPA3_PSK
#define WIFI_AP_AUTH 						WIFI_AUTH_WPA_WPA2_PSK //WIFI_AUTH_WPA2_WPA3_PSK
//predefined ip of host and subnet
#define WIFI_AP_DHCP_USE_STATIC_IP 			true
#define WIFI_AP_DHCP_STATIC_IP 				"192.168.5.1"
#define WIFI_AP_DHCP_STATIC_MASK 			"225.255.255.0"

//wifi access point dns server, make sense only if WIFI_MODE == WIFI_MODE_AP
#define WIFI_AP_DNS 						true
#define WIFI_AP_DNS_DOMAIN					"wkb.local"
#define WIFI_AP_DNS_CAPTIVE 				true

//factory reset trigger btn pin and led pin (if not addressable)
//values included from define.h and board depended, change it there if other pin needed.
#define RESET_TRIGGER_BTN					BOARD_BOOT
#define RESET_LED_INDICATOR					BOARD_LEAD

//SOCKET MANAGEMENT
//Exact socket count controlled by menuconfig via CONFIG_LWIP_MAX_SOCKETS define
//httpd server socket poolSize = CONFIG_LWIP_MAX_SOCKETS - SYSTEM_SOCKET_RESERVED;
#define SYSTEM_SOCKET_RESERVED 5

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
#define SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER true

//use lwip "last recently used" counter for every socket. when socket pool depleted least recently socket will-be used
//to process incoming connection
//WARNING: this may close persistent ws connection in some condition.
#define SOCKET_RECYCLE_USE_LRU_COUNTER true


#define SID_COOKIE_TTL                      3600*24
#define SESSION_TIMEOUT                     60*30
#define SESSION_SID_REFRESH_INTERVAL        3600
#define SESSION_MAX_CLIENT_COUNT 			10


#define HTTP_CACHE_USE_ETAG true


//USB STUFF, this is override of default values,
//comment string to use provided via menuconfig (idf.py menuconfig)
#define DEVICE_KB_VENDORID		0xACF0
#define DEVICE_KB_PRODUCTID     0xA1DD

//#define DEVICE_KB_MANUFACTURER	"SpaceSaltyChicken"
#define DEVICE_KB_MANUFACTURER	"RZI"
//#define DEVICE_KB_PRODUCT		"WithCurrySauce"
#define DEVICE_KB_PRODUCT		"DSProbe"
//#define DEVICE_KB_SERIALS     if comment it be hash of chipid
#define DEVICE_KB_HID 			"WebKeyboard for StarCitizen"


#define ASSERT_IF_SOCKET_COUNT_LESS (SESSION_MAX_CLIENT_COUNT + 1 * 7)


//JTAG DEBUG
//#define DEBUG_ALLOW_JTAG_VIA_SUPPRESSED_CDC   //uncomment to allow any non trivial JTAG debug, but this will entirely disable app USB stack
                                                //as CDC and JTAG are compete for resources
//use menuconfig Component config > ESP System Settings :: CONFIG_ESP_CONSOLE_UART = CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
//to redirect app logs to JTAG uart

//different subsytem log

//#define LOG_JOYSTICK 	true

//#define LOG_KEYBOARD 	true

//#define LOG_ENTROPY 	true

//#define LOG_SOCKET 	true

//#define LOG_HTTP 		true

//#define LOG_LED 		true

//#define LOG_MESSAGES true

//#define LOG_SCHEDULER true

//#define LOG_WIFI 		true

#define LOG_SESSION      false

#define LOG_SESSION_EVT  false

#define LOG_COOKIES      false

#define LOG_COOKIE_BUILD false

#define LOG_SERVER_GC    false

#define LOG_HTTPD_TASK_STACK false







