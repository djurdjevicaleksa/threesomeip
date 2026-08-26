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
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>


int main(int argc, char** argv) {
    using namespace threesomeip;

    const std::string app_name{"runtime"};

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%H:%M:%S.%e][%n]%^[%l] %v%$");
    console_sink->set_color(spdlog::level::info, console_sink->green);
    console_sink->set_color(spdlog::level::warn, console_sink->yellow);
    console_sink->set_color(spdlog::level::err, console_sink->red);
    console_sink->set_color(spdlog::level::critical, console_sink->red_bold);

    auto main_logger = std::make_shared<spdlog::logger>(app_name, console_sink);
    spdlog::register_logger(main_logger);

    auto active_object = utils::active_object_factory::make_active_object(app_name);



    runtime::runtime_stub_t runtime{
        active_object,
        "/home/adjurdjevic/Desktop/threesomeip/ipc_sockets",
        "runtime"
    };

    std::getchar();

    return 0;
}