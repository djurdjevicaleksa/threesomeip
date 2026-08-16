/*=====*\
 * C++ *
\*=====*/
#include <cstdio>

/*=============*\
 * APPLICATION *
\*=============*/
#include <runtime_stub.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/

int main(int argc, char** argv) {
    using namespace threesomeip::runtime;

    runtime_stub_t runtime{
        "/home/lecq/Desktop/threesomeip/ipc_sockets",
        "runtime"
    };


    std::getchar();

    return 0;
}