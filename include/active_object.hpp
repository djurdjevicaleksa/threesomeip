#ifndef _ACTIVE_OBJECT_HPP
#define _ACTIVE_OBJECT_HPP

/*=====*\
 * C++ *
\*=====*/
#include <vector>
#include <sys/poll.h>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>
#include <memory>


namespace threesomeip::utils {


class active_object_t;
using active_object_ptr_t = std::shared_ptr<active_object_t>;


class active_object_t {
public:
    using Fn = std::function<void()>;

    ~active_object_t();

    void add_to_todo_list(Fn);

    bool add_fd_to_readable_watchlist(int, Fn);
    bool add_fd_to_writeable_watchlist(int, Fn);

    bool remove_fd_from_readable_watchlist(int);
    bool remove_fd_from_writeable_watchlist(int);

    void work();

    static active_object_ptr_t _implementation_detail_make_active_object();

private:

    active_object_t();

    bool f_terminate;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<Fn> m_jobs;

    int m_external_stimuli_eventfd;
    std::vector<pollfd> m_polled_fds;

    std::unordered_map<int, Fn> m_fd_readable_job;
    std::unordered_map<int, Fn> m_fd_writeable_job;

    std::thread t_worker;
};


namespace active_object_factory {
inline active_object_ptr_t make_active_object() {
    return active_object_t::_implementation_detail_make_active_object();
}
} // namespace active_object_factory


} // namespace threesomeip::utils

#endif // _ACTIVE_OBJECT_HPP