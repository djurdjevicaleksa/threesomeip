#ifndef _CALCULATOR_PROXY_HPP
#define _CALCULATOR_PROXY_HPP

#include <cstdint>

namespace calculator {

class calculator_proxy_t {
private:
    static constexpr uint16_t SERVICE_ID = 0x1234;
    static constexpr uint16_t ADD_METHOD_ID = 0x0000;
    static constexpr uint16_t BEEPBOOP_METHOD_ID = 0x0001;
    static constexpr uint16_t GET_PRECISION_GETTER_ID = 0x0002;
    static constexpr uint16_t SET_PRECISION_SETTER_ID = 0x0003;

public:

    /* request-response method */
    float add(float a, float b);

    /* fire-and-forget method */
    void beepboop();

    /* field getter */
    uint32_t get_precision();

    /* field setter */
    void set_precision(uint32_t);
};

} // namespace calculator

#endif // _CALCULATOR_PROXY_HPP