/*=====*\
 * C++ *
\*=====*/
#include <print>

/*===========*\
 * APPLICATION *
\*===========*/
#include <runtime.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/

int main(int argc, char** argv) {

    threesomeip::runtime_t runtime("/home/lecq/Desktop/actuallySomeip/someip_ecu.json");
    runtime.run();

    return 0;
}