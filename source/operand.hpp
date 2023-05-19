#pragma once

// Standard headers
#include <cassert>
#include <memory>
#include <string>
#include <vector>

// Local headers
#include "operation.hpp"

namespace fermat {

using Integer = long long int;
using Real = long double;

enum : int64_t {
        eBlank,

        eInteger,
        eReal,
        eUnresolved,

        eVariable,
        eFunction,
        eBinaryGrouping
};

struct Variable;
struct Function;
struct BinaryGrouping;

using Uptr = std::shared_ptr <void>;

struct UnresolvedOperand {
        Uptr ptr;
        int64_t type;
        
        Variable &as_variable() {
                assert(type == eVariable);
                return *static_cast <Variable *> (ptr.get());
        }

        BinaryGrouping &as_binary_grouping() {
                assert(type == eBinaryGrouping);
                return *static_cast <BinaryGrouping *> (ptr.get());
        }

        const Variable &as_variable() const {
                assert(type == eVariable);
                return *static_cast <Variable *> (ptr.get());
        }

        const BinaryGrouping &as_binary_grouping() const {
                assert(type == eBinaryGrouping);
                return *static_cast <BinaryGrouping *> (ptr.get());
        }
};

template <typename T, typename ... Args>
Uptr new_(Args ... args)
{
        return std::make_shared <T> (args ...);
}

// Operands are:
//   integers
//   real numbers
//   variables
//   functions
//   factors
//   terms
//   expressions (parenthesized)
struct Operand {
        Operand() = default;

        // Constructor for constants
        template <typename I>
        requires std::is_integral_v <I>
        Operand(I i_) : i { i_ }, type { eInteger } {}

        template <typename R>
        requires std::is_floating_point_v <R>
        Operand(R r_) : r { r_ }, type { eReal } {}

        // Assume unresolved
        Operand(const Uptr &uptr, int64_t type_)
                : uo { .ptr = uptr, .type = type_ },
                type { eUnresolved } {}

        // Deep clone
        Operand clone() const;

        // Properties
        bool is_zero() const {
                return (type == eInteger && i == 0ll)
                        || (type == eReal && r == 0.0l);
        }

        bool is_one() const {
                return (type == eInteger && i == 1ll)
                        || (type == eReal && r == 1.0l);
        }

        bool is_integer() const {
                return (type == eInteger);
        }

        bool is_real() const {
                return (type == eReal);
        }

        bool is_constant() const {
                return (type == eInteger || type == eReal);
        }

        bool is_variable() const {
                return (type == eUnresolved) && (uo.type == eVariable);
        }

        bool is_binary_grouping() const {
                return (type == eUnresolved) && (uo.type == eBinaryGrouping);
        }

        bool is_blank() const {
                return (type == eBlank);
        }

        // Printing
        std::string string(Operation * = nullptr) const;
        std::string pretty(int = 0) const;

        // Special constructors
        static Operand zero() {
                return Operand { 0ll };
        }

        static Operand one() {
                return Operand { 1ll };
        }
        
        // Possible types
        Integer i;
        Real r;
        UnresolvedOperand uo;
        int64_t type = eBlank;
};

// Variables
struct Variable {
        std::string lexicon;

        Variable() = default;
        Variable(std::string lexicon_) : lexicon { lexicon_ } {}

        // TODO: store relations with other variables if being indexed...
        // e.g. x_i or y_i...
        std::string string(Operation * = nullptr) const {
                return lexicon;
        }

        std::string pretty(int indent = 0) const {
                std::string inter(4 * indent, ' ');
                return inter + "<variable:"  + lexicon + ">";
        }
};

// General binary grouping
struct BinaryGrouping {
        // A binary grouping is either a single term (null op)
        // or an operation with two operands
        Operation *op = nullptr;
        Operand opda;
        Operand opdb;

        BinaryGrouping() = default;
        BinaryGrouping(Operation *op_, Operand opda_, Operand opdb_)
                : op { op_ }, opda { opda_ }, opdb { opdb_ } {
                assert(op);
        }

        // Deep clone
        BinaryGrouping clone() const {
                return BinaryGrouping { op, opda.clone(), opdb.clone() };
        }

        bool degenerate() const {
                return (op == nullptr);
        }

        std::string string(Operation *parent = nullptr) const {
                if (degenerate())
                        return opda.string(op);

                std::string inter = opda.string(op) + op->lexicon + opdb.string(op);

                if (parent && op->priority < parent->priority)
                        return "(" + inter + ")";

                return inter;
        }

        std::string pretty(int indent = 0) const {
                if (degenerate())
                        return opda.pretty(indent);

                std::string inter = std::string(4 * indent, ' ') + "<op:" + op->lexicon + ">";
                std::string sub1 = "\n" + opda.pretty(indent + 1);
                std::string sub2 = "\n" + opdb.pretty(indent + 1);

                if (op->classifications & eOperationCommutative) {
                        if (sub1.length() > sub2.length())
                                std::swap(sub1, sub2);
                }

                return inter + sub1 + sub2;
        }
};

}
