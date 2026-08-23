#include <calculator_stub_impl.hpp>
#include <runtime_proxy.hpp>

int main() {
    using namespace threesomeip;

    runtime::runtime_proxy_t stub{
        "/home/lecq/Desktop/threesomeip/ipc_sockets",
        "calculator_stub",
        uint16_t{2},
        "runtime",
        std::vector<config::service_configuration_t>{
            config::service_configuration_t{
                uint16_t{0x1234},
                uint16_t{0x0000},
                uint16_t{31000},
                uint16_t{30506}
            }
        },
        {}
    };

    calculator::calculator_stub_impl_t calc{stub};


    std::getchar();



    return 0;
}