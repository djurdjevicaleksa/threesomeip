#ifndef _RUNTIME_STUB_HPP
#define _RUNTIME_STUB_HPP

/*=====*\
 * C++ *
\*=====*/

/*=============*\
 * APPLICATION *
\*=============*/

/*===========*\
 * 3RD PARTY *
\*===========*/


namespace threesomeip::runtime {


class runtime_stub_t {
public:

    int parseRequest(std::span<std::byte> data);

private:

    int m_ud_socket;
};


} // namespace threesomeip::runtime

#endif // _RUNTIME_STUB_HPP