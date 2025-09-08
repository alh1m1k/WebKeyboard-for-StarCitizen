#pragma once

#include "config.h"


#ifndef WIFI_MODE 
#define WIFI_MODE WIFI_MODE_AP
#endif

#ifndef WIFI_MODE 
#define WIFI_SSID "test"
#endif

#ifndef WIFI_PWD 
#define WIFI_PWD  "test"
#endif

#if defined(WIFI_AP_DHCP_STATIC_IP) && !defined(WIFI_AP_DHCP_STATIC_MASK)
#define WIFI_AP_DHCP_STATIC_MASK 225.255.255.0
#endif

#if defined(WIFI_AP_DHCP_STATIC_IP) && !defined(WIFI_AP_DHCP_STATIC_GATEWAY)
#define WIFI_AP_DHCP_STATIC_GATEWAY WIFI_AP_DHCP_STATIC_IP
#endif

#if defined(DEVICE_KB_VENDORID) || defined(DEVICE_KB_PRODUCTID) 

#define DEVICE_KB_DEVICE_DESC_OVERRIDED

#endif

#if defined(DEVICE_KB_MANUFACTURER) || defined(DEVICE_KB_PRODUCT) || defined(DEVICE_KB_SERIALS) || defined(DEVICE_KB_HID)

#define DEVICE_KB_DEVICE_STR_DESC_OVERRIDED

#endif

#ifndef SESSION_MAX_CLIENT_COUNT
#define SESSION_MAX_CLIENT_COUNT 10
#endif

#ifdef SESSION_ENABLE
#define _SESSION_ENABLE
#endif

#if defined(_SESSION_ENABLE) && defined(NOTIFICATION_ENABLE)
#define _NOTIFICATION_ENABLE
#endif

#ifndef WIFI_MIN_SSID_LEN
#define WIFI_MIN_SSID_LEN 3
#endif

#ifndef WIFI_MAX_SSID_LEN
#define WIFI_MAX_SSID_LEN 31
#endif

#ifndef WIFI_MIN_PWD_LEN
#define WIFI_MIN_PWD_LEN 7
#endif

#ifndef WIFI_MAX_PWD_LEN
#define WIFI_MAX_PWD_LEN 63
#endif

///LOGS

#ifndef LOG_JOYSTICK
#define LOG_JOYSTICK 	false
#endif

#ifndef LOG_KEYBOARD
#define LOG_KEYBOARD 	false
#endif

#ifndef LOG_ENTROPY
#define LOG_ENTROPY 	false
#endif

// LOG_KEYBOARD + LOG_ENTROPY to log keyboard delays

#ifndef LOG_SOCKET
#define LOG_SOCKET 		false
#endif

#ifndef LOG_HTTP
#define LOG_HTTP 		false
#endif

#ifndef LOG_LED
#define LOG_LED 		false
#endif

#ifndef LOG_MESSAGES
#define LOG_MESSAGES 	false
#endif

#ifndef LOG_SCHEDULER
#define LOG_SCHEDULER 	false
#endif

#ifndef LOG_WIFI
#define LOG_WIFI 		false
#endif

#ifndef LOG_WIFI_AP_PWD
#define LOG_WIFI_AP_PWD	false
#endif




