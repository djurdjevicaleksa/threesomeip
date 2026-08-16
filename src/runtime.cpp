/*=====*\
 * C++ *
\*=====*/
#include <span>
#include <cstddef>
#include <string>
#include <iostream>
#include <format>

/*=============*\
 * APPLICATION *
\*=============*/
#include <udsocket.hpp>
#include <serialization.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/logger.h>

int main(int argc, char** argv) {
    using namespace threesomeip::ipc;

    auto logger = spdlog::stdout_color_st("Runtime");
    logger->set_level(spdlog::level::debug);
    logger->set_pattern("[%H:%M:%S.%e][%n][%l] %v");

    ud_socket_t runtime{
        "/home/lecq/Desktop/threesomeip/ipc_sockets/runtime.sock",
        [&logger] (ud_socket_t& self, const socket_handle_t& sender, const std::span<const std::byte> data) {
            auto header = std::get<0>(serdes::deserialize<ipc_message_header_t>(data.data()));
            logger->info(
                std::format(
                    "Received payload:\n{}\n{}\n{}\n{}\n{}\n{}\n{}",
                    std::string_view{reinterpret_cast<const char*>(&header.start_of_frame), header.start_of_frame.size()},
                    std::to_string(header.protocol_version),
                    std::to_string(static_cast<uint8_t>(header.message_type)),
                    std::to_string(header._flags),
                    std::to_string(header._request_id),
                    std::to_string(header._reserved),
                    std::to_string(header.payload_length)
                )
            );
        }
        // Invalid reception example; try to deserialize then print
    };

    std::this_thread::sleep_for(std::chrono::seconds(60));


    return 0;
}