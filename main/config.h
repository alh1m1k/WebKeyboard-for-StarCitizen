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
#define WIFI_AP_DHCP_STATIC_IP 				"192.168.5.1"
#define WIFI_AP_DHCP_STATIC_MASK 			"255.255.255.0"

//this is not real DNS. mDns, only fresh system support's it.
//I also haven't heard of certificates issued for such a domain name
#define WIFI_AP_DNS							//using of dns may degrade perfomance
											//not work on old android
											//not work on windows earlier w10
#define WIFI_AP_DNS_SERVICE_NAME 			"wkb"
#define WIFI_AP_DNS_SERVICE_DESCRIPTION 	"StarCitizen WebKeyboard"

//factory reset trigger btn pin and led pin (if not addressable)
//values included from define.h and board depended, change it there if other pin needed
#define RESET_TRIGGER_BTN					BOARD_BOOT
#define RESET_LED_INDICATOR					BOARD_LEAD

//do not change
#define SESSION_ENABLE
//maximum number of clients, no point in increasing.
//The network stack probably won't be able to handle even that amount.
#define SESSION_MAX_CLIENT_COUNT 			10

//do not change
#define NOTIFICATION_ENABLE



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

#define LOG_MESSAGES    false

//#define LOG_SCHEDULER true

//#define LOG_WIFI 		true

#define LOG_SESSION        false

#define LOG_COOKIES     false

#define LOG_COOKIE_BUILD false






