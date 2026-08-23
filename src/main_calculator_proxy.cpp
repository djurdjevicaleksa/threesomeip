#include <calculator_proxy.hpp>
#include <runtime_proxy.hpp>

#include <print>

int main() {
    using namespace threesomeip;

    runtime::runtime_proxy_t client{
        "/home/lecq/Desktop/threesomeip/ipc_sockets",
        "calculator_proxy",
        uint16_t{1},
        "runtime",
        {},
        std::vector<config::service_configuration_t>{
            config::service_configuration_t{
                uint16_t{0x1234},
                uint16_t{0x0000},
                uint16_t{31000},
                uint16_t{30506}
            }
        }
    };

    calculator::calculator_proxy_t calc{client};

    std::getchar();

    auto result_future = calc.add(1.2, 3.4);

    std::print("Result: {}", result_future.get());

    return 0;
}