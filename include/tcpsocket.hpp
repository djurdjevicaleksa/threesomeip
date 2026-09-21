#ifndef _TCP_SOCKET_HPP
#define _TCP_SOCKET_HPP

#include <functional>
#include <cstddef>
#include <span>
#include <string>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <queue>

#include <active_object.hpp>
#include <lifecycle_listener.hpp>
#include <awaitable.hpp>

#include <spdlog/spdlog.h>


namespace threesomeip::net {
using namespace threesomeip;


enum class send_result_t {
    SENT = 0,
    DELAYED_RESULT,
    SOCKET_DEAD,
    EPHEMERAL_SOCKET_DEAD
};

struct endpoint_t {
    std::string address;
    int port;

    bool operator==(const endpoint_t&) const = default;

    struct hash {
        size_t operator()(const endpoint_t& e) const {
            size_t h = std::hash<std::string>{}(e.address);
            h ^= std::hash<int>{}(e.port) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };
};


class tcp_socket_t: public ipc::lifecycle_listener_t {
public:

    using DelayedResultCallback = std::function<void(const send_result_t result, const std::string& address, const int port, const std::span<const std::byte> data)>;
    using ReceiveCallback = std::function<void(const std::string& address, const int port, const std::span<const std::byte> data)>;

    tcp_socket_t(utils::active_object_ptr_t active_object, const std::string& address, const int port, ReceiveCallback on_receive);

    ~tcp_socket_t() noexcept {}

    utils::detached_task_t send_to(const std::string& address, const int port, std::span<const std::byte> data, std::optional<DelayedResultCallback> on_delayed_result);

private:

    void init() noexcept;

    utils::awaitable_t<int> connect_or_reuse_connection(const std::string& address, const int port);

    utils::awaitable_t<send_result_t> write_all(const int fd, std::shared_ptr<std::vector<std::byte>> data);

    int _internal_detail_connect_to(const std::string& address, const int port, std::function<void(int)> on_delayed_result);

    void accept_connections();

    void receive_messages(const int fd);

    void retry_messages(const int fd);

    void evict_connection(const int fd);

    void on_dead() override;

    void on_alive() override;


    struct pending_message_t {
        std::vector<std::byte> data;
        size_t bytes_already_written;
        DelayedResultCallback on_delayed_result;
    };

    struct connection_details_t {
        endpoint_t recipient;
        std::queue<pending_message_t> pending_messages;
    };


    utils::active_object_ptr_t m_active_object;
    ReceiveCallback m_on_receive;

    int m_fd;
    std::string m_address;
    int m_port;

    std::unordered_map<endpoint_t, int, endpoint_t::hash> m_connection_fds;
    std::unordered_map<int, connection_details_t> m_connection_details;

    std::shared_ptr<spdlog::logger> m_logger;
};

} // namespace threesomeip::net

#endif // _TCP_SOCKET_HPP