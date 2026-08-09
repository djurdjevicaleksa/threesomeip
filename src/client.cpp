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

    runtime_proxy_t client{"/home/lecq/Desktop/threesomeip/ipc_sockets", "test_client", uint16_t{1}, "runtime"};
    client.registerApplication("test-client", 1);

    std::this_thread::sleep_for(std::chrono::seconds(30));

    return 0;
}