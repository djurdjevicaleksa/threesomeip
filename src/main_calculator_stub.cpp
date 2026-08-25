#include <chrono>
#include <iostream>

#include <calculator_stub_impl.hpp>
#include <runtime_proxy.hpp>

#include <active_object.hpp>
#include <timer.hpp>

int main() {
    using namespace threesomeip;

    runtime::runtime_proxy_t stub{
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

    calculator::calculator_stub_impl_t calc{stub};

    auto active_object = utils::active_object_factory::make_active_object();
    auto timer1 = utils::timer_factory::make_oneshot_timer_go(active_object, std::chrono::seconds(5), [] () {
        std::cout << "timer1 expired" << std::endl;
    });

    auto timer2 = utils::timer_factory::make_periodic_timer_go(active_object, std::chrono::seconds(2), [] () {
        std::cout << "timer2 expired" << std::endl;
    });

    auto lam = [] () -> void {
        std::cout << "regular job" << std::endl;
    };

    for (size_t i{0}; i < 100; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        active_object->add_to_todo_list(lam);
    }

    std::this_thread::sleep_for(std::chrono::seconds(20));

    return 0;
}