#ifndef _CALCULATOR_STUB_IMPL_HPP
#define _CALCULATOR_STUB_IMPL_HPP

/*=============*\
 * APPLICATION *
\*=============*/
#include <calculator_stub.hpp>


namespace calculator {

class calculator_stub_impl_t final: public calculator::calculator_stub_t {
public:

    calculator_stub_impl_t(runtime::runtime_proxy_t& runtime_proxy);

    float on_add(float, float) const final override;

    void on_beepboop() const final override;

    uint32_t on_get_precision() const final override;

    void on_set_precision(uint32_t) final override;

private:

    uint32_t m_Precision;
};


} // namespace calculator

#endif // _CALCULATOR_STUB_IMPL_HPP