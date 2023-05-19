#pragma once

// Standard headers
#include <map>
#include <iostream> // TODO: <- remove

// Local headers
#include "jit.hpp"
#include "operand.hpp"
#include "simplify.hpp"

namespace fermat {

struct PartiallyEvaluated {
        const Operand src; // NOTE: tihs one does not change...
        Operand opd;

        std::map <std::string, int> ordering;
        std::map <std::string, std::vector <Operand *>> addresses;

        Operand operator()(const std::map <std::string, Operand> &values) const {
                for (const auto &pair : values) {
                        const std::string &var = pair.first;
                        const Operand &opd = pair.second;

                        const std::vector <Operand *> &addresses = this->addresses.at(var);
                        for (Operand *address : addresses)
                                *address = opd;
                }

                // std::cout << "Substituted: " << opd.string() << std::endl;
                // std::cout << "src = " << src.string() << std::endl;

                detail::simplification_context sctx;
                return simplify(opd, sctx);
        }

        template <typename ... Args>
        Operand operator()(Args ... args) const {
                std::vector <Operand> opds = { args ... };
                assert(opds.size() == ordering.size());

                for (const auto &pair : addresses) {
                        const std::string &var = pair.first;
                        const std::vector <Operand *> &addresses = pair.second;

                        int index = ordering.at(var);
                        assert(index < opds.size());

                        for (Operand *address : addresses)
                                *address = opds[index];
                }

                // std::cout << "Substituted: " << opd.string() << std::endl;
                // std::cout << "src = " << src.string() << std::endl;

                detail::simplification_context sctx;
                return simplify(opd, sctx);
        }

        // Generate JIT-compiled function
        JITFunction emit(OptimizationLevel level = O0, bool dump = false) {
                // std::cout << "emitting: " << src.string() << std::endl;
                // TODO: detect maximal duplicate nodes and emit code as
                // apprpriate..

                gccjit::context ctx = gccjit::context::acquire();

                // Configure options
                ctx.set_bool_option (GCC_JIT_BOOL_OPTION_DUMP_GENERATED_CODE, dump);

                if (level == Og)
                        ctx.set_bool_option (GCC_JIT_BOOL_OPTION_DEBUGINFO, 1);
                else
                        ctx.set_int_option (GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL, level);

                // Set types
                gccjit::type type = ctx.get_type(GCC_JIT_TYPE_LONG_DOUBLE);
                gccjit::type type_ptr = type.get_pointer().get_const();

                // Allocate rvalues for variables
                gccjit::param array = ctx.new_param(type_ptr, "array");

                std::map <std::string, gccjit::lvalue> variables;
                for (const auto &pair : ordering)
                        variables[pair.first] = array[pair.second];

                std::vector <gccjit::param> args = { array };
                gccjit::function ftn = ctx.new_function(GCC_JIT_FUNCTION_EXPORTED,
                        ctx.get_type(GCC_JIT_TYPE_LONG_DOUBLE), "ftn", args, 0);

                // Generate the code for the expression
                JITContext jit_ctx {
                        ctx, type, type_ptr,
                        ftn.new_block(), variables
                };

                gccjit::rvalue ret = detail::jit_parse(jit_ctx, src);
                jit_ctx.block.end_with_return(ret);

                // Compile the code
                gcc_jit_result *result = ctx.compile();
                if (!result)
                        throw std::runtime_error("JITFunction: failed to compile");

                // ctx.dump_to_file("jit.c", 0);
                ctx.release();

                return JITFunction { result, ordering.size() };
        }

        // TODO: method to partiall evaluate with respect to a variable
        // tjen the name will make more sense
};

PartiallyEvaluated partially_evaluate(const Operand &);

}
