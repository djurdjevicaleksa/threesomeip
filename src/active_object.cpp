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

/*=============*\
 * APPLICATION *
\*=============*/
#include <active_object.hpp>


namespace threesomeip::utils {


active_object_t::active_object_t():
    f_terminate{false},
    m_external_stimuli_eventfd(eventfd(0, EFD_CLOEXEC)),
    m_polled_fds{pollfd{m_external_stimuli_eventfd, static_cast<short int>(POLLIN), static_cast<short int>(0)}},
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
}

void active_object_t::add_to_todo_list(Fn job) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_jobs.emplace(std::move(job));

        /* wake the thread */
        uint64_t flag{1};
        (void) write(m_external_stimuli_eventfd, &flag, sizeof(flag));
    }
}

void active_object_t::add_fd_to_watchlist(int fd, Fn callback) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_polled_fds.emplace_back(fd, /* watched events */ static_cast<short int>(POLLIN), static_cast<short int>(0));
        m_fd_job.emplace(fd, std::move(callback));

        /* wake the thread */
        uint64_t flag{1};
        (void) write(m_external_stimuli_eventfd, &flag, sizeof(flag));
    }
}

void active_object_t::remove_fd_from_watchlist(int fd) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::erase_if(m_polled_fds, [fd] (const pollfd& polled_fd) {
            return polled_fd.fd == fd;
        });
        m_fd_job.erase(fd);

        /* wake the thread */
        uint64_t flag{1};
        (void) write(m_external_stimuli_eventfd, &flag, sizeof(flag));
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
                if (m_fd_job.contains(readable_polled_fd.fd) && m_fd_job[readable_polled_fd.fd]) {
                    /* explicitly copy because it can be removed in the unlocked window */
                    const auto callback = m_fd_job[readable_polled_fd.fd];
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

active_object_ptr_t active_object_t::_implementation_detail_make_active_object() {
    return std::shared_ptr<active_object_t>(new active_object_t());
}

} // namespace threesomeip::utils