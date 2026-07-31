/*=====*\
 * C++ *
\*=====*/
#include <string>
#include <cstdint>
#include <vector>
#include <expected>


namespace threesomeip {





class configurable_t {
public:

    configurable_t(const char*);

private:

    struct ecu_configuration_t {
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


        std::string unicast_address;
        logging_configuration_t logging;
        std::vector<application_configuration_t> applications;
        std::vector<service_configuration_t> services;
        std::string runtime_application_name;
        // TODO service_discovery_configuration_t sd;
    };

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
    std::expected<ecu_configuration_t, ConfigurationParsingError>
    parseEcuConfiguration(const char*) noexcept;

protected:

    ecu_configuration_t m_ecu_configuration;
};

} // namespace threesomeip