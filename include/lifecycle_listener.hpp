#ifndef _LIFECYCLE_LISTENER_HPP
#define _LIFECYCLE_LISTENER_HPP

/*=====*\
 * C++ *
\*=====*/
#include <atomic>


namespace threesomeip::ipc {

class lifecycle_listener_t {
public:

    lifecycle_listener_t(): m_stage(lifecycle_stage_t::INIT) {}

    bool is_alive() const {
        return lifecycle_stage_t::ALIVE == m_stage.load(std::memory_order_acquire);
    }

    bool is_dead() const {
        return lifecycle_stage_t::DEAD == m_stage.load(std::memory_order_acquire);
    }

protected:

    bool to_alive() {
        if (lifecycle_stage_t::INIT == m_stage.load(std::memory_order_acquire)) {
            m_stage.store(lifecycle_stage_t::ALIVE, std::memory_order_release);
            this->on_alive();
            return true;
        } else return false;
    }

    void to_dead() {
        if (lifecycle_stage_t::DEAD != m_stage.load(std::memory_order_acquire)) {
            m_stage.store(lifecycle_stage_t::DEAD, std::memory_order_release);
            this->on_dead();
        }
    }

    virtual void on_alive() {}

    virtual void on_dead() {}

private:

    enum class lifecycle_stage_t {
        INIT = 0,
        ALIVE,
        DEAD
    };

    std::atomic<lifecycle_stage_t> m_stage;
};

} // namespace threesomeip::ipc

#endif // _LIFECYCLE_LISTENER_HPP