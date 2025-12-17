#pragma once

#include "config.h"


#ifndef WIFI_MODE 
#define WIFI_MODE WIFI_MODE_AP
#endif

#ifndef WIFI_AP_AUTH
#define WIFI_AP_AUTH WIFI_AUTH_WPA2_WPA3_PSK
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

#ifndef WIFI_AP_DNS
#if WIFI_MODE == WIFI_MODE_AP
#define WIFI_AP_DNS true
#else
#define WIFI_AP_DNS false
#endif
#endif

#ifndef WIFI_AP_DHCP_USE_STATIC_IP
#define WIFI_AP_DHCP_USE_STATIC_IP false
#endif

#ifndef WIFI_AP_DHCP_STATIC_IP
#define WIFI_AP_DHCP_STATIC_IP DEFAULT_WIFI_AP_DHCP_STATIC_IP
#endif

#if defined(WIFI_AP_DHCP_STATIC_IP) && !defined(WIFI_AP_DHCP_STATIC_GATEWAY)
#define WIFI_AP_DHCP_STATIC_GATEWAY WIFI_AP_DHCP_STATIC_IP
#endif

#ifndef WIFI_AP_DHCP_STATIC_MASK
#define WIFI_AP_DHCP_STATIC_MASK DEFAULT_WIFI_AP_DHCP_STATIC_MASK
#endif

#ifndef WIFI_AP_DNS_DOMAIN
#define WIFI_AP_DNS_DOMAIN  "wkb.local"
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

#ifndef LOG_SESSION
#define LOG_SESSION	false
#endif

#ifndef LOG_SESSION_EVT
#define LOG_SESSION_EVT	false
#endif

#ifndef LOG_COOKIES
#define LOG_COOKIES	false
#endif

#ifndef LOG_COOKIE_BUILD
#define LOG_COOKIE_BUILD	false
#endif

#ifndef LOG_SERVER_GC
#define LOG_SERVER_GC	false
#endif

#ifndef LOG_HTTPD_TASK_STACK
#define LOG_HTTPD_TASK_STACK	false
#endif

#ifndef LOG_HTTPD_HEAP
#define LOG_HTTPD_HEAP	false
#endif

#ifndef LOG_SERVER_SOCK_RECYCLE
#define LOG_SERVER_SOCK_RECYCLE	false
#endif

#ifndef SESSION_TIMEOUT
#define SESSION_TIMEOUT (60*20)
#endif

#ifndef SID_COOKIE_TTL
#define SID_COOKIE_TTL (3600*24)
#endif

#ifndef SESSION_SID_REFRESH_INTERVAL
#define SESSION_SID_REFRESH_INTERVAL 3600
#endif

#ifndef HTTP_CACHE_USE_ETAG
#define HTTP_CACHE_USE_ETAG true
#endif

#ifndef HTTP_USE_HTTPS
#define HTTP_USE_HTTPS false
#endif

#ifndef HTTP_MAX_RESPONSE_HEADERS
#define HTTP_MAX_RESPONSE_HEADERS 10
#endif

#ifndef HTTP_PORT
#define	HTTP_PORT 80
#endif

#ifndef HTTP_HTTPS_PORT
#define	HTTP_HTTPS_PORT 443
#endif

#ifndef SYSTEM_SOCKET_RESERVED
#define SYSTEM_SOCKET_RESERVED 3
#endif

#ifndef SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER
#define SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER true
#endif

#ifndef SOCKET_RECYCLE_USE_LRU_COUNTER
#define SOCKET_RECYCLE_USE_LRU_COUNTER true
#endif

#ifndef ASSERT_IF_INSECURE_HTTPS
#define ASSERT_IF_INSECURE_HTTPS true
#endif

#ifndef WIFI_AP_DNS_CAPTIVE
#define WIFI_AP_DNS_CAPTIVE false
#endif

#if WIFI_AP_DNS_CAPTIVE
#define APP_DNS_DOMAIN "*"
#else
#define APP_DNS_DOMAIN WIFI_AP_DNS_DOMAIN
#endif

#ifndef ASSERT_IF_SOCKET_COUNT_LESS
#if HTTP_USE_HTTPS && defined(HTTPS_ASSERT_IF_SOCKET_COUNT_LESS)
#define ASSERT_IF_SOCKET_COUNT_LESS HTTPS_ASSERT_IF_SOCKET_COUNT_LESS
#elif defined(HTTP_ASSERT_IF_SOCKET_COUNT_LESS)
#define ASSERT_IF_SOCKET_COUNT_LESS HTTP_ASSERT_IF_SOCKET_COUNT_LESS
#endif
#endif


