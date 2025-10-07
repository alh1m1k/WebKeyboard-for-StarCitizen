# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS 				
    					resultStream.cpp http/uri.cpp  http/headers.cpp http/cookies.cpp http/request.cpp http/response.cpp http/socket/handler.cpp
						http/session/web.cpp http/session/websocket.cpp http/session/memoryManager.cpp
						http/socket/socket.cpp http/socket/asyncSocket.cpp
    					http/resource/memory/file.cpp http/server.cpp http/route.cpp 
    					hid/usbDevice.cpp
    					hid/usbModuleAdapter.cpp
    					hid/keyboard.cpp
    					hid/joystick.cpp
    					parsers/keyboard.cpp
    					wifi/wifi.cpp  
    					main.cpp  
    					
    					
    					# list the source files of this component
    INCLUDE_DIRS        
    					. ./exception
    					./http ./http/exception ./http/resource ./http/session ./http/resource/memory ./http/lib/httpparser
    					./http/socket
    					./dns
    					./hid
    					./wifi
    					./parsers
    					 # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            freertos nvs_flash esp_event esp_netif esp_wifi esp_https_server esp_timer tinyusb mdns driver bootloader_support	# optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-terminate)
