#pragma once

// Standard headers
#include <optional>
#include <string>

// Local headers
#include "operand.hpp"

namespace fermat {

std::optional <Operand> parse(const std::string &);

}
