#ifndef _TIMER_HPP
#define _TIMER_HPP

/*=====*\
 * C++ *
\*=====*/
#include <chrono>
#include <functional>
#include <sys/timerfd.h>
#include <algorithm>
#include <unistd.h>
#include <atomic>
#include <memory>
#include <cassert>

/*=============*\
 * APPLICATION *
\*=============*/
#include <active_object.hpp>


namespace threesomeip::utils {


class timer_handle_t: public std::enable_shared_from_this<timer_handle_t> {
public:

    bool start() {
        auto expected_state = timer_state_t::DISARMED;
        if (!m_state.compare_exchange_strong(
            expected_state,
            timer_state_t::ARMED,
            std::memory_order_acq_rel,
            std::memory_order_acquire
        )) return false;

        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(m_duration);
        auto s = std::chrono::duration_cast<std::chrono::seconds>(ns);
        ns -= s;

        itimerspec read_spec{0};
        timerfd_gettime(m_timerfd, &read_spec);
        read_spec.it_value = {s.count(), ns.count()};
        timerfd_settime(m_timerfd, 0, &read_spec, nullptr);

        m_active_object->add_fd_to_watchlist(m_timerfd, m_callback);

        return true;
    }

    bool pause() {
        if (m_state.load(std::memory_order_acquire) != timer_state_t::ARMED) return false;

        itimerspec read_spec{0};
        timerfd_gettime(m_timerfd, &read_spec);

        m_paused_timespec = read_spec.it_value;

        read_spec.it_value = {0, 0};
        timerfd_settime(m_timerfd, 0, &read_spec, nullptr);

        m_state.store(timer_state_t::PAUSED, std::memory_order_release);
        return true;
    }

    bool resume() {
        if (m_state.load(std::memory_order_acquire) != timer_state_t::PAUSED) return false;

        itimerspec read_spec{0};
        timerfd_gettime(m_timerfd, &read_spec);
        read_spec.it_value = m_paused_timespec;
        timerfd_settime(m_timerfd, 0, &read_spec, nullptr);

        m_state.store(timer_state_t::ARMED, std::memory_order_release);
        return true;
    }

    bool stop() {
        const auto current_state = m_state.load(std::memory_order_acquire);
        if (current_state != timer_state_t::ARMED && current_state != timer_state_t::PAUSED) return false;

        itimerspec read_spec{0};
        timerfd_gettime(m_timerfd, &read_spec);
        read_spec.it_value = {0, 0};
        timerfd_settime(m_timerfd, 0, &read_spec, nullptr);

        m_active_object->remove_fd_from_watchlist(m_timerfd);
        m_state.store(timer_state_t::STOPPED, std::memory_order_release);

        return true;
    }

    bool restart() {
        if (m_state.load(std::memory_order_acquire) == timer_state_t::DISARMED) return false;
        m_state.store(timer_state_t::DISARMED, std::memory_order_release);
        return this->start();
    }

    /* used when a timer is no longer needed and the user wants to remove its fd from the active object by calling stop() */
    bool is_running() const {
        const auto current_state = m_state.load(std::memory_order_acquire);
        return (current_state == timer_state_t::ARMED) || (current_state == timer_state_t::PAUSED);
    }

    ~timer_handle_t() {
        if (this->is_running()) {
            this->stop();
        }
    }


    timer_handle_t(const timer_handle_t&) = delete;
    timer_handle_t& operator=(const timer_handle_t&) = delete;

    timer_handle_t(timer_handle_t&&) = delete;
    timer_handle_t& operator=(timer_handle_t&&) = delete;


    /*
        FACTORY MEMBER FUNCTIONS
    */

    template<typename Rep, typename Period>
    static auto _implementation_detail_make_oneshot(active_object_ptr_t active_object, std::chrono::duration<Rep, Period> duration, const active_object_t::Fn& callback) {
        auto timer{std::shared_ptr<timer_handle_t>(new timer_handle_t{active_object, duration, true})};
        timer->m_callback = make_callback(timer->weak_from_this(), callback);
        return timer;
    }

    template<typename Rep, typename Period>
    static auto _implementation_detail_make_oneshot_go(active_object_ptr_t active_object, std::chrono::duration<Rep, Period> duration, const active_object_t::Fn& callback) {
        auto timer = std::shared_ptr<timer_handle_t>(new timer_handle_t{active_object, duration, true});
        timer->m_callback = make_callback(timer->weak_from_this(), callback);
        timer->start();
        return timer;
    }

    template<typename Rep, typename Period>
    static auto _implementation_detail_make_periodic(active_object_ptr_t active_object, std::chrono::duration<Rep, Period> duration, const active_object_t::Fn& callback) {
        auto timer{std::shared_ptr<timer_handle_t>(new timer_handle_t{active_object, duration, false})};
        timer->m_callback = make_callback(timer->weak_from_this(), callback);
        return timer;
    }

    template<typename Rep, typename Period>
    static auto _implementation_detail_make_periodic_go(active_object_ptr_t active_object, std::chrono::duration<Rep, Period> duration, const active_object_t::Fn& callback) {
        auto timer = std::shared_ptr<timer_handle_t>(new timer_handle_t{active_object, duration, false});
        timer->m_callback = make_callback(timer->weak_from_this(), callback);
        timer->start();
        return timer;
    }

private:

    static active_object_t::Fn make_callback(std::weak_ptr<timer_handle_t> weak, active_object_t::Fn cb) {
        return [weak = std::move(weak), cb = std::move(cb)] -> void {
            auto self = weak.lock();
            if (!self) return;

            if (self->m_oneshot) {
                self->m_state.store(timer_state_t::EXPIRED, std::memory_order_release);
            }

            uint64_t drain{0};
            (void) read(self->m_timerfd, &drain, sizeof(drain));

            if (cb) [[likely]] cb();

            if (self->m_oneshot) {
                self->m_active_object->remove_fd_from_watchlist(self->m_timerfd);
            }
        };
    }

    template<typename Rep, typename Period>
    timer_handle_t(active_object_ptr_t active_object, std::chrono::duration<Rep, Period> duration, bool oneshot):
        m_active_object(active_object),
        m_duration(duration),
        m_oneshot(oneshot),
        m_state(timer_state_t::DISARMED)
    {
        if (int timerfd = timerfd_create(CLOCK_BOOTTIME, TFD_CLOEXEC); timerfd == -1) {
            assert(false && "Could not create a timerfd.");
        }
        else m_timerfd = timerfd;


        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(m_duration);
        auto s = std::chrono::duration_cast<std::chrono::seconds>(ns);
        ns -= s;

        itimerspec initial_timer_spec{
            .it_interval{m_oneshot? timespec{0, 0} : timespec{s.count(), ns.count()}},
            .it_value{0, 0}
        };
        timerfd_settime(m_timerfd, 0, &initial_timer_spec, nullptr);
    }

    enum class timer_state_t {
        DISARMED = 0, /* timer not started yet */
        ARMED, /* timer is running */
        PAUSED, /* timer was running but is paused */
        EXPIRED, /* timer expired */
        STOPPED /* timer was started but was then explicitly stopped before expiring */
    };


    active_object_ptr_t m_active_object;

    int m_timerfd;
    const std::chrono::nanoseconds m_duration;
    const bool m_oneshot;

    timespec m_paused_timespec;
    std::atomic<timer_state_t> m_state;

    active_object_t::Fn m_callback;
};


namespace timer_factory {
template<typename Rep, typename Period>
auto make_oneshot_timer(active_object_ptr_t active_object, std::chrono::duration<Rep, Period> duration, const active_object_t::Fn& callback) {
    return timer_handle_t::_implementation_detail_make_oneshot(active_object, duration, callback);
}

template<typename Rep, typename Period>
auto make_oneshot_timer_go(active_object_ptr_t active_object, std::chrono::duration<Rep, Period> duration, const active_object_t::Fn& callback) {
    return timer_handle_t::_implementation_detail_make_oneshot_go(active_object, duration, callback);
}

template<typename Rep, typename Period>
auto make_periodic_timer(active_object_ptr_t active_object, std::chrono::duration<Rep, Period> duration, const active_object_t::Fn& callback) {
    return timer_handle_t::_implementation_detail_make_periodic(active_object, duration, callback);
}

template<typename Rep, typename Period>
auto make_periodic_timer_go(active_object_ptr_t active_object, std::chrono::duration<Rep, Period> duration, const active_object_t::Fn& callback) {
    return timer_handle_t::_implementation_detail_make_periodic_go(active_object, duration, callback);
}
} // namespace timer_factory


} // namespace threesomeip::utils

#endif // _TIMER_HPP