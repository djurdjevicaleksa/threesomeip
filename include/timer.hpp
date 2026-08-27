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
#include <cstdlib>
#include <mutex>

/*=============*\
 * APPLICATION *
\*=============*/
#include <active_object.hpp>


namespace threesomeip::utils {


class timer_handle_t: public std::enable_shared_from_this<timer_handle_t> {
public:

    bool start() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return this->_start_impl();
    }

    bool pause() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return this->_pause_impl();
    }

    bool resume() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return this->_resume_impl();
    }

    bool stop() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return this->_stop_impl();
    }

    bool restart() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return this->_restart_impl();
    }

    bool is_running() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return (m_state == timer_state_t::ARMED) || (m_state == timer_state_t::PAUSED);
    }

    template<typename Rep, typename Period>
    bool reschedule(std::chrono::duration<Rep, Period> new_duration) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return this->_reschedule_impl<Rep, Period>(new_duration);
    }

    ~timer_handle_t() {
        this->stop();
        (void) close(m_fd);
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

    bool _start_impl() {
        if (m_state == timer_state_t::DISARMED) {
            (void) m_active_object->add_fd_to_readable_watchlist(m_fd, m_callback);

            m_current_timer_spec.it_value = m_current_timer_spec.it_interval;
            timerfd_settime(m_fd, 0, &m_current_timer_spec, nullptr);
            m_state = timer_state_t::ARMED;
            return true;
        }
        else return false;
    }

    bool _pause_impl() {
        if (m_state == timer_state_t::ARMED) {
            timerfd_gettime(m_fd, &m_current_timer_spec);

            itimerspec paused_spec{
                .it_interval{m_current_timer_spec.it_interval},
                .it_value{0, 0}
            };
            timerfd_settime(m_fd, 0, &paused_spec, nullptr);

            m_state = timer_state_t::PAUSED;
            return true;
        }
        else return false;
    }

    bool _resume_impl() {
        if (m_state == timer_state_t::PAUSED) {
            timerfd_settime(m_fd, 0, &m_current_timer_spec, nullptr);
            m_state = timer_state_t::ARMED;
            return true;
        }
        else return false;
    }

    bool _stop_impl() {
        if (m_state == timer_state_t::ARMED || m_state == timer_state_t::PAUSED) {
            (void) m_active_object->remove_fd_from_readable_watchlist(m_fd);

            m_current_timer_spec.it_value = {0, 0};
            timerfd_settime(m_fd, 0, &m_current_timer_spec, nullptr);
            m_state = timer_state_t::STOPPED;
            return true;
        }
        else return false;
    }

    bool _restart_impl() {
        /* enforce start() for fresh timers and restart() for others; mainly expired, stopped, paused */
        if (m_state != timer_state_t::DISARMED) {
            /* could be removed from the watch list here but would immediately get re-added */
            /* relying on the fact that adding fds to the active objects is idempotent */
            m_state = timer_state_t::DISARMED;
            return this->_start_impl();
        }
        else return false;
    }

    template<typename Rep, typename Period>
    bool _reschedule_impl(std::chrono::duration<Rep, Period> new_duration) {
        if (new_duration != m_duration) {
            /* any state allowed */
            (void) this->_stop_impl();

            m_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(new_duration);

            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(m_duration);
            auto s = std::chrono::duration_cast<std::chrono::seconds>(ns);
            ns -= s;

            m_current_timer_spec.it_interval = {s.count(), ns.count()};
            timerfd_settime(m_fd, 0, &m_current_timer_spec, nullptr);
            m_state = timer_state_t::DISARMED;

            return true;
        }
        /* reschedule will take place; we either do stuff or not, outcome the same, thats why theres 2 'true' */
        else return true;
    }

    static active_object_t::Fn make_callback(std::weak_ptr<timer_handle_t> weak, active_object_t::Fn cb) {
        return [weak = std::move(weak), cb = std::move(cb)] -> void {
            /* check if the callback is running after the timer was destroyed */
            auto self = weak.lock();
            if (!self) return;

            std::unique_lock<std::mutex> lock(self->m_mutex);
            if (self->m_oneshot) {
                (void) self->m_active_object->remove_fd_from_readable_watchlist(self->m_fd);
                self->m_state = timer_state_t::EXPIRED;
            }

            uint64_t drain{0};
            (void) read(self->m_fd, &drain, sizeof(drain));

            lock.unlock();
            if (cb) [[likely]] cb();
        };
    }

    template<typename Rep, typename Period>
    timer_handle_t(active_object_ptr_t active_object, std::chrono::duration<Rep, Period> duration, bool oneshot):
        m_active_object(active_object),
        m_fd(-1),
        m_oneshot(oneshot),
        m_duration(std::chrono::duration_cast<std::chrono::nanoseconds>(duration)),
        m_state(timer_state_t::DISARMED)
    {
        /* create the timerfd */
        if (int timerfd = timerfd_create(CLOCK_BOOTTIME, TFD_CLOEXEC); timerfd == -1) {
            assert(false && "Could not create a timerfd.");
        }
        else m_fd = timerfd;

        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(m_duration);
        auto s = std::chrono::duration_cast<std::chrono::seconds>(ns);
        ns -= s;

        m_current_timer_spec = {
            .it_interval{m_oneshot? timespec{0, 0} : timespec{s.count(), ns.count()}},
            .it_value{0, 0}
        };
        timerfd_settime(m_fd, 0, &m_current_timer_spec, nullptr);
    }

    enum class timer_state_t {
        DISARMED = 0, /* timer not started yet */
        ARMED, /* timer is running */
        PAUSED, /* timer was running but is paused */
        EXPIRED, /* timer expired */
        STOPPED /* timer was started but was then explicitly stopped before expiring */
    };


    active_object_ptr_t m_active_object;

    int m_fd;
    const bool m_oneshot;
    std::chrono::nanoseconds m_duration;

    itimerspec m_current_timer_spec;
    timespec m_paused_timespec;

    timer_state_t m_state;
    mutable std::mutex m_mutex;

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