/*=====*\
 * C++ *
\*=====*/
#include <cstdio>

/*=============*\
 * APPLICATION *
\*=============*/
#include <runtime_stub.hpp>
#include <active_object.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/

int main(int argc, char** argv) {
    using namespace threesomeip;

    auto active_object = utils::active_object_factory::make_active_object();

    runtime::runtime_stub_t runtime{
        active_object,
        "/home/adjurdjevic/Desktop/threesomeip/ipc_sockets",
        "runtime"
    };

    std::getchar();

    return 0;
}