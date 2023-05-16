#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <stack>
#include <unordered_map>
#include <variant>
#include <vector>

#include "error.hpp"
#include "operation.hpp"
#include "operand.hpp"
#include "operation_impl.hpp"

// using VariableId = int64_t;
// using Variable = VariableId;
//
// using FunctionId = int64_t;
// using Function = std::pair <FunctionId, void *>;

Operand simplify(const Operand &);

// TODO: rules for simlpification
// 1 - for constant expressions, unfold strings of commutative operations, but
// this needs a notion of inverse (which + and * -- to some extent -- have)
// 2 - for unresolved expressions, if the operation is mostly invertible, then
// hash subtrees and compare hashes to see if they are equal

template <typename Grouping>
requires std::is_base_of <BinaryGrouping, Grouping>::value
Operand simplify(const Grouping &bg)
{
        if (bg.degenerate())
                return simplify(bg.opda);

        Operand a = simplify(bg.opda);
        Operand b = simplify(bg.opdb);

        // If both are constant, then combine them
        if (a.is_constant() && b.is_constant()) {
                Operand res = opftn(bg.op, { a, b });
                return res;
        }

        // TODO: otherwise fallback to specialized simplification rules

        // Otherwise, return the grouping
        Grouping out = bg;

        out.opda = a;
        out.opdb = b;

        return { new_ <Grouping> (out), Grouping::type };
}

Operand simplify(const Operand &opd)
{
        if (opd.is_constant() || opd.is_blank())
                return opd;

        UnresolvedOperand uo = opd.uo;
        switch (uo.type) {
        case eVariable:
                return opd;
        case eFactor:
                return simplify(uo.as_factor());
        case eTerm:
                return simplify(uo.as_term());
        case eExpression:
                return simplify(uo.as_expression());
        }

        throw std::runtime_error("simplify: unsupported operand type, opd=<" + opd.string() + ">");
}

// TODO

std::optional <Operand> parse(const std::string &);

int main()
{
        // Example parsing
        // std::string input = "1.3/(2/3 + 0.6^9) + 2.6 * 3^7/4.7";
        // TODO: implicit multiplication via conjunction
        std::string input = "2 + 6 + 5 * (x - x) + 6/y * y + 5^(z * z) - 12";
        
        Operand result = parse(input).value();
        std::cout << "result: " << result.string() << std::endl;

        Operand s = simplify(result);
        std::cout << "simplified: " << s.string() << std::endl;
}
