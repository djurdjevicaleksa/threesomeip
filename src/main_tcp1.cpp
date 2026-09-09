#include <span>
#include <cstddef>
#include <string>
#include <cstdint>
#include <iostream>

#include <tcpsocket.hpp>
#include <active_object.hpp>
#include <multi_index_map.hpp>

#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/bin_to_hex.h>


int main() {
    using namespace threesomeip;

    // const std::string app_name{"tcp1"};

    // auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    // console_sink->set_pattern("[%H:%M:%S.%e][" + app_name + "][%n]%^[%l] %v%$");
    // console_sink->set_color(spdlog::level::info, console_sink->green);
    // console_sink->set_color(spdlog::level::warn, console_sink->yellow);
    // console_sink->set_color(spdlog::level::err, console_sink->red);
    // console_sink->set_color(spdlog::level::critical, console_sink->red_bold);

    // auto main_logger = std::make_shared<spdlog::logger>(app_name, console_sink);
    // main_logger->set_level(spdlog::level::debug);
    // spdlog::register_logger(main_logger);

    // auto active_object = utils::active_object_factory::make_active_object(app_name);

    // net::tcp_socket_t m_socket{active_object, "172.17.0.1", 5678, [&main_logger](const std::string& address, const int port, const std::span<const std::byte> data) -> void {
    //     main_logger->debug("Received: {}", spdlog::to_hex(data));
    // }};

    // std::getchar();

    // std::array<std::byte, 5> message{std::byte{0x00}, std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04}};
    // m_socket.send_to("172.17.0.1", 5679, message, [&main_logger] (const net::send_result_t result, const std::string& address, const int port, const std::span<const std::byte> data) {
    //     main_logger->debug("Delayed send: {}", static_cast<uint8_t>(result));
    // });

    // std::getchar();

    // utils::multi_index_unordered_map_t<std::string, int32_t, float, std::byte> map;

    // map.insert(1, "Aleksa", 2.0f, std::byte{3});

    // if (map.contains(static_cast<int32_t>(1))) {
    //     std::cout << "1 " <<  map.at(static_cast<int32_t>(1)) << std::endl;
    // }

    // if (map.contains(2.0f)) {
    //     std::cout << "2 " <<  map.at(2.0f) << std::endl;
    // }

    // if (map.contains(static_cast<std::byte>(3))) {
    //     std::cout << "2 " <<  map.at(static_cast<std::byte>(3)) << std::endl;
    // }




    utils::multi_index_unordered_map_t<std::string, int32_t, float, std::byte> map_2;

    map_2.insert(1, "Aleksa2");

    std::cout << "4 " <<  map_2.at(static_cast<int32_t>(1)) << std::endl;

    map_2.alias(1, 2.0f);

    std::cout << "5 " <<  map_2.at(static_cast<int32_t>(1)) << std::endl;
    std::cout << "6 " <<  map_2.at(2.0f) << std::endl;

    map_2.remove_alias(2.0f);

    std::cout << "7 " <<  map_2.at(static_cast<int32_t>(1)) << std::endl;
    try {
        std::cout << "6 " <<  map_2.at(2.0f) << std::endl;
    }
    catch (std::out_of_range& e) {
        std::cout << "8 Caught: " << e.what() << std::endl;
    }
}