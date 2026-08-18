#pragma once
#include <cstdint>
#include <functional>
#include <future>

namespace threesomeip::app {

using subscription_handle_t = uint32_t;

// ============================================================
// PROXY — client side
// ============================================================
class calculator_proxy_t {
public:
    virtual ~calculator_proxy_t() = default;

    // request-response
    virtual int32_t add(int32_t a, int32_t b) = 0;

    // fire-and-forget
    virtual void reset() = 0;

    // field (get + change notification)
    virtual int32_t get_precision() = 0;
    virtual subscription_handle_t subscribe_precision(std::function<void(int32_t)> on_change) = 0;

    // event
    virtual subscription_handle_t subscribe_on_overflow(std::function<void(int32_t)> callback) = 0;

    virtual void unsubscribe(subscription_handle_t) = 0;
};

// ============================================================
// STUB BASE — server side, you derive from this
// ============================================================
class calculator_stub_t {
public:
    virtual ~calculator_stub_t() = default;

    // request-response
    virtual int32_t on_add(int32_t a, int32_t b) = 0;

    // fire-and-forget
    virtual void on_reset() = 0;

    // field
    virtual int32_t on_get_precision() = 0;

    // you call these to push to clients
    void fire_overflow(int32_t result);
    void notify_precision_changed(int32_t value);

protected:
    virtual void on_message(/* someip_header_t, payload */) = 0;
};

// ============================================================
// CONCRETE IMPL — what you actually write
// ============================================================
class calculator_impl_t final : public calculator_stub_t {
public:
    int32_t on_add(int32_t a, int32_t b) override { return a + b; }
    void on_reset() override { m_precision = 2; }
    int32_t on_get_precision() override { return m_precision; }

private:
    int32_t m_precision{2};
};

} // namespace threesomeip::app