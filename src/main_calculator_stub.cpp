#include <chrono>

#include <calculator_stub_impl.hpp>
#include <runtime_proxy.hpp>

#include <active_object.hpp>
#include <timer.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

int main() {
    using namespace threesomeip;

    const std::string app_name{"calculator_stub"};

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%H:%M:%S.%e][" + app_name + "][%n]%^[%l] %v%$");
    console_sink->set_color(spdlog::level::info, console_sink->green);
    console_sink->set_color(spdlog::level::warn, console_sink->yellow);
    console_sink->set_color(spdlog::level::err, console_sink->red);
    console_sink->set_color(spdlog::level::critical, console_sink->red_bold);

    auto main_logger = std::make_shared<spdlog::logger>(app_name, console_sink);
    spdlog::register_logger(main_logger);

    auto active_object = utils::active_object_factory::make_active_object(app_name);


    runtime::runtime_proxy_t runtime_proxy{
        "/home/adjurdjevic/Desktop/threesomeip/someip.json",
        active_object,
        "calculator_stub",
        uint16_t{2},
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