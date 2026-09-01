#include <span>
#include <cstddef>

#include <tcpsocket.hpp>
#include <active_object.hpp>

#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/bin_to_hex.h>


int main() {
    using namespace threesomeip;

    const std::string app_name{"tcp2"};

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%H:%M:%S.%e][" + app_name + "][%n]%^[%l] %v%$");
    console_sink->set_color(spdlog::level::info, console_sink->green);
    console_sink->set_color(spdlog::level::warn, console_sink->yellow);
    console_sink->set_color(spdlog::level::err, console_sink->red);
    console_sink->set_color(spdlog::level::critical, console_sink->red_bold);

    auto main_logger = std::make_shared<spdlog::logger>(app_name, console_sink);
    main_logger->set_level(spdlog::level::debug);
    spdlog::register_logger(main_logger);

    auto active_object = utils::active_object_factory::make_active_object(app_name);

    net::tcp_socket_t m_socket{active_object, "172.17.0.1", 5679, [&main_logger](const std::string& address, const int port, const std::span<const std::byte> data) -> void {
        main_logger->debug("Received: {}", spdlog::to_hex(data));
    }};

    std::getchar();

}