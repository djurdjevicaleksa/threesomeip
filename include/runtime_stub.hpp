#ifndef _RUNTIME_STUB_HPP
#define _RUNTIME_STUB_HPP

/*=====*\
 * C++ *
\*=====*/
#include <filesystem>
#include <string>
#include <utility>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <span>
#include <cstddef>
#include <list>
#include <variant>

/*=============*\
 * APPLICATION *
\*=============*/
#include <comm_ipc.hpp>
#include <udsocket.hpp>
#include <tcpsocket.hpp>
#include <active_object.hpp>
#include <timer.hpp>
#include <configurable.hpp>
#include <multi_index_map.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/
#include <spdlog/logger.h>


namespace fs = std::filesystem;

namespace threesomeip::runtime {
using namespace threesomeip;


class runtime_stub_t: public configurable_t {
public:

    runtime_stub_t(fs::path configuration_path, utils::active_object_ptr_t active_object);

private:

    using service_id_t = uint16_t;
    using application_id_t = uint16_t;

    /* IPC comm */
    void handle_on_receive(ipc::ud_socket_t& self, const ipc::types::socket_handle_t& sender, const std::span<const std::byte> data) noexcept;
    void handle_register_application(const ipc::types::socket_handle_t& sender, const std::span<const std::byte> data);
    void handle_unregister_application(const ipc::types::socket_handle_t& sender, const std::span<const std::byte> data);
    void handle_offer_services(const ipc::types::socket_handle_t& sender, const std::span<const std::byte> data);
    void handle_request_services(const ipc::types::socket_handle_t& sender, const std::span<const std::byte> data);
    void handle_send(const ipc::types::socket_handle_t& sender, const std::span<const std::byte> data);
    void handle_heartbeat(const ipc::types::socket_handle_t& sender, const std::span<const std::byte> data);

    /* RELIABLE comm */
    void handle_on_receive_reliable(const std::string& address, const int port, const std::span<const std::byte> data) noexcept;
    void handle_on_reliable_assembled(const std::string& address, const int port, const std::span<const std::byte> data) noexcept;

    std::string_view message_type_name(ipc::types::message_type_t type) const;

    std::vector<std::byte> wrap_with_ipc_header(const std::span<const std::byte> data, ipc::types::message_type_t message_type);


    struct application_entry_t {
        ipc::types::socket_handle_t handle;
        application_id_t app_id;
        std::string app_name;
        std::vector<config::service_configuration_t> offered_services;
        std::vector<config::service_configuration_t> requested_services;
    };

    void evict_application(const application_entry_t app);


    struct request_key_t {
        uint16_t service_id;    /* whose services are requested */
        uint16_t method_id;     /* their method* */
        uint16_t client_id;     /* who requests the service */
        uint16_t session_id;    /* counter */

        bool operator==(const request_key_t&) const = default;

        struct hash {
            size_t operator()(const request_key_t& k) const {
                size_t h = std::hash<uint16_t>{}(k.service_id);
                h ^= std::hash<uint16_t>{}(k.method_id)  + 0x9e3779b9 + (h << 6) + (h >> 2);
                h ^= std::hash<uint16_t>{}(k.client_id)  + 0x9e3779b9 + (h << 6) + (h >> 2);
                h ^= std::hash<uint16_t>{}(k.session_id) + 0x9e3779b9 + (h << 6) + (h >> 2);
                return h;
            }
        };
    };

    request_key_t makeRequestKey(const someip::types::message_header_t& header);


    using application_map_t = utils::multi_index_unordered_map_t<application_entry_t, ipc::types::socket_handle_t, service_id_t>;


    utils::active_object_ptr_t m_active_object;
    ipc::types::socket_handle_t m_own_socket_handle;

    ipc::ud_socket_t m_socket;
    net::tcp_socket_t m_reliable;

    std::shared_ptr<spdlog::logger> m_logger;

    application_map_t m_apps;

    std::unordered_map<request_key_t, std::variant<ipc::types::socket_handle_t, net::endpoint_t>, request_key_t::hash> m_pending_requests;

    /*
        heartbeat
    */
    struct heartbeat_t {
        ipc::types::socket_handle_t sender;
        std::chrono::steady_clock::time_point timepoint;
    };
    /* front is the least recent */
    std::list<heartbeat_t> m_heartbeat_by_recency;
    std::unordered_map<ipc::types::socket_handle_t, std::list<heartbeat_t>::iterator> m_heartbeat_lookup;

    std::shared_ptr<utils::timer_handle_t> m_eviction_timer;


    struct assembly_site_t {
        std::vector<std::byte> data;
        size_t declared_payload_size;
    };
    std::unordered_map<net::endpoint_t, assembly_site_t, net::endpoint_t::hash> m_tcp_package_assembly;
};

} // namespace threesomeip::runtime

#endif // _RUNTIME_STUB_HPP