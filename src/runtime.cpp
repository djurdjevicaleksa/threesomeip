/*=====*\
 * C++ *
\*=====*/
#include <span>
#include <cstddef>
#include <string>
#include <iostream>

/*=============*\
 * APPLICATION *
\*=============*/
#include <udsocket.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/


int main(int argc, char** argv) {
    using namespace threesomeip::ipc;

    ud_socket_t runtime{
        "/home/lecq/Desktop/threesomeip/ipc_sockets/runtime.sock",
        [] (ud_socket_t& self, const socket_handle_t& sender, const std::span<const std::byte> data) {
            std::cout << "Received data:" << std::endl << std::string_view{reinterpret_cast<const char*>(data.data()) , data.size()} << std::endl;
        }
        // Invalid reception example; try to deserialize then print
    };

    std::this_thread::sleep_for(std::chrono::seconds(60));


    return 0;
}