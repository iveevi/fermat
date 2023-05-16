#pragma once

#include <termcolor.hpp>

// TODO: format string with %v for values?
inline void warning(const std::string &header, const std::string &str)
{
        std::cerr << termcolor::bold << termcolor::yellow
                << "[W] " << header << ": " << termcolor::reset
                << str << std::endl;
}
