#ifndef _RUNTIME_HPP
#define _RUNTIME_HPP

/*=====*\
 * C++ *
\*=====*/

/*=============*\
 * APPLICATION *
\*=============*/
#include <configurable.hpp>


namespace threesomeip {

#error "Implement destructor to close socket"

class runtime_t: private configurable_t {
public:

    runtime_t(const char*);
    ~runtime_t();

    void run();

private:

    int m_ud_socket;
};


} // namespace threesomeip

#endif // _RUNTIME_HPP