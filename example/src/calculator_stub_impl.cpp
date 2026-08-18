#include <print>

#include <calculator_stub_impl.hpp>


namespace calculator {

float calculator_stub_impl_t::on_add(float a, float b) const {
    return a + b;
}

void calculator_stub_impl_t::on_beepboop() const {
    std::println("Beep Boop!");
}

uint32_t calculator_stub_impl_t::on_get_precision() const {
    return m_Precision;
}

void calculator_stub_impl_t::on_set_precision(uint32_t newPrecision) {
    m_Precision = newPrecision;
}


} // namespace calculator