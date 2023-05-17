#pragma once

// Standard headers
#include <functional>

// Local headers
#include "operation.hpp"
#include "operand.hpp"

using __operation_function = std::function <Operand (const std::vector <Operand> &)>;

OperationId getid();
Operand opftn(const Operation *, const std::vector <Operand> &);

extern Operation *op_add;
extern Operation *op_sub;
extern Operation *op_mul;
extern Operation *op_div;
extern Operation *op_exp;

extern std::vector <Operation> g_operations;
extern std::unordered_map <OperationId, __operation_function> g_operation_functions;

struct CommutativeInverse {
        OperationId id = -1;
        std::function <Operand (const Operand &)> transformation;
};

extern std::unordered_map <OperationId, CommutativeInverse> commutative_inverses;
