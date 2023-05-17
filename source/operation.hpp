#pragma once

// Standard headers
#include <string>

namespace fermat {

using OperationId = int64_t;

enum Classifications : uint64_t {
        eOperationNone,
        eOperationCommutative,
        eOperationAssociative,
        eOperationDistributive,
        eOperationLinear,
};

inline Classifications operator|(Classifications lhs, Classifications rhs)
{
        return static_cast <Classifications>
                (static_cast <uint64_t> (lhs)
                | static_cast <uint64_t> (rhs));
}

enum Priority {
        ePriorityAdditive = 0,
        ePriorityMultiplicative = 1,
        ePriorityExponential = 2,
};

struct Operation {
        OperationId id;
        std::string lexicon;
        Priority priority; // TODO: refactor to precedence
        Classifications classifications;
};

}
