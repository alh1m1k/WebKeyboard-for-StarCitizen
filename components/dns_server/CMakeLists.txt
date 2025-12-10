idf_component_register(SRCS dns_server.c
                       INCLUDE_DIRS include
                       PRIV_REQUIRES esp_netif)

target_compile_definitions("${COMPONENT_LIB}"  PUBLIC "-DLOG_LOCAL_LEVEL=ESP_LOG_WARN")
