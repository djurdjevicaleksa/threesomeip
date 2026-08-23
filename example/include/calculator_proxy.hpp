#ifndef _CALCULATOR_PROXY_HPP
#define _CALCULATOR_PROXY_HPP

/*=====*\
 * C++ *
\*=====*/
#include <cstdint>
#include <future>
#include <unordered_map>
#include <any>
#include <memory>
#include <span>
#include <cstddef>

/*=============*\
 * APPLICATION *
\*=============*/
#include <runtime_proxy.hpp>


namespace calculator {
using namespace threesomeip::someip::types;
using namespace threesomeip;

class calculator_proxy_t {
private:
    static constexpr uint16_t SERVICE_ID = 0x1234;
    static constexpr uint16_t ADD_METHOD_ID = 0x0000;
    static constexpr uint16_t BEEPBOOP_METHOD_ID = 0x0001;
    static constexpr uint16_t GET_PRECISION_GETTER_ID = 0x0002;
    static constexpr uint16_t SET_PRECISION_SETTER_ID = 0x0003;
    static constexpr uint16_t SOMEIP_PROTOCOL_VERSION = 0x01;
    static constexpr uint16_t INTERFACE_VERSION = 0x01;
    static constexpr std::array METHOD_IDS{ADD_METHOD_ID, BEEPBOOP_METHOD_ID, GET_PRECISION_GETTER_ID, SET_PRECISION_SETTER_ID};

public:

    calculator_proxy_t(runtime::runtime_proxy_t& runtime_proxy);

    /* request-response method */
    std::future<float> add(float a, float b);

    /* fire-and-forget method */
    void beepboop();

    /* field getter */
    std::future<uint32_t> get_precision();

    /* field setter */
    std::future<void> set_precision(uint32_t);

    void on_message(std::span<const std::byte> payload);

private:

    uint16_t m_session_counter;
    std::unordered_map<uint16_t, std::shared_ptr<void>> m_pending_requests;
    runtime::runtime_proxy_t& m_runtime_proxy;
};

} // namespace calculator

#endif // _CALCULATOR_PROXY_HPP