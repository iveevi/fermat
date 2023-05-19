#pragma once

// Standard headers
#include <iostream>

// #define FERMAT_DEBUGGING

namespace fermat {

struct logging_stream {} lout;

#ifdef FERMAT_DEBUGGING

template <typename T>
constexpr logging_stream &operator<<(logging_stream &lout, const T &t)
{
        std::cerr << t;
        return lout;
}

#else

template <typename T>
constexpr logging_stream &operator<<(logging_stream &lout, const T &)
{
        return lout;
}

#endif

}
