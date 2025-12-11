#include "network.h"

#include "sdkconfig.h"
#include "lwip/sockets.h"
#include "lwip/etharp.h"
#include "lwip/netif.h"
#include "esp_netif_types.h"
#include "esp_netif_net_stack.h"
#include "lwip/nd6.h"
#include "lwip/ip6_addr.h"
#include "esp_netif_ip_addr.h"
#include "esp_mac.h"

#include "util.h"
#include "not_implemented.h"
#include "unsupported.h"

#define _POPULATE if (_buffer == nullptr) {\
                    _buffer = populate(_handler, _remote);\
                  }

#define ip6_addr_zero(addr1) (((addr1)->addr[0] == 0)  && \
                              ((addr1)->addr[1] == 0)  && \
                              ((addr1)->addr[2] == 0)  && \
                              ((addr1)->addr[3] == 0))

namespace http {

    const network::ipv4_address_type invalidIpv4 = {};
    const network::ipv6_address_type invalidIpv6 = {};
    const network::mac_address_type  invalidMac  = {};

#ifdef CONFIG_LWIP_IPV6
    static inline ip6_addr_t ip6(sockaddr_in6* in) {
        if (in->sin6_family == AF_INET) {
            return {{0x0000, 0x0000, 0xFFFF, in->sin6_addr.un.u32_addr[3]}, 0};
        } else if (in->sin6_family == AF_INET6) {
            return {{in->sin6_addr.un.u32_addr[0], in->sin6_addr.un.u32_addr[1], in->sin6_addr.un.u32_addr[2], in->sin6_addr.un.u32_addr[3]}, 0};
        }
        return {};
    }
#endif

#ifdef CONFIG_LWIP_IPV4
    static inline ip4_addr_t ip4(sockaddr_in* in) {
        if (in->sin_family == AF_INET) {
            return {((sockaddr_in*)in)->sin_addr.s_addr};
        }
#ifdef CONFIG_LWIP_IPV6
        else if (in->sin_family == AF_INET6) {
            if (ip6_addr_isipv4mappedipv6((const ip6_addr_t*)&((sockaddr_in6*)in)->sin6_addr)) {
                return {((sockaddr_in6*)in)->sin6_addr.un.u32_addr[3]};
            }
        }
#endif
        return {};
    }
#endif

    static std::unique_ptr<uint8_t[]> populate(int handler, bool remote) {
#ifdef CONFIG_LWIP_IPV6
        constexpr socklen_t struct_size = sizeof(struct sockaddr_in6);
#elifdef CONFIG_LWIP_IPV4
        constexpr socklen_t struct_size = sizeof(struct sockaddr_in);
#else
        constexpr socklen_t struct_size = sizeof(struct sockaddr);
#endif
        socklen_t size = struct_size;
        auto buffer = std::make_unique<uint8_t[]>(size);

        auto indirection = remote ? &getpeername : &getsockname;

        if (auto status = indirection(handler, (struct sockaddr*)buffer.get(), &size); status == 0) {
            if (struct_size < size) {
                //initially there was logic that was conservative
                //allocate minimum buffer size "sockaddr_in"
                //then reallocate to new updated _size, if buffer is not enough
                //but that was not work as _size not updated if it too small and error also not generated
                //that was probably getpeername bug
                //so just allocate "all the money"
                throw std::logic_error("unexpected buffer size");
            }
            return buffer;
        } else {
            throw std::logic_error("unable retrieve data from backend");
        }
    }

    struct exec_netif_packet {
        const std::unique_ptr<uint8_t[]>& data;
              esp_netif_t*                result;
              uint8_t                     family;
    };
    static esp_err_t execFindNetIf(void *ctx) {
        auto packet = static_cast<exec_netif_packet*>(ctx);

#ifdef CONFIG_LWIP_IPV6
        auto direct = (sockaddr_in6*)packet->data.get();
#elifdef CONFIG_LWIP_IPV4
        auto direct = (sockaddr_in*)packet->data.get();
#else
        auto direct = (sockaddr*)packet->data.get();
#endif


        for (auto netif = esp_netif_next_unsafe(nullptr); netif != nullptr; netif = esp_netif_next_unsafe(netif)) {
#ifdef CONFIG_LWIP_IPV4
            esp_netif_ip_info_t ipi = {};
            if (auto code = esp_netif_get_ip_info(netif, &ipi); code != ESP_OK) {
                return code;
            }
            if (ipi.ip.addr != 0 && ipi.ip.addr == ip4((sockaddr_in*)direct).addr) {
                packet->result = netif;
                packet->family = AF_INET;
                return ESP_OK;
            }
#endif
#ifdef CONFIG_LWIP_IPV6
            esp_ip6_addr_t ipis[LWIP_IPV6_NUM_ADDRESSES] = {};
            if (auto code = esp_netif_get_all_ip6(netif, ipis); code != ESP_OK) {
                return code;
            }
            auto candidate = ip6((sockaddr_in6*)direct);
            for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; ++i) {
                if (!ip6_addr_zero(&ipis[i]) && ip6_addr_zoneless_eq(&ipis[i], &candidate)) {
                    packet->result = netif;
                    packet->family = AF_INET6;
                    return ESP_OK;
                }
            }
#endif
        }

        return ESP_FAIL;
    }

    network::network(int sock, bool remote) noexcept : _handler(sock), _remote(remote) {}

    network::netif_info_type network::queryNetIf(int handler) const {
        if (_remote) {
            exec_netif_packet pack { .data = populate(handler, false), .result = nullptr, .family = 0 };
            if (esp_netif_tcpip_exec(execFindNetIf, &pack) == ESP_OK) {
                return {pack.result, pack.family};
            }
        } else {
            exec_netif_packet pack { .data   = _buffer, .result = nullptr, .family = 0 };
            if (esp_netif_tcpip_exec(execFindNetIf, &pack) == ESP_OK) {
                return {pack.result, pack.family};
            }
        }
        return {nullptr, 0};
    }

    uint8_t network::family() const {
        _POPULATE;
        return ((struct sockaddr*)_buffer.get())->sa_family;
    }

    int8_t network::version() const {
        _POPULATE;
        if (auto f = family(); f == AF_INET) {
            return  4;
        } else if (f == AF_INET6) {
            return  6;
        } else {
            return -1;
        }
    }

    network::ipv4_address_type network::ipv4() const {
#ifdef CONFIG_LWIP_IPV4
        _POPULATE;
        if (family() == AF_INET) {
			//accessor allow "safe" access through optimization level aka dereferencing type-punned pointer
			void* accessor = &((struct sockaddr_in*)_buffer.get())->sin_addr;
            return *reinterpret_cast<network::ipv4_address_type*>(accessor);
        }
#endif
        return invalidIpv4;
    }

    network::ipv6_address_type network::ipv6() const {
#ifdef CONFIG_LWIP_IPV6
        _POPULATE;
        if (family() == AF_INET6) {
			//accessor allow "safe" access through optimization level aka dereferencing type-punned pointer
			void* accessor = &((struct sockaddr_in6*)_buffer.get())->sin6_addr;
			return *reinterpret_cast<network::ipv6_address_type*>(accessor);
        }
#endif
        return invalidIpv6;
    }

    uint16_t network::port() const {
#if defined(CONFIG_LWIP_IPV4) || defined(CONFIG_LWIP_IPV6)
        _POPULATE;
#endif
#ifdef CONFIG_LWIP_IPV4
        if (auto f = family(); f == AF_INET) {
            return ((struct sockaddr_in*)_buffer.get())->sin_port;
        }
#endif
#ifdef CONFIG_LWIP_IPV6
        else if (f == AF_INET6) {
            return ((struct sockaddr_in6*)_buffer.get())->sin6_port;
        }
#endif
        return 0;
    }

    network::mac_address_type network::mac() const {
        _POPULATE;
#ifdef CONFIG_LWIP_IPV4
        if (auto result = queryNetIf(_handler); result.netif != nullptr) {
            if (result.family == AF_INET) {
                ip4_addr_t ipv4in = ip4((sockaddr_in*)_buffer.get());
                const ip4_addr_t* ipv4out   = nullptr;
                eth_addr* macOut  = nullptr;
                if (etharp_find_addr((struct netif*)esp_netif_get_netif_impl(result.netif), &ipv4in, &macOut, &ipv4out) != -1) {
                    return {{macOut->addr[0], macOut->addr[1], macOut->addr[2], macOut->addr[3], macOut->addr[4], macOut->addr[5]}};
                }
            }
#endif
#ifdef CONFIG_LWIP_IPV6
            else if (result.family == AF_INET6) {
                struct pbuf buf     = {};
                ip6_addr_t  ipv6in  = ip6((sockaddr_in6*)_buffer.get());
                const uint8_t* macOut  = nullptr;
                //nd6_get_next_hop_entry is more suitable there but it's private
                //nd6_get_next_hop_addr_or_queue can execute query
                if (nd6_get_next_hop_addr_or_queue((struct netif*)esp_netif_get_netif_impl(result.netif), &buf, &ipv6in, &macOut) == 0) {
                    if (macOut != nullptr) {
                        return {{macOut[0], macOut[1], macOut[2], macOut[3], macOut[4], macOut[5]}};
                    }
                }
            }
#endif
        }
        return invalidMac;
    }

}

std::ostream& operator<<(std::ostream& stream, const http::network::ipv4_address_type& ip4addr) {
#ifdef CONFIG_LWIP_IPV4
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET6, &ip4addr, ip_str, INET_ADDRSTRLEN);
    return stream << ip_str;
#else
    return stream << "0.0.0.0";
#endif
}

std::ostream& operator<<(std::ostream& stream, const http::network::ipv6_address_type& ip6addr) {
#ifdef CONFIG_LWIP_IPV6
    char ip_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &ip6addr, ip_str, INET6_ADDRSTRLEN);
    return stream << ip_str;
#else
    return stream << "::";
#endif
}

std::ostream& operator<<(std::ostream& stream, const http::network::mac_address_type& macaddr) {
    char mac_str[20];
    std::snprintf(mac_str, 20, MACSTR, MAC2STR(macaddr.addr));
    return stream << mac_str;
}

