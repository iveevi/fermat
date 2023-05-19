#pragma once

// Standard headers
#include <functional>

// Local headers
#include "operation.hpp"
#include "operand.hpp"

namespace fermat {

using binary_operation = std::function <Operand (const Operand &, const Operand &)>;

OperationId getid();
Operand opftn(const Operation *, const Operand &, const Operand &);

extern Operation *op_add;
extern Operation *op_sub;
extern Operation *op_mul;
extern Operation *op_div;
extern Operation *op_exp;

extern std::vector <Operation> g_operations;
extern std::unordered_map <OperationId, binary_operation> g_operation_functions;

struct CommutativeInverse {
        OperationId id = -1;
        std::function <Operand (const Operand &)> transformation;
};

extern std::unordered_map <OperationId, CommutativeInverse> commutative_inverses;

// Operator overloading for operands
Operand operator+(const Operand &, const Operand &);
Operand operator-(const Operand &, const Operand &);
Operand operator*(const Operand &, const Operand &);
Operand operator/(const Operand &, const Operand &);
Operand operator^(const Operand &, const Operand &);

}
