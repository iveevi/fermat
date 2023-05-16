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

// TODO: To define the property of operations, create group and ring abstractions?
// struct Group {};

// NOTE: using operation instead of grouping as anchor to generalize outside of
// binary groupings...
// std::vector <Operand> unfold(Operation *parent, const Operand &opd)
// {
// }

Operand simplify(const BinaryGrouping &bg)
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
        BinaryGrouping out = bg;

        out.opda = a;
        out.opdb = b;

        return { new_ <BinaryGrouping> (out), eBinaryGrouping };
}

Operand simplify(const Operand &opd)
{
        if (opd.is_constant() || opd.is_blank())
                return opd;

        UnresolvedOperand uo = opd.uo;
        switch (uo.type) {
        case eVariable:
                return opd;
        case eBinaryGrouping:
                return simplify(uo.as_binary_grouping());
        }

        throw std::runtime_error("simplify: unsupported operand type, opd=<" + opd.string() + ">");
}

// TODO

std::optional <Operand> parse(const std::string &);

int main()
{
        // Example parsing
        // TODO: implicit multiplication via conjunction
        std::string input1 = "2 + 6 + 5 * (x - x) + 6/y * y + 5^(z * z) - 12";
        
        Operand result1 = parse(input1).value();
        std::cout << "result: " << result1.string() << std::endl;

        Operand s1 = simplify(result1);
        std::cout << "simplified: " << s1.string() << std::endl;
        
        std::string input2 = "1.3/(2/3 + 0.6^9) + 2.6 * 3^7/4.7";
        
        Operand result2 = parse(input2).value();
        std::cout << "result: " << result2.string() << std::endl;

        Operand s2 = simplify(result2);
        std::cout << "simplified: " << s2.string() << std::endl;
}
