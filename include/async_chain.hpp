#ifndef _ASYNC_CHAIN_HPP
#define _ASYNC_CHAIN_HPP

#include <memory>
#include <functional>
#include <vector>

namespace threesomeip::utils {

enum class step_status_t {
    SUCCESS,
    PENDING,
    FAILURE
};

template <typename Context = void>
class async_chain_t;


template <typename Context>
class async_chain_t: public std::enable_shared_from_this<async_chain_t<Context>> {
public:
    using Pointer = std::shared_ptr<async_chain_t<Context>>;
    using ChainCallback = std::function<void(step_status_t)>;
    using ResultCallback = std::function<void(step_status_t, std::shared_ptr<Context>)>;
    using StepFunction = std::function<step_status_t(std::shared_ptr<Context>, ChainCallback)>;


    template <typename... ContextArgs>
    static Pointer create(ResultCallback on_done, ContextArgs&&... args) {
        return Pointer(new async_chain_t(
            std::move(on_done),
            std::make_shared<Context>(std::forward<ContextArgs>(args)...)
        ));
    }

    async_chain_t* then(StepFunction step) {
        m_steps.push_back(std::move(step));
        return this;
    }

    void run() { this->run_step(0); }

private:
    async_chain_t(ResultCallback on_done, std::shared_ptr<Context> context)
        : m_on_done(std::move(on_done)), m_context(std::move(context)) {}

    void run_step(size_t index) {
        if (index == m_steps.size()) {
            m_on_done(step_status_t::SUCCESS, m_context);
            return;
        }

        auto self = this->shared_from_this();
        auto delayed_cb = [self, index](step_status_t result) {
            if (result == step_status_t::SUCCESS) self->run_step(index + 1);
            else self->m_on_done(step_status_t::FAILURE, self->m_context);
        };

        step_status_t immediate_result = m_steps[index](m_context, delayed_cb);
        if (immediate_result != step_status_t::PENDING) delayed_cb(immediate_result);
    }

    ResultCallback m_on_done;
    std::shared_ptr<Context> m_context;
    std::vector<StepFunction> m_steps;
};


template <>
class async_chain_t<void>: public std::enable_shared_from_this<async_chain_t<void>> {
public:
    using Pointer = std::shared_ptr<async_chain_t<void>>;
    using ChainCallback = std::function<void(step_status_t)>;
    using ResultCallback = std::function<void(step_status_t)>;
    using StepFunction = std::function<step_status_t(ChainCallback)>;


    static Pointer create(ResultCallback on_done) {
        return Pointer(new async_chain_t(std::move(on_done)));
    }

    async_chain_t* then(StepFunction step) {
        m_steps.push_back(std::move(step));
        return this;
    }

    void run() { this->run_step(0); }

private:
    async_chain_t(ResultCallback on_done) : m_on_done(std::move(on_done)) {}

    void run_step(size_t index) {
        if (index == m_steps.size()) {
            m_on_done(step_status_t::SUCCESS);
            return;
        }

        auto self = this->shared_from_this();
        auto delayed_cb = [self, index](step_status_t result) {
            if (result == step_status_t::SUCCESS) self->run_step(index + 1);
            else self->m_on_done(step_status_t::FAILURE);
        };

        step_status_t immediate_result = m_steps[index](delayed_cb);
        if (immediate_result != step_status_t::PENDING) delayed_cb(immediate_result);
    }

    ResultCallback m_on_done;
    std::vector<StepFunction> m_steps;
};

} // namespace threesomeip::utils

#endif // _ASYNC_CHAIN_HPP