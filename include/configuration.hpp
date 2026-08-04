#ifndef _CONFIGURATION_HPP
#define _CONFIGURATION_HPP

/*=====*\
 * C++ *
\*=====*/
#include <string>
#include <cstdint>
#include <vector>


namespace threesomeip::config {

struct logging_configuration_t {
    std::string output_file_name;
    std::string level;
    bool console;
    bool dlt; // unused TODO find out what this is
};

struct application_configuration_t {
    std::string name;
    uint16_t id;
};

struct service_configuration_t {
    uint16_t service_id;
    uint16_t instance_id;
    uint16_t udp_port;
    uint16_t tcp_port;
};

// TODO service_discovery_configuration_t {};


struct ecu_configuration_t {
    std::string unicast_address;
    logging_configuration_t logging;
    std::vector<application_configuration_t> applications;
    std::vector<service_configuration_t> services;
    std::string runtime_application_name;
    // TODO service_discovery_configuration_t sd;
};

} // namespace threesomeip

#endif // _CONFIGURATION_HPP