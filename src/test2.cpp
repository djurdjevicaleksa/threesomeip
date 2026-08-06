#include <span>
#include <string>
#include <cstddef>
#include <iostream>
#include <cstring>
#include <cassert>
#include <cstddef>

#include <udsocket.hpp>
#include <ipc_format.hpp>

int main(int argc, char** argv) {

    threesomeip::ipc::ud_socket_t socket{
        "/home/adjurdjevic/Desktop/threesomeip/ipc_sockets/test2.sock",
        [](threesomeip::ipc::ud_socket_t& self, const std::string_view sender_pathname, const std::span<const std::byte> data) {
            std::cout << "Received message:" << std::string_view{reinterpret_cast<const char*>(data.data()), data.size()} << std::endl;
        }
    };

    std::string message{"Hello from test2!"};
    std::span<const std::byte> payload{reinterpret_cast<const std::byte*>(message.data()), message.size()};

    assert(socket.is_alive());

    socket.send("/home/adjurdjevic/Desktop/threesomeip/ipc_sockets/test1.sock",
        payload,
        [] {
            std::cout << "FATAL ERROR" << std::endl;
        }
    );

    std::this_thread::sleep_for(std::chrono::minutes(5));

    return 0;
}