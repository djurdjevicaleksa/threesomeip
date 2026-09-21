#ifndef _TCP_PACKET_ASSEMBLER_HPP
#define _TCP_PACKET_ASSEMBLER_HPP


#include <span>
#include <cstddef>
#include <unordered_map>
#include <vector>
#include <functional>

#include <tcpsocket.hpp>


namespace threesomeip::net {


class tcp_packet_assembler_t {
public:

    tcp_packet_assembler_t(std::function<void()> on_package_assembled);

    void give_package_part(endpoint_t endpoint, const std::span<const std::byte> data);

private:

    

    std::function<void()> m_on_package_assembled;
    std::unordered_map<endpoint_t, assembly_site_t, endpoint_t::hash> m_data;
};

} // namespace threesomeip::net

#endif // _TCP_PACKET_ASSEMBLER_HPP