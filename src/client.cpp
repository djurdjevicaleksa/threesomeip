/*=====*\
 * C++ *
\*=====*/
#include <span>
#include <cstddef>
#include <string>
#include <iostream>
#include <cstdint>

/*=============*\
 * APPLICATION *
\*=============*/
#include <runtime_proxy.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/


int main(int argc, char** argv) {
    using namespace threesomeip::runtime;

    runtime_proxy_t client{
        "/home/lecq/Desktop/threesomeip/ipc_sockets",
        "test_client",
        uint16_t{1},
        "runtime",
        std::vector<threesomeip::config::service_configuration_t>{
            threesomeip::config::service_configuration_t{
                static_cast<uint16_t>(0x1234),
                static_cast<uint16_t>(0x5678),
                static_cast<uint16_t>(31000),
                static_cast<uint16_t>(30509)
            }
        },
        std::vector<threesomeip::config::service_configuration_t>{
            threesomeip::config::service_configuration_t{
                static_cast<uint16_t>(0x1235),
                static_cast<uint16_t>(0x5678),
                static_cast<uint16_t>(31000),
                static_cast<uint16_t>(30506)
            }
        }
    };


    std::getchar();

    return 0;
}