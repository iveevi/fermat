#pragma once

#include <termcolor/termcolor.hpp>

inline void fatal_error(const std::string &header, const std::string &str)
{
        std::cerr << termcolor::bold << termcolor::magenta
                << "[E] " << header << ": " << termcolor::reset
                << str << std::endl;
        exit(1);
}

// TODO: format string with %v for values?
inline void warning(const std::string &header, const std::string &str)
{
        std::cerr << termcolor::bold << termcolor::yellow
                << "[W] " << header << ": " << termcolor::reset
                << str << std::endl;
}
