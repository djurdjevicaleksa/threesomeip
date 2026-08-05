#ifndef _LIFECYCLE_LISTENER_HPP
#define _LIFECYCLE_LISTENER_HPP


namespace threesomeip::ipc {

class lifecycle_listener_t {
public:

    lifecycle_listener_t(): m_stage(lifecycle_stage_t::INIT) {}

    bool is_alive() const {
        return m_stage == lifecycle_stage_t::ALIVE;
    }

    bool is_dead() const {
        return m_stage == lifecycle_stage_t::DEAD;
    }

protected:

    bool to_alive() {
        if (lifecycle_stage_t::INIT == m_stage) {
            m_stage = lifecycle_stage_t::ALIVE;
            return true;
        } else return false;
    }

    void to_dead() {
        m_stage = lifecycle_stage_t::DEAD;
    }

    virtual void on_alive() const {}

    virtual void on_dead() const {}

private:

    enum class lifecycle_stage_t {
        INIT = 0,
        ALIVE,
        DEAD
    };

    lifecycle_stage_t m_stage;
};

} // namespace threesomeip::ipc

#endif // _LIFECYCLE_LISTENER_HPP