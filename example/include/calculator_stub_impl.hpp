#ifndef _CALCULATOR_STUB_IMPL_HPP
#define _CALCULATOR_STUB_IMPL_HPP

#include <calculator_stub.hpp>


namespace calculator {

class calculator_stub_impl_t final: protected calculator::calculator_stub_t {
public:

    float on_add(float, float) const final override;

    void on_beepboop() const final override;

    uint32_t on_get_precision() const final override;

    void on_set_precision(uint32_t) final override;

private:

    uint32_t m_Precision;
};


} // namespace calculator

#endif // _CALCULATOR_STUB_IMPL_HPP