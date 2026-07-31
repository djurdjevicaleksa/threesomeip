/*=====*\
 * C++ *
\*=====*/
#include <print>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <sys/un.h>
#include <cstring>

/*=============*\
 * APPLICATION *
\*=============*/
#include <runtime.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/


namespace threesomeip {


runtime_t::runtime_t(const char* ecu_configuration_file_path):
    configurable_t(ecu_configuration_file_path),
    m_ud_socket(-1)
{
    if (int sock = socket(AF_UNIX, SOCK_DGRAM, 0); sock < 0) {
        // Handle error
        return;
    }
    else {
        m_ud_socket = sock;
    }

    sockaddr_un address{};

    char socket_path[sizeof(address.sun_path)];
    std::memset(&socket_path, 0, sizeof(socket_path));
    std::snprintf(
        socket_path,
        sizeof(socket_path),
        "/home/lecq/Desktop/actuallySomeip/sockets/%s.sock",
        m_ecu_configuration.runtime_application_name.c_str()
    );

    address.sun_family = AF_UNIX;
    std::memcpy(address.sun_path, socket_path, sizeof(address.sun_path));

    unlink(socket_path);

    if (bind(m_ud_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        // Handle error
        if (m_ud_socket > 0) {
            close(m_ud_socket);
        }
    }
}


} // namespace threesomeip