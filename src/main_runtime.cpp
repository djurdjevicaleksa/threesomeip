/*=====*\
 * C++ *
\*=====*/
#include <print>

/*===========*\
 * APPLICATION *
\*===========*/
#include <ecu_configuration.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/

int main(int argc, char** argv) {

    const auto config = threesomeip::parseEcuConfiguration("/home/lecq/Desktop/actuallySomeip/someip_ecu.json");
    if (!config.has_value()) {
        std::println("Error:");
        std::println("{}", to_string(config.error()));
        return 1;
    }

    std::println("SUCCESS");
    return 0;
}