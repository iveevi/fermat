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

bool is_constant(const Operand &opd)
{
        // Constant or vacuously true
        if (opd.is_constant() || opd.is_blank())
                return true;

        // Unresolved operand
        if (opd.uo.type == eBinaryGrouping) {
                BinaryGrouping bg = opd.uo.as_binary_grouping();
                if (!is_constant(bg.opda))
                        return false;

                if (!bg.degenerate() && !is_constant(bg.opdb))
                        return false;

                return true;
        }

        return false;
}

std::vector <Operand> unfold(const BinaryGrouping &bg)
{
        assert(bg.op->classifications & eOperationCommutative);

        std::vector <Operand> items;
        std::stack <Operand> stack;

        stack.push(bg.opda);
        if (!bg.degenerate())
                stack.push(bg.opdb);

        CommutativeInverse ci;
        if (commutative_inverses.find(bg.op->id) != commutative_inverses.end())
                ci = commutative_inverses[bg.op->id];
        else
                warning("unfold", "no commutative inverse for operation " + bg.op->lexicon);

        while (!stack.empty()) {
                Operand opd = stack.top();
                stack.pop();

                // NOTE: Ignoring blank operands
                if (opd.is_constant()) {
                        items.push_back(opd);
                        continue;
                }

                UnresolvedOperand uo = opd.uo;
                switch (uo.type) {
                case eVariable:
                        items.push_back(opd);
                        break;
                case eBinaryGrouping:
                        BinaryGrouping bg_nested = uo.as_binary_grouping();
                        if (bg_nested.op == bg.op) {
                                stack.push(bg_nested.opda);

                                if (!bg_nested.degenerate())
                                        stack.push(bg_nested.opdb);
                        } else if (bg_nested.op->id == ci.id) {
                                stack.push(bg_nested.opda);

                                if (!bg_nested.degenerate())
                                        stack.push(ci.transformation(bg_nested.opdb));
                        } else {
                                items.push_back(opd);
                        }

                        break;
                }
        }

        return items;
}

Operand fold(Operation *op, const std::vector <Operand> &opds)
{
        // Makes sure that we can fold the operation
        // in a binary partition fashion
        assert(op->classifications & eOperationCommutative);

        std::vector <Operand> current = opds;
        std::vector <Operand> next;

        while (current.size() > 1) {
                for (size_t i = 0; i < current.size(); i += 2) {
                        if (i + 1 < current.size()) {
                                Operand a = current[i];
                                Operand b = current[i + 1];

                                if (a.is_constant() && b.is_constant()) {
                                        Operand res = opftn(op, { a, b });
                                        next.push_back(res);
                                } else {
                                        BinaryGrouping bg = { op, a, b };
                                        next.push_back({ new_ <BinaryGrouping> (bg), eBinaryGrouping });
                                }
                        } else {
                                next.push_back(current[i]);
                        }
                }

                assert(next.size() < current.size());
                current = next;
                next.clear();
        }

        return current[0];
}

Operand __simplificiation_fold(const BinaryGrouping &bg)
{
        assert(bg.op->classifications & eOperationCommutative);

        std::vector <Operand> items = unfold(bg);
        // return fold(bg.op, items);

        std::vector <Operand> constants;
        std::vector <Operand> unresolved;

        for (const Operand &opd : items) {
                if (is_constant(opd))
                        constants.push_back(opd);
                else
                        unresolved.push_back(opd);
        }

        Operand constant;
        if (constants.size() > 0) {
                constant = simplify(constants[0]);
                assert(constant.is_constant());

                for (int i = 1; i < constants.size(); ++i) {
                        Operand opd = simplify(constants[i]);
                        assert(opd.is_constant());

                        constant = opftn(bg.op, { constant, opd });
                }
        }

        Operand unresolved_folded = fold(bg.op, unresolved);

        assert(!constant.is_blank() || !unresolved_folded.is_blank());
        if (constant.is_blank())
                return unresolved_folded;
        if (unresolved_folded.is_blank())
                return constant;

        return Operand {
                new_ <BinaryGrouping> (bg.op, constant, unresolved_folded),
                eBinaryGrouping
        };
}

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

        // TODO: top down simplification or bottom up simplification?
        // bottom down is simpler/faster, but top down is more powerful
        // and we may be able to manipulate the tree in a more meaningful
        // way

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
        std::string input = "2 + 6 + 5 * (x - x) + 6/y * y + 5^(z * z) - 12";
        
        Operand result = parse(input).value();
        std::cout << "result: " << result.string() << std::endl;

        // Operand s = simplify(result);
        // std::cout << "simplified: " << s.string() << std::endl;

        const BinaryGrouping &e = result.uo.as_binary_grouping();

        auto items = unfold(e);
        std::cout << "unfolded [" << items.size() << "]:\n";
        for (const Operand &opd : items) {
                std::cout << "Constant? " << is_constant(opd) << "\n";
                std::cout << opd.pretty(1) << std::endl;
        }

        Operand f = fold(e.op, items);
        std::cout << "folded: " << f.string() << std::endl;
        std::cout << f.pretty(1) << std::endl;

        Operand fs = __simplificiation_fold(e);
        std::cout << "simplified folded: " << fs.string() << std::endl;
        std::cout << fs.pretty(1) << std::endl;

        // TODO:
        // PartiallyEvaluated pe = partially_evaluate(e);
        // a = pe(1, 2, 3) or pe({"x": 1, "y": 2, "z": 3})
        // default for no dict is alphabetical order
}
