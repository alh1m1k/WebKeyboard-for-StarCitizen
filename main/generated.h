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

#ifdef WIFI_AP_DNS
#if WIFI_MODE != WIFI_MODE_AP
#undef  WIFI_MODE;
#define WIFI_MODE false
#endif
#else
#define WIFI_AP_DNS true
#endif

#ifndef WIFI_AP_DHCP_STATIC_IP
#define WIFI_AP_DHCP_STATIC_IP false
#endif

#ifndef WIFI_AP_DHCP_STATIC_IP
#define WIFI_AP_DHCP_STATIC_IP DEFAULT_WIFI_AP_DHCP_STATIC_IP
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

#ifndef WIFI_AP_DNS_CAPTIVE
#define WIFI_AP_DNS_CAPTIVE true
#endif

#if WIFI_AP_DNS_CAPTIVE
#define APP_DNS_DOMAIN "*"
#else
#define APP_DNS_DOMAIN WIFI_AP_DNS_DOMAIN
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

#ifndef SESSION_TIMEOUT
SESSION_TIMEOUT 60*20
#endif

#ifndef SID_COOKIE_TTL
SID_COOKIE_TTL 3600*24
#endif

#ifndef HTTP_CACHE_USE_ETAG
HTTP_CACHE_USE_ETAG true
#endif

#ifndef SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER
#define SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER true
#endif

#ifndef SOCKET_RECYCLE_USE_LRU_COUNTER
#define SOCKET_RECYCLE_USE_LRU_COUNTER true
#endif





