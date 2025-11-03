#pragma once

#include "generated.h"

#include <memory>
#include <iostream>

#include "esp_netif.h"

namespace http {

    class network {

        typedef struct { esp_netif_t* netif; uint8_t family; } netif_info_type;

        const int    _handler;
        const bool   _remote;
        mutable std::unique_ptr<uint8_t[]> _buffer = nullptr;

        netif_info_type queryNetIf(int handler)   const;

        public:

            typedef struct {
                uint32_t addr;
            } ipv4_address_type;

            typedef struct {
                union {
                    uint32_t u32_addr[4];
                    uint8_t  u8_addr[16];
                } un;
            } ipv6_address_type;

            typedef struct {
                uint8_t addr[6];
            } mac_address_type;

            explicit network(int sock, bool remote = true) noexcept; //queue net from sock

            virtual ~network() = default;

            uint8_t             family()  const;
            int8_t              version() const;
            ipv4_address_type   ipv4()    const;
            ipv6_address_type   ipv6()    const;
            uint16_t            port()    const;
            mac_address_type    mac()     const;
    };

    extern const network::ipv4_address_type invalidIpv4;
    extern const network::ipv6_address_type invalidIpv6;
    extern const network::mac_address_type  invalidMac;
}

std::ostream& operator<<(std::ostream& stream, const http::network::ipv4_address_type& ip4addr);
std::ostream& operator<<(std::ostream& stream, const http::network::ipv6_address_type& ip6addr);
std::ostream& operator<<(std::ostream& stream, const http::network::mac_address_type&  macaddr);