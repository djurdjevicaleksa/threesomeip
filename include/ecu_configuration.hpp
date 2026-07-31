/*=====*\
 * C++ *
\*=====*/
#include <string>
#include <cstdint>
#include <fstream>
#include <vector>
#include <expected>
#include <charconv>

/*===========*\
 * 3RD PARTY *
\*===========*/
#include <nlohmann/json.hpp>


namespace threesomeip {

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

namespace {
std::expected<uint16_t, ConfigurationParsingError> parseStringAsU16(std::string_view value) noexcept {

    int number_base = 10;

    if (value.compare("0x") or value.compare("0X")) number_base = 16;
    else if (value.compare("0o") or value.compare("0O")) number_base = 8;
    else if (value.compare("0b") or value.compare("0B")) number_base = 2;

    int prefix_offset = (number_base == 10) ? 0 : 2;

    uint16_t parsed_value = 0;
    const auto[pointer, ec] = std::from_chars(
        value.data() + prefix_offset,
        value.data() + value.length(),
        parsed_value,
        number_base
    );

    if (ec != std::errc{}) {
        return std::unexpected(ConfigurationParsingError::INVALID_NUMBER_INPUT);
    }

    return parsed_value;
}
}


std::expected<ecu_configuration_t, ConfigurationParsingError> parseEcuConfiguration(const char* ecu_configuration_file_path) noexcept {
    using json = nlohmann::json;

    std::ifstream ecu_config_file_handle(ecu_configuration_file_path);
    if (!ecu_config_file_handle.is_open()) {
        // Handle error
        return std::unexpected(ConfigurationParsingError::UNABLE_TO_OPEN_FILE);
    }

    try {
        ecu_configuration_t config{};

        json data = json::parse(ecu_config_file_handle);

        // TODO validate someip_ecu.json file

        config.unicast_address = std::move(data["unicast"]);

        config.logging.level = std::move(data["logging"]["level"]);
        config.logging.console = data["logging"]["console"];
        config.logging.output_file_name =
            static_cast<bool>(data["logging"]["file"]["enable"])?
                std::move(data["logging"]["file"]["path"]) : "";
        config.logging.dlt = data["logging"]["dlt"];

        for (const auto& application: data["applications"]) {
            config.applications.emplace_back(
                std::move(application["name"]),
                parseStringAsU16(static_cast<std::string>(application["id"])).value()
            );
        }

        for (const auto& service: data["services"]) {
            config.services.emplace_back(
                parseStringAsU16(static_cast<std::string>(service["service"])).value(),
                parseStringAsU16(static_cast<std::string>(service["instance"])).value(),
                parseStringAsU16(static_cast<std::string>(service["unreliable"])).value(),
                parseStringAsU16(static_cast<std::string>(service["reliable"]["port"])).value()
            );
        }

        config.runtime_application_name = std::move(data["routing"]);

        // TODO parse service discovery

        return config;
    }
    catch (std::exception& e) {
        return std::unexpected(ConfigurationParsingError::NLOHMANN_JSON_ERROR);
    }
}


} // namespace threesomeip