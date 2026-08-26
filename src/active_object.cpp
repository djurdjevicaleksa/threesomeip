/*=====*\
 * C++ *
\*=====*/
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <cstdint>
#include <ranges>
#include <algorithm>
#include <mutex>
#include <thread>
#include <memory>
#include <cstdlib>
#include <string>

/*=============*\
 * APPLICATION *
\*=============*/
#include <active_object.hpp>


namespace threesomeip::utils {


active_object_t::active_object_t(std::string_view name):
    f_terminate{false},
    m_external_stimuli_eventfd(eventfd(0, EFD_CLOEXEC)),
    m_polled_fds{pollfd{m_external_stimuli_eventfd, static_cast<short int>(POLLIN), static_cast<short int>(0)}},
    m_name(name),
    t_worker(std::bind_front(&active_object_t::work, this))
{}

active_object_t::~active_object_t() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        f_terminate = true;

        /* wake the thread */
        uint64_t flag{1};
        (void) write(m_external_stimuli_eventfd, &flag, sizeof(flag));
    }

    if (t_worker.joinable()) {
        t_worker.join();
    }

    close(m_external_stimuli_eventfd);
}

bool active_object_t::add_to_todo_list(Fn job) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (f_terminate) return false;

        m_jobs.emplace(std::move(job));

        /* wake the thread */
        uint64_t flag{1};
        (void) write(m_external_stimuli_eventfd, &flag, sizeof(flag));
    }
    return true;
}

bool active_object_t::add_fd_to_readable_watchlist(int fd, Fn callback) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (f_terminate) return false;

        bool notify = false;

        auto it = std::ranges::find_if(m_polled_fds, [fd] (const pollfd& pollable) { return pollable.fd == fd; });
        if (it == m_polled_fds.end()) {
            m_polled_fds.emplace_back(fd, /* watched events */ static_cast<short int>(POLLIN), static_cast<short int>(0));
            m_fd_readable_job.emplace(fd, std::move(callback));

            /* actual change happened */
            notify = true;
        }
        else if (! (it->events & POLLIN)) {
            it->events |= POLLIN;
            m_fd_readable_job.emplace(fd, std::move(callback));

            /* subscribed to something, but not POLLIN; actual change */
            notify = true;
        }
        else {
            /* no change observed */
            return false;
        }

        if (notify) {
            /* wake the thread */
            uint64_t flag{1};
            (void) write(m_external_stimuli_eventfd, &flag, sizeof(flag));
        }

        return true;
    }
}

bool active_object_t::add_fd_to_writeable_watchlist(int fd, Fn callback) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (f_terminate) return false;

        bool notify = false;

        auto it = std::ranges::find_if(m_polled_fds, [fd] (const pollfd& pollable) { return pollable.fd == fd; });
        if (it == m_polled_fds.end()) {
            m_polled_fds.emplace_back(fd, /* watched events */ static_cast<short int>(POLLOUT), static_cast<short int>(0));
            m_fd_writeable_job.emplace(fd, std::move(callback));

            /* actual change happened */
            notify = true;
        }
        else if (! (it->events & POLLOUT)) {
            it->events |= POLLOUT;
            m_fd_writeable_job.emplace(fd, std::move(callback));

            /* subscribed to something, but not POLLOUT; actual change */
            notify = true;
        }
        else {
            /* no change observed */
            return false;
        }

        if (notify) {
            /* wake the thread */
            uint64_t flag{1};
            (void) write(m_external_stimuli_eventfd, &flag, sizeof(flag));
        }

        return true;
    }
}

bool active_object_t::remove_fd_from_readable_watchlist(int fd) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (f_terminate) return false;

        auto it = std::ranges::find_if(m_polled_fds, [fd] (const pollfd& pollable) { return pollable.fd == fd; });
        if (it == m_polled_fds.end()) {
            /* nothing to remove */
            return false;
        }
        else if (! (it->events & POLLIN)) {
            /* not subscribed to POLLIN */
            return false;
        }
        else {
            /* is subscribed to POLLIN */
            it->events &= ~POLLIN;
            m_fd_readable_job.erase(fd);

            if (! (it->events & POLLOUT)) {
                /* if not subscribed to POLLOUT after unsubscribing from POLLIN, then remove */
                std::erase_if(m_polled_fds, [fd] (const pollfd& pollable) { return pollable.fd == fd; });
            }

            /* wake the thread */
            uint64_t flag{1};
            (void) write(m_external_stimuli_eventfd, &flag, sizeof(flag));
            return true;
        }
    }
}

bool active_object_t::remove_fd_from_writeable_watchlist(int fd) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (f_terminate) return false;

        auto it = std::ranges::find_if(m_polled_fds, [fd] (const pollfd& pollable) { return pollable.fd == fd; });
        if (it == m_polled_fds.end()) {
            /* nothing to remove */
            return false;
        }
        else if (! (it->events & POLLOUT)) {
            /* not subscribed to POLLOUT */
            return false;
        }
        else {
            /* is subscribed to POLLOUT */
            it->events &= ~POLLOUT;
            m_fd_writeable_job.erase(fd);

            if (! (it->events & POLLIN)) {
                /* if not subscribed to POLLIN after unsubscribing from POLLOUT, then remove */
                std::erase_if(m_polled_fds, [fd] (const pollfd& pollable) { return pollable.fd == fd; });
            }

            /* wake the thread */
            uint64_t flag{1};
            (void) write(m_external_stimuli_eventfd, &flag, sizeof(flag));
            return true;
        }
    }
}

void active_object_t::work() {
    std::unique_lock<std::mutex> lock(m_mutex);

    while (!f_terminate) {
        /* first serve fd-related events */

        /* snapshot polled fds */
        auto polled_fds_snapshot{m_polled_fds};

        lock.unlock();
        if (poll(polled_fds_snapshot.data(), polled_fds_snapshot.size(), -1) < 0) {
            lock.lock();
            continue;
        }
        lock.lock();

        const bool stimuli_readable = std::ranges::any_of(polled_fds_snapshot, [this] (const pollfd& polled_fd) {
            return (polled_fd.fd == m_external_stimuli_eventfd) && (polled_fd.revents & POLLIN);
        });
        if (stimuli_readable) {
            /* drain eventfd */
            uint64_t flag{0};
            (void) read(m_external_stimuli_eventfd, &flag, sizeof(flag));
        }

        std::ranges::for_each(polled_fds_snapshot
            | std::views::filter([this] (const pollfd& polled_fd) { return (polled_fd.revents & POLLIN) && (polled_fd.fd != m_external_stimuli_eventfd); }),

            [this, &lock] (const pollfd& readable_polled_fd) {
                if (m_fd_readable_job.contains(readable_polled_fd.fd) && m_fd_readable_job.at(readable_polled_fd.fd)) {
                    /* explicitly copy because it can be removed in the unlocked window */
                    const auto callback = m_fd_readable_job.at(readable_polled_fd.fd);
                    lock.unlock();
                    callback();
                    lock.lock();
                }
            }
        );

        std::ranges::for_each(polled_fds_snapshot
            | std::views::filter([this] (const pollfd& polled_fd) { return (polled_fd.revents & POLLOUT); }),

            [this, &lock] (const pollfd& writeable_polled_fd) {
                if (m_fd_writeable_job.contains(writeable_polled_fd.fd) && m_fd_writeable_job.at(writeable_polled_fd.fd)) {
                    /* explicitly copy because it can be removed in the unlocked window */
                    const auto callback = m_fd_writeable_job.at(writeable_polled_fd.fd);
                    lock.unlock();
                    callback();
                    lock.lock();
                }
            }
        );

        while (!m_jobs.empty()) {
            const auto job = std::move(m_jobs.front());
            m_jobs.pop();

            lock.unlock();
            if (job) [[likely]] {
                job();
            }
            lock.lock();
        }
    }
}

active_object_ptr_t active_object_t::_implementation_detail_make_active_object(std::string_view name) {
    return std::shared_ptr<active_object_t>(new active_object_t(name));
}

std::string_view active_object_t::get_name() const {
    return m_name;
}

} // namespace threesomeip::utils