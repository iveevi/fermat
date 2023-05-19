// Standard headers
#include <cmath>

// Local headers
#include "error.hpp"
#include "operand.hpp"
#include "operation_impl.hpp"

namespace fermat {

OperationId getid() {
        static OperationId id = 0;
        return id++;
}

std::vector <Operation> g_operations {
        {
                getid(), "+", ePriorityAdditive,
                eOperationCommutative | eOperationAssociative | eOperationDistributive | eOperationLinear,
        },

        {
                getid(), "-", ePriorityAdditive,
                eOperationLinear,
        },

        {
                getid(), "*", ePriorityMultiplicative,
                eOperationCommutative | eOperationAssociative | eOperationDistributive | eOperationLinear,
        },

        {
                getid(), "/", ePriorityMultiplicative,
                eOperationLinear,
        },

        {
                getid(), "^", ePriorityExponential,
                eOperationNone,
        },
};

Operation *op_add = &g_operations[0];
Operation *op_sub = &g_operations[1];
Operation *op_mul = &g_operations[2];
Operation *op_div = &g_operations[3];
Operation *op_exp = &g_operations[4];

// TODO: overload manager to resolve between different groups of items
// TODO: detail namespace
inline Operand operation_function_add(const Operand &a, const Operand &b)
{
        Operand res;
        if (a.type == eInteger && b.type == eInteger)
                res = Operand { a.i + b.i };
        else if (a.type == eInteger && b.type == eReal)
                res = Operand { static_cast <Real> (a.i) + b.r };
        else if (a.type == eReal && b.type == eInteger)
                res = Operand { a.r + static_cast <Real> (b.i) };
        else if (a.type == eReal && b.type == eReal)
                res = Operand { a.r + b.r };
        else
                throw std::runtime_error("add: unsupported operand types "
                        + std::to_string(a.type) + " and " + std::to_string(b.type));

        return res;
}

inline Operand operation_function_sub(const Operand &a, const Operand &b)
{
        Operand res;
        if (a.type == eInteger && b.type == eInteger)
                res = Operand { a.i - b.i };
        else if (a.type == eInteger && b.type == eReal)
                res = Operand { static_cast <Real> (a.i) - b.r };
        else if (a.type == eReal && b.type == eInteger)
                res = Operand { a.r - static_cast <Real> (b.i) };
        else if (a.type == eReal && b.type == eReal)
                res = Operand { a.r - b.r };
        else
                throw std::runtime_error("sub: unsupported operand types "
                        + std::to_string(a.type) + " and " + std::to_string(b.type));

        return res;
}

inline Operand operation_function_mul(const Operand &a, const Operand &b)
{
        Operand res;
        if (a.type == eInteger && b.type == eInteger)
                res = Operand { a.i * b.i };
        else if (a.type == eInteger && b.type == eReal)
                res = Operand { static_cast <Real> (a.i) * b.r };
        else if (a.type == eReal && b.type == eInteger)
                res = Operand { a.r * static_cast <Real> (b.i) };
        else if (a.type == eReal && b.type == eReal)
                res = Operand { a.r * b.r };
        else
                throw std::runtime_error("mul: unsupported operand types "
                        + std::to_string(a.type) + " and " + std::to_string(b.type));

        return res;
}

inline Operand operation_function_div(const Operand &a, const Operand &b)
{
        if (b.is_zero())
                warning("div", "division by zero");

        Operand res;
        if (a.type == eInteger && b.type == eInteger) {
                Integer ia = a.i;
                Integer ib = b.i;

                if (ia % ib == 0)
                        res = Operand { a.i / b.i };
                else
                        res = Operand { static_cast <Real> (a.i) / b.i };
        } else if (a.type == eInteger && b.type == eReal) {
                res = Operand { static_cast <Real> (a.i) / b.r };
        } else if (a.type == eReal && b.type == eInteger) {
                res = Operand { a.r / static_cast <Real> (b.i) };
        } else if (a.type == eReal && b.type == eReal) {
                res = Operand { a.r / b.r };
        } else {
                throw std::runtime_error("div: unsupported operand types "
                        + std::to_string(a.type) + " and " + std::to_string(b.type));
        }

        return res;
}

inline Operand operation_function_exp(const Operand &opda, const Operand &opdb)
{
        Real a;
        Real b;

        if (opda.type == eInteger)
                a = static_cast <Real> (opda.i);
        else
                a = opda.r;

        if (opdb.type == eInteger)
                b = static_cast <Real> (opdb.i);
        else
                b = opdb.r;

        return std::pow(a, b);
}

std::unordered_map <OperationId, binary_operation> g_operation_functions {
        { op_add->id, operation_function_add },
        { op_sub->id, operation_function_sub },
        { op_mul->id, operation_function_mul },
        { op_div->id, operation_function_div },
        { op_exp->id, operation_function_exp },
};

Operand opftn(const Operation *op, const Operand &opda, const Operand &opdb)
{
        return g_operation_functions[op->id](opda, opdb);
}

std::unordered_map <OperationId, CommutativeInverse> commutative_inverses {
        { op_add->id, {
                op_sub->id,
                [](const Operand &opd) {
                        return Operand {
                                // TODO: just return -opd
                                new_ <BinaryGrouping> (op_mul, -1ll, opd),
                                eBinaryGrouping
                        };
                }
        }},
        { op_mul->id, {
                op_div->id,
                // TODO: ijnstead have a funciton that applies an inverse
                // For example, if 2x/4 should be kept as a rational, etc...
                [](const Operand &opd) {
                        return Operand {
                                new_ <BinaryGrouping> (op_exp, opd, -1ll),
                                eBinaryGrouping
                        };

                        // TODO: make sure opd is constant?
                        
                        // Real x = (opd.type == eReal) ? opd.r : static_cast <Real> (opd.i);
                        // if (x == 0.0)
                        //         warning("inverse", "division by zero");
                        //
                        // return 1.0/x;
                }
        }},
};

// Operator overloads
// NOTE: Generates a binary grouping, but simplification must be done later
Operand operator+(const Operand &opda, const Operand &opdb)
{
        return Operand {
                new_ <BinaryGrouping> (op_add, opda, opdb),
                eBinaryGrouping
        };
}

Operand operator-(const Operand &opda, const Operand &opdb)
{
        return Operand {
                new_ <BinaryGrouping> (op_sub, opda, opdb),
                eBinaryGrouping
        };
}

Operand operator*(const Operand &opda, const Operand &opdb)
{
        return Operand {
                new_ <BinaryGrouping> (op_mul, opda, opdb),
                eBinaryGrouping
        };
}

Operand operator/(const Operand &opda, const Operand &opdb)
{
        return Operand {
                new_ <BinaryGrouping> (op_div, opda, opdb),
                eBinaryGrouping
        };
}

Operand operator^(const Operand &opda, const Operand &opdb)
{
        return Operand {
                new_ <BinaryGrouping> (op_exp, opda, opdb),
                eBinaryGrouping
        };
}

}
