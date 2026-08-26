#include <chrono>

#include <calculator_stub_impl.hpp>
#include <runtime_proxy.hpp>

#include <active_object.hpp>
#include <timer.hpp>

int main() {
    using namespace threesomeip;

    auto active_object = utils::active_object_factory::make_active_object();

    runtime::runtime_proxy_t runtime_proxy{
        active_object,
        "/home/adjurdjevic/Desktop/threesomeip/ipc_sockets",
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

    calculator::calculator_stub_impl_t calculator_stub{runtime_proxy};

    std::getchar();

    return 0;
}