#ifndef _RUNTIME_STUB_HPP
#define _RUNTIME_STUB_HPP

/*=====*\
 * C++ *
\*=====*/
#include <filesystem>

/*=============*\
 * APPLICATION *
\*=============*/
#include <ipc_format.hpp>
#include <udsocket.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/


namespace fs = std::filesystem;


namespace threesomeip::runtime {


class runtime_stub_t {
public:

    runtime_stub_t(const fs::path& sockets_path, std::string_view runtime_name) noexcept;

private:

    // void handle_delayed_socket_response(
    //     const threesomeip::ipc::send_result_t result,
    //     const threesomeip::ipc::socket_handle_t& recipient,
    //     const std::span<const std::byte> data
    // ) noexcept;

    void handle_on_receive(
        threesomeip::ipc::ud_socket_t& self,
        const threesomeip::ipc::socket_handle_t& sender,
        const std::span<const std::byte> data
    ) noexcept;

    threesomeip::ipc::socket_handle_t m_own_socket_handle;
    threesomeip::ipc::ud_socket_t m_socket;
};

} // namespace threesomeip::runtime

#endif // _RUNTIME_STUB_HPP