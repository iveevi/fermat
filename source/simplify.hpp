#pragma once

// Local headers
#include "operand.hpp"

#include <map>

namespace fermat {

namespace detail {

// TODO: lower-case
struct ExpressionHash {
        std::vector <int64_t> linear;
        // std::vector <int32_t> redirect;

        std::string string() const {
                std::string ret;

                for (auto i : linear) {
                        ret += std::to_string(i);
                        if (i != linear.back())
                                ret += " ";
                }

                return "(" + ret + ")";
        }
};

int64_t cmp(const ExpressionHash &, const ExpressionHash &);
struct simplification_context {
        // TODO: use to cache repeated expressions
        using cachelet = std::pair <ExpressionHash, std::vector <Operand>>;

        std::vector <cachelet> cache;

        int64_t find(const ExpressionHash &hash) const {
                for (int64_t i = 0; i < cache.size(); ++i) {
                        if (cmp(cache[i].first, hash) == 0) {
                                return i;
                        }
                }

                return -1;
        }

        std::string string() const {
                std::string ret;

                for (auto &i : cache) {
                        ret += i.first.string() + " -> ";
                        for (auto &j : i.second) {
                                ret += j.string() + " ";
                        }
                        ret += "\n";
                }

                return ret;
        }
};

}

Operand simplify(const Operand &, detail::simplification_context &);
Operand simplify(const BinaryGrouping &, detail::simplification_context &);

// TODO: is this needed?
// std::vector <Operand> unfold(const BinaryGrouping &bg);
// Operand fold(Operation *op, const std::vector <Operand> &opds);

}
