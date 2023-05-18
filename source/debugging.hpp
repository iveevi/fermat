#pragma once

// Standard headers
#include <iostream>

// #define FERMAT_DEBUGGING

namespace fermat {

struct __lout {} lout;

#ifdef FERMAT_DEBUGGING

template <typename T>
constexpr __lout &operator<<(__lout &lout, const T &t)
{
        std::cerr << t;
        return lout;
}

#else

template <typename T>
constexpr __lout &operator<<(__lout &lout, const T &)
{
        return lout;
}

#endif

}
