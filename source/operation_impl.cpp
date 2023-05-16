// Standard headers
#include <cmath>

// Local headers
#include "error.hpp"
#include "operation_impl.hpp"

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

using __operation_function = std::function <Operand (const OperandVector &)>;

// TODO: overload manager to resolve between different groups of items
inline Operand __operation_function_add(const OperandVector &vec)
{
        Operand a = vec[0];
        Operand b = vec[1];

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

inline Operand __operation_function_sub(const OperandVector &vec)
{
        Operand a = vec[0];
        Operand b = vec[1];

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

inline Operand __operation_function_mul(const OperandVector &vec)
{
        Operand a = vec[0];
        Operand b = vec[1];

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

inline Operand __operation_function_div(const OperandVector &vec)
{
        Operand a = vec[0];
        Operand b = vec[1];

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

inline Operand __operation_function_exp(const OperandVector &vec)
{
        Real a;
        Real b;

        if (vec[0].type == eInteger)
                a = static_cast <Real> (vec[0].i);
        else
                a = vec[0].r;

        if (vec[1].type == eInteger)
                b = static_cast <Real> (vec[1].i);
        else
                b = vec[1].r;

        return std::pow(a, b);
}

std::unordered_map <OperationId, __operation_function> g_operation_functions {
        { op_add->id, __operation_function_add },
        { op_sub->id, __operation_function_sub },
        { op_mul->id, __operation_function_mul },
        { op_div->id, __operation_function_div },
        { op_exp->id, __operation_function_exp },
};

Operand opftn(Operation *op, const OperandVector &vec)
{
        return g_operation_functions[op->id](vec);
}
