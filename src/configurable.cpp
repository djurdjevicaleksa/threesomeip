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

/*=============*\
 * APPLICATION *
\*=============*/
#include <configurable.hpp>
#include <configuration.hpp>


namespace threesomeip {


configurable_t::configurable_t(const char* ecu_configuration_file_path) {
    auto loaded_configuration = parseEcuConfiguration(ecu_configuration_file_path);
    if (loaded_configuration) {
        m_ecu_configuration = loaded_configuration.value();
    }
    else throw new std::runtime_error("Failed to parse ecu configuration.");
}

std::expected<uint16_t, configurable_t::ConfigurationParsingError>
configurable_t::parseStringAsU16(std::string_view value) noexcept {

    int number_base = 10;

    if (value.compare("0x") or value.compare("0X")) number_base = 16;
    else if (value.compare("0o") or value.compare("0O")) number_base = 8;
    else if (value.compare("0b") or value.compare("0B")) number_base = 2;

    const uint8_t prefix_offset = (number_base == 10) ? 0 : 2;

    uint16_t parsed_value = 0;
    const auto[pointer, ec] = std::from_chars(
        value.data() + prefix_offset,
        value.data() + value.length(),
        parsed_value,
        number_base
    );
    if (ec != std::errc{}) return std::unexpected(ConfigurationParsingError::INVALID_NUMBER_INPUT);

    return parsed_value;
}

std::expected<config::ecu_configuration_t, configurable_t::ConfigurationParsingError>
configurable_t::parseEcuConfiguration(const char* ecu_configuration_file_path) noexcept {
    using json = nlohmann::json;

    std::ifstream ecu_config_file_handle(ecu_configuration_file_path);
    if (!ecu_config_file_handle.is_open()) {
        return std::unexpected(ConfigurationParsingError::UNABLE_TO_OPEN_FILE);
    }

    try {
        config::ecu_configuration_t config{};

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