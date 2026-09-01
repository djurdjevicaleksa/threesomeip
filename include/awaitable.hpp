#ifndef _AWAITABLE_HPP
#define _AWAITABLE_HPP

#include <functional>
#include <optional>
#include <coroutine>
#include <exception>

namespace threesomeip::utils {


template<typename T>
class awaitable_t {
public:
    using Resume = std::function<void(T)>;
    using Starter = std::function<void(Resume)>;

    explicit awaitable_t(Starter starter): m_starter(std::move(starter)) {}

    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
        m_starter([this, handle] (T value) {
            m_value = std::move(value);
            handle.resume();
        });
    }

    T await_resume() { return std::move(*m_value); }

private:

    Starter m_starter;
    std::optional<T> m_value;
};


struct detached_task_t {
    struct promise_type {
        detached_task_t get_return_object() { return {}; }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
};

} // namespace threesomeip::utils

#endif // _AWAITABLE_HPP