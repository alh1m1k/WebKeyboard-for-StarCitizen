# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS 				
    					resultStream.cpp http/uri.cpp http/network.cpp http/headers.cpp http/cookies.cpp http/request.cpp http/response.cpp http/socket/handler.cpp
						http/socket/socket.cpp http/socket/asyncSocket.cpp
    					http/resource/memory/file.cpp http/server.cpp http/action.cpp http/cookie.cpp

						crypto/hash/sha256Engine.cpp
    					hid/usbDevice.cpp
    					hid/usbModuleAdapter.cpp
    					hid/keyboard.cpp
    					hid/joystick.cpp
    					parsers/keyboard.cpp
    					wifi/wifi.cpp
						sessionManager.cpp
    					main.cpp  
    					
    					
    					# list the source files of this component
    INCLUDE_DIRS        
    					. ./exception
    					./http ./http/exception ./http/resource ./http/session ./http/resource/memory ./http/lib/httpparser
    					./http/socket
						./syncing
						./crypto
    					./dns
    					./hid
    					./wifi
    					./parsers
    					 # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            freertos nvs_flash esp_event esp_netif esp_wifi esp_https_server esp_timer tinyusb mdns driver bootloader_support mbedtls	# optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-terminate)
