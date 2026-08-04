/*=====*\
 * C++ *
\*=====*/
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <cstring>
#include <string>
#include <unistd.h>

/*=============*\
 * APPLICATION *
\*=============*/
#include <runtime_proxy.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/


namespace threesomeip {


runtime_proxy_t::runtime_proxy_t(std::string_view app_name, uint16_t app_id, std::string_view runtime_socket_name) {

    // TODO work on implementing SOCK_STREAM as the comm type

    if (int sock = socket(AF_UNIX, SOCK_STREAM, 0); sock < 0) {
        // Handle error
        return;
    }
    else {
        m_ud_socket = sock;
    }

    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    std::snprintf(
        address.sun_path,
        sizeof(address.sun_path),
        "/home/lecq/Desktop/actuallySomeip/sockets/%s_%d.sock",
        app_name.data(),
        app_id
    );

    unlink(address.sun_path);

    if (bind(m_ud_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        // Handle error
        close(m_ud_socket);
    }
}

runtime_proxy_t::~runtime_proxy_t() {
    close(m_ud_socket);
}

int runtime_proxy_t::registerApplication(std::string_view name, uint16_t id) {

}



} // namespace threesomeip
