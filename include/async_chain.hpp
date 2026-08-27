#ifndef _ASYNC_CHAIN_HPP
#define _ASYNC_CHAIN_HPP

/*=====*\
 * C++ *
\*=====*/
#include <memory>
#include <functional>
#include <vector>
#include <cstddef>
#include <optional>


namespace threesomeip::utils {


template<typename ResultType, typename... Args>
class async_chain_t: public std::enable_shared_from_this<async_chain_t<ResultType, Args...>> {
public:
    using Pointer = std::shared_ptr<async_chain_t>;
    using DelayedCallback = std::function<void(ResultType, Args...)>;
    using StepFunction = std::function<ResultType(std::optional<DelayedCallback>)>;
    using ResultCallback = std::function<void(ResultType)>;

    static Pointer create(ResultType pending_sentinel, ResultType success_sentinel, ResultCallback on_done) {
        return Pointer(new async_chain_t(pending_sentinel, success_sentinel, std::move(on_done)));
    }

    async_chain_t* then(StepFunction step) {
        m_steps.push_back(std::move(step));
        return this;
    }

    void run() {
        this->run_step(0);
    }

private:

    async_chain_t(ResultType pending_sentinel, ResultType success_sentinel, ResultCallback on_done):
        m_pending_sentinel(pending_sentinel),
        m_success_sentinel(success_sentinel),
        m_on_done(std::move(on_done))
    {}

    void run_step(size_t index) {
        if (index == m_steps.size()) {
            m_on_done(m_success_sentinel);
            return;
        }

        auto self = this->shared_from_this();
        auto handle_next = [self, index] (ResultType result) {
            if (result == self->m_success_sentinel) self->run_step(index + 1);
            else self->m_on_done(result);
        };

        const ResultType in_between_result = m_steps[index](
            [handle_next] (ResultType result, Args...) {
                handle_next(result);
            }
        );

        if (in_between_result != m_pending_sentinel) {
            handle_next(in_between_result);
        }
    }


    ResultType m_pending_sentinel;
    ResultType m_success_sentinel;
    ResultCallback m_on_done;
    std::vector<StepFunction> m_steps;
};

} // namespace threesomeip::utils

#endif // _ASYNC_CHAIN_HPP