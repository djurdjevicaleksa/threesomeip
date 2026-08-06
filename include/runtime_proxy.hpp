#ifndef _RUNTIME_PROXY_HPP
#define _RUNTIME_PROXY_HPP

/*=====*\
 * C++ *
\*=====*/
#include <cstdint>
#include <string>
#include <span>

/*=============*\
 * APPLICATION *
\*=============*/
#include <configuration.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/


namespace threesomeip {

class runtime_proxy_t {
public:

    runtime_proxy_t(std::string_view app_name, uint16_t app_id, std::string_view runtime_socket_name);
    ~runtime_proxy_t();

    int registerApplication(std::string_view name, uint16_t id);
    int unregisterApplication();
    int offerServices(std::span<config::service_configuration_t> services);
    int requestServices(std::span<config::service_configuration_t> services);

private:



};


} // namespace threesomeip


#endif // _RUNTIME_PROXY_HPP