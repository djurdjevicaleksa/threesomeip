#ifndef _APPLICATION_RUNTIME_CLIENT
#define _APPLICATION_RUNTIME_CLIENT

/*=====*\
 * C++ *
\*=====*/
#include <string>
#include <cstdint>
#include <span>

/*=============*\
 * APPLICATION *
\*=============*/
#include <configurable.hpp>
#include <configuration.hpp>


namespace threesomeip {


class application_runtime_client: private configurable_t {
public:

    #errror "Implement constructor; which configuration file should clients load?"

    application_runtime_client();

    int register_application(std::string_view, uint16_t id);
    int offer_services(std::span<config::service_configuration_t> services);
    int subscribe_to_services(std::span<config::service_configuration_t> services);

private:

    int m_ud_socket;
    config::ecu_configuration_t m_configuration;
};



} // namespace threesomeip


#endif // _APPLICATION_RUNTIME_CLIENT