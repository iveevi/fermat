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
                        if (bg_nested.degenerate()) {
                                stack.push({ bg_nested.opda, si.canon_inverse });
                                break;
                        }

                        if (bg_nested.op->id == focus->id) {
                                stack.push({ bg_nested.opda, si.canon_inverse });
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

        // lout << "Result of unfolding:\n";
        // for (Operand opd : items)
        //         lout << opd.string() << "\n";

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

bool cmp(const Operand &a, const Operand &b)
{
        assert(!a.is_blank() && !b.is_blank());

        if (a.type != b.type)
                return false;

        if (a.is_constant()) {
                if (a.type == eInteger && b.type == eInteger)
                        return a.i == b.i;

                if (a.type == eReal && b.type == eReal)
                        return a.r == b.r;

                return a.type == b.type;
        }

        if (a.uo.type != b.uo.type)
                return false;

        UnresolvedOperand uoa = a.uo;
        UnresolvedOperand uob = b.uo;

        if (uoa.type == eVariable && uob.type == eVariable)
                return uoa.as_variable().lexicon == uob.as_variable().lexicon;

        if (uoa.type == eBinaryGrouping && uob.type == eBinaryGrouping) {
                BinaryGrouping bga = uoa.as_binary_grouping();
                BinaryGrouping bgb = uob.as_binary_grouping();

                return bga.op == bgb.op && cmp(bga.opda, bgb.opda) && cmp(bga.opdb, bgb.opdb);
        }

        warning("cmp", "unknown operand type");
        return false;
}

// Returns a constant factor if possible
// NOTE: This is different from general factorization,
// which is deffered to a later stage
Operand additive_constant_factor_match(Operation *prop, const Operand &base, const Operand &target)
{
        // NOTE: ONLY SUPPORTS MULTIPLICATIVE FACTORIZING
        // exponentiation isnt commutative
        assert(prop->id == op_mul->id);

        // We simply want to find a subexpression of target that equals base
        lout << "Constant factoring between " << base.string() << " and " << target.string() << "\n";

        std::vector <Operand> items = unfold(prop, target);
        for (Operand opd : items)
                lout << "  $ " << opd.string() << "\n";

        bool found = false;
        for (auto it = items.begin(); it != items.end(); it++) {
                if (cmp(base, *it)) {
                        lout << "  common: " << it->string() << "\n";
                        items.erase(it);
                        found = true;
                        break;
                }
        }

        if (!found) {
                lout << "  no common factor\n";
                return {};
        }

        lout << "Remaining items:\n";
        for (Operand opd : items)
                lout << "  $ " << opd.string() << "\n";

        if (items.size() == 0)
                return 1ll;

        if (items.size() == 1)
                return items[0];

        lout << "Folded:" << fold(prop, items).string() << "\n";
        return fold(prop, items);
}

Operand multiplicative_constant_factor_match(Operation *prop, const Operand &base, const Operand &target)
{
        // TODO: make a non commutative version of this
        // which aggresivel checks for common base (or bases that
        // can be factored out)
        //  - if the bases are both integers, then factor if perfect powers of each other
        //  - if the bases are both reals, then change base to e?
        assert(prop->id == op_exp->id);

        static auto base_of = [](const Operand &opd) -> std::pair <Operand, Operand> {
                // Sanity check
                if (opd.type == eUnresolved && opd.uo.type == eBinaryGrouping) {
                        BinaryGrouping bg = opd.uo.as_binary_grouping();
                        if (bg.op && bg.op->id == op_exp->id)
                                return { bg.opda, bg.opdb };
                }

                return { opd, 1ll };
        };

        auto [b1, e1] = base_of(base);
        auto [b2, e2] = base_of(target);

        lout << "Constant (multiplicative) factoring between " << base.string() << " and " << target.string() << "\n";
        lout << "  base 1: " << b1.string() << ", exponent: " << e1.string() << "\n";
        lout << "  base 2: " << b2.string() << ", exponent: " << e2.string() << "\n";

        if (!cmp(b1, b2))
                return {};

        lout << "  same base!\n";

        // TODO: add operators for this kind of semantics...
        // Operand e = e1 + e2;
        Operand exp;
        if (e1.is_one()) {
                exp = e2;
        } else if (e1.is_constant()) {
                if (e1.type == eInteger && e2.type == eInteger) {
                        if (e2.i % e1.i == 0)
                                exp = e2.i / e1.i;
                }
        } // TODO: if the exponents are themself additive factors...

        if (exp.is_blank()) {
                lout << "  no exponent match\n";
                return {};
        }

        lout << "  exponent match: " << exp.string() << "\n";
        
        // TODO: simplify exponent
        return simplify(exp);
}

Operand constant_factor_match(Operation *prop, const Operand &base, const Operand &target)
{
        if (prop->id == op_mul->id)
                return additive_constant_factor_match(prop, base, target);
        else if (prop->id == op_exp->id)
                return multiplicative_constant_factor_match(prop, base, target);

        warning("constant_factor_match", "unknown operation");
        return {};
}

inline Operation *promote(Operation *op)
{
        assert(op->classifications & eOperationCommutative);

        if (op->id == op_add->id)
                return op_mul;
        if (op->id == op_mul->id)
                return op_exp;

        warning("promote", "unknown promotion rule for \'" + op->lexicon + "\'");
        return op;
}

inline Operand identity(Operation *op)
{
        assert(op->classifications & eOperationCommutative);

        if (op->id == op_add->id)
                return 0ll;
        if (op->id == op_mul->id)
                return 1ll;

        warning("identity", "unknown identity rule for \'" + op->lexicon + "\'");
        return {};
}

// TODO: Assumes the parent operation was commutative, but does not check for it
std::vector <Operand> simplification_gather(Operation *focus, const std::vector <Operand> &unordered_items)
{
        // Sanity check for later assumptions
        assert(unordered_items.size() > 0);

        //TODO: pass operation to give context to matching
        // Gather common factors
        // TODO: log instead of printing outright...
        // e.g. define [...] cosntexpr operator<< (void) {} if disabled...

        lout << "[!] Attempting to gather items from:\n";
        for (Operand opd : unordered_items)
                lout << "  $ " << opd.string() << "\n";

        std::vector <ExpressionHash> hashes;
        for (Operand opd : unordered_items)
                hashes.push_back(hash(opd));

        lout << "[!] Hashes:\n";
        for (ExpressionHash hash : hashes) {
                lout << "  $ ";
                for (int64_t h : hash.linear)
                        lout << h << " ";
                lout << "\n";
        }

        // Sort items by hash length, in hopes that the
        // shorter ones will lead to successful hashes
        std::vector <Operand> items = unordered_items;
        std::sort(items.begin(), items.end(),
                [](const Operand &a, const Operand &b) {
                        return hash(a).linear.size() < hash(b).linear.size();
                }
        );

        // Recompute hashes
        hashes.clear();
        for (Operand opd : items)
                hashes.push_back(hash(opd));

        lout << "[!] Sorted items:\n";
        for (Operand opd : items)
                lout << "  $ " << opd.string() << "\n";

        // Search for pairwise matches
        // TODO: matching should account for commutativity and inverse operations

        // TODO: first pass gathering; combine hashes that are identical
        // (either by commutativity) or inverses...

        // NOTE: index, factor
        std::unordered_map <int32_t, Operand> gathered;

        std::vector <bool> ticked(items.size(), false);

        // TODO: currently doesnt combine 2x + 3x or y^3 * y^2
        // Need to find pairs that are identical or inverses -- easy for multiplication,
        // harder for addition -- specialize this function for these
        lout << "Checking for matches, focus = " << focus->lexicon << "\n";
        for (size_t i = 0; i < hashes.size(); i++) {
                if (!ticked[i]) {
                        gathered[i] = 1;
                        ticked[i] = true;
                } else {
                        gathered[i] = {};
                        continue;
                }

                for (size_t j = i + 1; j < hashes.size(); j++) {
                        Operand factor = constant_factor_match(promote(focus), items[i], items[j]);
                        if (factor.is_blank())
                                continue;

                        lout << "Factor: " << factor.string() << "\n";
                        gathered[i] = opftn(op_add, { gathered[i], factor });
                        ticked[j] = true;
                }
        }
        
        // TODO: factor.cpp which contains factorization functions
        // and uses a pairwise strategy like string substring matching

        // TODO: return early if no matches were found at all

        lout << "Gathered map:\n";
        for (auto [i, factor] : gathered)
                lout << "  $ " << i << " -> " << factor.string() << "\n";

        Operation *op = promote(focus);

        std::vector <Operand> gathered_items;
        for (auto [i, factor] : gathered) {
                if (factor.is_blank() || factor.is_zero())
                        continue;

                lout << "Combining " << items[i].string() << " with factor " << factor.string() << "\n";
                lout << "\tfactor is one? " << factor.is_one() << ", zero? " << factor.is_zero() << "\n";

                // TODO: comparison operations...
                if (!factor.is_one()) {
                        // TODO: fold if promotion is commutative
                        // to reduce the number of operations
                        Operand opd = items[i];
                        gathered_items.push_back(Operand {
                                new_ <BinaryGrouping> (op, opd, factor),
                                eBinaryGrouping
                        });
                } else {
                        gathered_items.push_back(items[i]);
                }
        }

        lout << "[!] Gathered items:\n";
        for (Operand opd : gathered_items)
                lout << "  $ " << opd.string() << "\n";

        // TODO: if empty, then return the identity element for the operation
        if (gathered_items.empty())
                gathered_items.push_back(identity(focus));

        return gathered_items;
}

Operand simplification_fold(Operation *focus, const BinaryGrouping &bg)
{
        assert(focus->classifications & eOperationCommutative);

        std::vector <Operand> items = unfold(focus, bg);

        std::vector <Operand> constants;
        std::vector <Operand> unresolved;

        for (const Operand &opd : items) {
                if (is_constant(opd))
                        constants.push_back(opd);
                else
                        unresolved.push_back(opd);
        }

        // Process unresolved in case it simplifies to a constant
        Operand unresolved_folded;
        if (unresolved.size() == 1) {
                unresolved_folded = simplify(unresolved[0]);
        } else if (unresolved.size() > 0) {
                // TODO: refactor to factor_compress...
                unresolved = simplification_gather(focus, unresolved);
                lout << "Simplification gather result:\n";
                for (Operand opd : unresolved)
                        lout << "  $ " << opd.string() << "\n";

                // TODO: check if the result is a constant...
                // if (unresolved.size() > 0)
                unresolved_folded = fold(focus, unresolved);
                if (is_constant(unresolved_folded)) {
                        constants.push_back(unresolved_folded);
                        unresolved_folded = {};
                }

                lout << "Net unresolved folded: " << unresolved_folded.string() << "\n";
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

// NOTE: Aggressive simplification, specialized for each operation
// this happens after fold simplification, so what is left are mostly
// special case optimizations
Operand simplification_aggressive(const BinaryGrouping &bg)
{
        // NOTE: By now the operands are not both constants...
        // also not degerate
        assert(!is_constant(bg.opda) || !is_constant(bg.opdb) || !bg.degenerate());

        Operand out = { new_ <BinaryGrouping> (bg), eBinaryGrouping };
        if (bg.op->classifications & eOperationCommutative) {
                const Operand &opda = bg.opda;
                const Operand &opdb = bg.opdb;

                if (bg.op->id == op_add->id) {
                        if (opda.is_zero())
                                out = opdb;
                        else if (opdb.is_zero())
                                out = opda;
                } else if (bg.op->id == op_mul->id) {
                        if (opda.is_one())
                                out = opdb;
                        else if (opdb.is_one())
                                out = opda;
                }
        }

        lout << "[*]  Aggressive simplification: " << out.string() << "\n";
        return out;
}

}

// TODO: graphviz DOT output of the simplification process
// or an alternative represenationt that recordst he process
// (and can be offered as an explanation later on)
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

        // lout << "[*]  out: " << out.string() << "\n";
        return detail::simplification_aggressive(out);

        // TODO: simplify combination again? if hashes have changed?
        // return { new_ <BinaryGrouping> (out), eBinaryGrouping };
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
