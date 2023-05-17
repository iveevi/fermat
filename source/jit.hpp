#pragma once

// Standard headers
#include <map>
#include <string>

// JIT
#include <libgccjit++.h>

// Local headers
#include "operand.hpp"

namespace fermat {

enum OptimizationLevel { O0 = 0, O1 = 1, O2 = 2, O3 = 3, Og = -1 };

struct JITContext {
        gccjit::context ctx;
        
        gccjit::type type;
        gccjit::type type_ptr;

        gccjit::block block;

        std::map <std::string, gccjit::lvalue> variables;

        // TODO: local function table for currently imported function symbols
};

struct JITFunction {
        using __jit_ftn_t = Real (*)(const Real *);
        
        __jit_ftn_t ftn;
        uint32_t parameters;
        gcc_jit_result *result = nullptr;

        JITFunction(gcc_jit_result *result_, uint32_t parameters_)
                          : parameters(parameters_), result(result_) {
                constexpr const char *name = "ftn";

                void *ptr = gcc_jit_result_get_code(result, name);
                if (!ptr)
                        throw std::runtime_error("JITFunction: failed to get code");

                ftn = reinterpret_cast <__jit_ftn_t> (ptr);
        }

        ~JITFunction() {
                if (result)
                        gcc_jit_result_release(result);
        }

        Real operator()(const std::vector <Real> &args) const {
                assert(args.size() == parameters);
                return ftn(args.data());
        }

        template <typename ... Args>
        Real operator()(Args ... args) const {
                Real opds[] = { args ... };
                assert(sizeof(opds) / sizeof(Real) == parameters);
                return ftn(opds);
        }
};


gccjit::rvalue __jit_parse(JITContext &, const Operand &);

}
