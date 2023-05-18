// Standard headers
#include <stack>
#include <limits>

// Local headers
#include "simplify.hpp"
#include "operation_impl.hpp"
#include "error.hpp"
#include "debugging.hpp"

namespace fermat {

// TODO: rules for simlpification
// 1 - for constant expressions, unfold strings of commutative operations, but
// this needs a notion of inverse (which + and * -- to some extent -- have)
// 2 - for unresolved expressions, if the operation is mostly invertible, then
// hash subtrees and compare hashes to see if they are equal

// TODO: To define the property of operations, create group and ring abstractions?
// struct Group {};

// using ExpressionHash = int64_t; // NOTE: first bit is always tractable signage
// NOTE: we instead need some parity information wrt an operation
// of the expression; e.g. for 2x is 0 and for -2x is 1

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

// TODO: maybe these are also detail namespaces?
std::vector <Operand> unfold(const Operation *focus, const BinaryGrouping &bg)
{
        assert(focus->classifications & eOperationCommutative);

        std::vector <Operand> items;

        struct stack_item {
                Operand opd;
                bool canon_inverse = true;
        };

        std::stack <stack_item > stack;

        Operand origin = Operand { new_ <BinaryGrouping> (bg), eBinaryGrouping };
        stack.push({ origin, true });

        CommutativeInverse ci;
        if (commutative_inverses.find(focus->id) != commutative_inverses.end())
                ci = commutative_inverses[focus->id];
        else
                warning("unfold", "no recorded inverse for commutative operation " + focus->lexicon);

        while (!stack.empty()) {
                stack_item si = stack.top();
                Operand opd = si.opd;
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
                        if (bg_nested.op->id == focus->id) {
                                stack.push({ bg_nested.opda, si.canon_inverse });

                                if (!bg_nested.degenerate())
                                        stack.push({ bg_nested.opdb, si.canon_inverse });
                        } else if (bg_nested.op->id == ci.id) {
                                // TODO: for nested inverse commutative
                                // operations, the order of B or A switches
                                // lout << "[*]  Inverse branch: " << bg_nested.string() << "\n";

                                if (!si.canon_inverse)
                                        std::swap(bg_nested.opda, bg_nested.opdb);

                                // NOTE: The canon inverse corretion is not that
                                // important in practice...
                                stack.push({ bg_nested.opda, si.canon_inverse });
                                if (!bg_nested.degenerate())
                                        stack.push({ ci.transformation(bg_nested.opdb), !si.canon_inverse });
                        } else {
                                items.push_back(opd);
                        }

                        break;
                }
        }

        lout << "Result of unfolding:\n";
        for (Operand opd : items)
                lout << opd.string() << "\n";

        return items;
}

Operand fold(Operation *op, const std::vector <Operand> &opds)
{
        if (opds.size() == 0) {
                fatal_error("fold", "no operands to fold");
                return Operand {};
        }

        lout << "Folding with operation: " << op->lexicon << "\n";
        for (Operand opd : opds)
                lout << "  $ " << opd.string() << "\n";

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

namespace detail {

// TODO: hash.hpp/cpp

struct ExpressionHash {
        std::vector <int64_t> linear;
        // std::vector <int32_t> redirect;
};

ExpressionHash hash(const Operand &opd)
{
        if (opd.is_constant()) {
                int64_t hash = (opd.type == eInteger) ?
                        std::bit_cast <int64_t, Integer> (opd.i)
                        : std::bit_cast <int64_t, double> (opd.r);

                return ExpressionHash { { hash } };
        }

        UnresolvedOperand uo = opd.uo;
        if (uo.type == eVariable) {
                // TODO: compress with 8 chars per hash
                std::vector <int64_t> hash;
                for (char c : uo.as_variable().lexicon)
                        hash.push_back(c);

                return ExpressionHash { hash };
        }

        if (uo.type == eBinaryGrouping) {
                BinaryGrouping bg = uo.as_binary_grouping();
                ExpressionHash hash_a = hash(bg.opda);
                ExpressionHash hash_b = hash(bg.opdb);

                std::vector <int64_t> hash { bg.op->id };
                hash.insert(hash.end(), hash_a.linear.begin(), hash_a.linear.end());
                hash.insert(hash.end(), hash_b.linear.begin(), hash_b.linear.end());

                // TODO: populate rediction table to be able to rotate commutation operations...

                return ExpressionHash { hash };
        }

        throw std::runtime_error("hash: unknown operand type");
}

int64_t match(const ExpressionHash &a, const ExpressionHash &b)
{
        // TODO: return the size of best match (e.g. factor out common subexpressions)
        int64_t min = 0;

        // In order comparison first
        uint64_t size = std::min(a.linear.size(), b.linear.size());
        for (size_t i = 0; i < size; i++)
                min += std::abs(a.linear[i] - b.linear[i]);

        for (size_t i = size; i < a.linear.size(); i++)
                min += std::abs(a.linear[i]);

        for (size_t i = size; i < b.linear.size(); i++)
                min += std::abs(b.linear[i]);

        // TODO: commutativity and inverses, etc

        return min;
}

Operation *promote(Operation *op)
{
        assert(op->classifications & eOperationCommutative);

        if (op->id == op_add->id)
                return op_mul;
        if (op->id == op_mul->id)
                return op_exp;

        warning("promote", "unknown promotion rule for \'" + op->lexicon + "\'");
        return op;
}

// NOTE: Assumes the parent operation was commutative, but does not check for it
std::vector <Operand> simplification_gather(Operation *focus, const std::vector <Operand> &items)
{
        //TODO: pass operation to give context to matching
        // Gather common factors
        // TODO: log instead of printing outright...
        // e.g. define [...] cosntexpr operator<< (void) {} if disabled...
        lout << "[!] Attempting to gather items from:\n";
        for (Operand opd : items)
                lout << "  $ " << opd.string() << "\n";

        std::vector <ExpressionHash> hashes;
        for (Operand opd : items)
                hashes.push_back(hash(opd));

        lout << "[!] Hashes:\n";
        for (ExpressionHash hash : hashes) {
                lout << "  $ ";
                for (int64_t h : hash.linear)
                        lout << h << " ";
                lout << "\n";
        }

        // Search for pairwise matches
        // TODO: matching should account for commutativity and inverse operations

        // NOTE: index, count
        std::unordered_map <int32_t, int32_t> gathered;

        std::vector <bool> ticked(items.size(), false);
        for (size_t i = 0; i < hashes.size(); i++) {
                if (!ticked[i]) {
                        gathered[i] = 1;
                        ticked[i] = true;
                }

                for (size_t j = i + 1; j < hashes.size(); j++) {
                        int64_t m = match(hashes[i], hashes[j]);
                        if (m == 0) {
                                lout << "[!] Match between " << i << " and " << j << ": " << m << "\n";
                                gathered[i]++;
                                ticked[j] = true;
                        }
                }
        }

        lout << "Gathered map:\n";
        for (auto [i, count] : gathered)
                lout << "  $ " << i << " -> " << count << "\n";

        Operation *op = promote(focus);

        std::vector <Operand> gathered_items;
        for (auto [i, count] : gathered) {
                if (count > 1) {
                        // TODO: fold if promotion is commutative
                        // to reduce the number of operations
                        Operand opd = items[i];
                        gathered_items.push_back(Operand {
                                new_ <BinaryGrouping> (op, opd, count),
                                eBinaryGrouping
                        });
                } else {
                        gathered_items.push_back(items[i]);
                }
        }

        lout << "[!] Gathered items:\n";
        for (Operand opd : gathered_items)
                lout << "  $ " << opd.string() << "\n";

        return gathered_items;
}

Operand simplification_fold(Operation *focus, const BinaryGrouping &bg)
{
        assert(focus->classifications & eOperationCommutative);

        std::vector <Operand> items = unfold(focus, bg);
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

                        constant = opftn(focus, { constant, opd });
                }
        }

        Operand unresolved_folded;
        if (unresolved.size() == 1) {
                unresolved_folded = simplify(unresolved[0]);
        } else if (unresolved.size() > 0) {
                unresolved = simplification_gather(focus, unresolved);
                unresolved_folded = fold(focus, unresolved);
        }

        // TODO: hash each element and see if we can combine terms...
        // or factor common terms (thats maximizes some score...)

        assert(!constant.is_blank() || !unresolved_folded.is_blank());
        if (constant.is_blank())
                return unresolved_folded;
        if (unresolved_folded.is_blank())
                return constant;

        return Operand {
                new_ <BinaryGrouping> (focus, constant, unresolved_folded),
                eBinaryGrouping
        };
}

}

Operand simplify(const BinaryGrouping &bg)
{
        if (bg.degenerate())
                return simplify(bg.opda);

        lout << "\n--> Simplifying: " << bg.string() << "\n";
        Operation *focus = bg.op;
        lout << "Original focus: " << focus->lexicon << "\n";

        // TODO: prefix with g_
        for (auto pr : commutative_inverses) {
                if (pr.second.id == bg.op->id) {
                        focus = &g_operations[pr.first];
                        lout << "Found inverse: " << focus->lexicon << "\n";
                        break;
                }
        }

        if (!(focus->classifications & eOperationCommutative)) {
                lout << "Regular simplification:\n" << bg.string() << "\n";

                // Simplify the operands
                Operand a = simplify(bg.opda);
                Operand b = simplify(bg.opdb);

                // If both are constant, then combine them
                if (a.is_constant() && b.is_constant()) {
                        Operand res = opftn(bg.op, { a, b });
                        return res;
                }

                // Otherwise, return the grouping
                BinaryGrouping out = bg;

                out.opda = a;
                out.opdb = b;

                return { new_ <BinaryGrouping> (out), eBinaryGrouping };
        }

        // Perform fold simplification first
        lout << "Simplifying with operation: " << focus->lexicon << "\n";
        Operand simplified = detail::simplification_fold(focus, bg);
        lout << "Fold simplification:\n" << simplified.pretty() << "\n";

        // If no longer a binary grouping, then return
        if (!simplified.is_binary_grouping()) {
                lout << "No longer a binary grouping" << "\n";
                // Check what other strategies we have
                return simplify(simplified);
        }
        
        // Check for degeneracy again
        // TODO: why is this allowed in the first plane?
        BinaryGrouping bgopt = simplified.uo.as_binary_grouping();
        if (bgopt.degenerate()) {
                lout << "Degenerate simplification: " << bgopt.opda.string() << "\n";
                return simplify(bgopt.opda);
        }

        // Other wise
        lout << "Simplifying first branch: " << bgopt.opda.string() << "\n";
        Operand a = simplify(bgopt.opda);

        lout << "Simplifying second branch: " << bgopt.opdb.string() << "\n";
        Operand b = simplify(bgopt.opdb);

        // If both are constant, then combine them
        if (a.is_constant() && b.is_constant()) {
                Operand res = opftn(bgopt.op, { a, b });
                return res;
        }

        // TODO: otherwise fallback to specialized simplification rules

        // TODO: top down simplification or bottom up simplification?
        // bottom down is simpler/faster, but top down is more powerful
        // and we may be able to manipulate the tree in a more meaningful
        // way
        
        // TODO: negative exponents should be transfered to the denominator

        // TODO: hash each branch and compare to see if we can combine or cancel terms
        // What is the hash function?

        // TODO: lastly, apply operation specific rules (e.g. x * 1 = x, x * 0 = 0, etc., x^0 = 1, etc.)
        // Otherwise, return the grouping
        BinaryGrouping out = bgopt;

        out.opda = a;
        out.opdb = b;

        lout << "[*]  post branch simplification: " << out.string() << "\n";
        // TODO: std::optional <Operand> simplify_aggressive(const BinaryGrouping &bg)
        // -> simplify_aggressive_mult,etc.

        // TODO: possibly loop until no more simplifications can be made

        lout << "[*]  out: " << out.string() << "\n";

        return { new_ <BinaryGrouping> (out), eBinaryGrouping };
}

Operand simplify(const Operand &opd)
{
        // TODO: this function is short, combine with one above?
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

}
