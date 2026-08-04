#ifndef _CONFIGURABLE_HPP
#define _CONFIGURABLE_HPP

/*=====*\
 * C++ *
\*=====*/
#include <string>
#include <cstdint>
#include <vector>
#include <expected>

/*=============*\
 * APPLICATION *
\*=============*/
#include <configuration.hpp>


namespace threesomeip {


class configurable_t {
public:

    configurable_t(const char*);

private:

    enum class ConfigurationParsingError {
        INVALID_NUMBER_SYSTEM = 0,
        INVALID_NUMBER_INPUT,
        UNABLE_TO_OPEN_FILE,
        NLOHMANN_JSON_ERROR
    };

    std::string to_string(ConfigurationParsingError error) {
        switch (error) {
            case ConfigurationParsingError::INVALID_NUMBER_SYSTEM: return "INVALID_NUMBER_SYSTEM";
            case ConfigurationParsingError::INVALID_NUMBER_INPUT: return "INVALID_NUMBER_INPUT";
            case ConfigurationParsingError::UNABLE_TO_OPEN_FILE: return "UNABLE_TO_OPEN_FILE";
            case ConfigurationParsingError::NLOHMANN_JSON_ERROR: return "NLOHMANN_JSON_ERROR";
            default: return "!UNKNOWN CONFIGURATION PARSING ERROR!";
        }
    }

    /*
        Converts numbers represented as strings from different numeric systems to a uint16_t.
    */
    std::expected<uint16_t, ConfigurationParsingError>
    parseStringAsU16(std::string_view) noexcept;

    /*
        Initializes internal configuration by reading the ecu configuration file.
    */
    std::expected<config::ecu_configuration_t, ConfigurationParsingError>
    parseEcuConfiguration(const char*) noexcept;

protected:

    config::ecu_configuration_t m_ecu_configuration;
};

} // namespace threesomeip

#endif // _CONFIGURABLE_HPP