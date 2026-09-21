

#include <functional>
#include <span>
#include <cstddef>


#include <tcp_packet_assembler.hpp>


namespace threesomeip::net {


tcp_packet_assembler_t::tcp_packet_assembler_t(std::function<void()> on_packet_assembled):
    m_on_package_assembled(std::move(on_packet_assembled))
{}


void tcp_packet_assembler_t::give_package_part(endpoint_t endpoint, const std::span<const std::byte> data) {
    /* create one if it doesn't exist */
    auto& assembly_site = m_data[endpoint];

    if (assembly_site.data.size() == 0) {
        
        
        if ()

    }

}


} // namespace threesomeip::net