#pragma once

// Local headers
#include "operand.hpp"

namespace fermat {

Operand simplify(const Operand &);

// TODO: is this needed?
// std::vector <Operand> unfold(const BinaryGrouping &bg);
// Operand fold(Operation *op, const std::vector <Operand> &opds);

}
