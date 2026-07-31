/*=====*\
 * C++ *
\*=====*/

/*=============*\
 * APPLICATION *
\*=============*/
#include <configurable.hpp>


namespace threesomeip {


class runtime_t: private configurable_t {
public:

    runtime_t(const char*);

private:

    int m_ud_socket;
};


} // namespace threesomeip