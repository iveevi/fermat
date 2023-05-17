// Local headers
#include "jit.hpp"
#include "operation_impl.hpp"

namespace fermat {

// TODO: detail
gccjit::rvalue __jit_parse(JITContext &jit_ctx, const BinaryGrouping &bg)
{
        if (bg.degenerate())
                return __jit_parse(jit_ctx, bg.opda);

        gccjit::rvalue a = __jit_parse(jit_ctx, bg.opda);
        gccjit::rvalue b = __jit_parse(jit_ctx, bg.opdb);

        gccjit::rvalue c;
        if (bg.op->id == op_add->id)
                c = jit_ctx.ctx.new_plus(jit_ctx.type, a, b);
        else if (bg.op->id == op_sub->id)
                c = jit_ctx.ctx.new_minus(jit_ctx.type, a, b);
        else if (bg.op->id == op_mul->id)
                c = jit_ctx.ctx.new_mult(jit_ctx.type, a, b);
        else if (bg.op->id == op_div->id)
                c = jit_ctx.ctx.new_divide(jit_ctx.type, a, b);
        else if (bg.op->id == op_exp->id) {
                // Import function
                // TODO: cache...
                std::vector <gccjit::param> powl_args {
                        jit_ctx.ctx.new_param(jit_ctx.type, "a"),
                        jit_ctx.ctx.new_param(jit_ctx.type, "b")
                };

                gccjit::function powl_fn = jit_ctx.ctx.new_function(GCC_JIT_FUNCTION_IMPORTED,
                        jit_ctx.type, "powl", powl_args, 0);

                c = jit_ctx.ctx.new_call(powl_fn, a, b);
                // c = jit_ctx.ctx.new_divide(jit_ctx.type, a, b);
        } else
                throw std::runtime_error("unsupported binary operator: " + bg.string());

        return c;
}

gccjit::rvalue __jit_parse(JITContext &jit_ctx, const Operand &opd)
{
        if (opd.is_blank())
                throw std::runtime_error("blank operand");

        if (opd.is_constant()) {
                double d = (opd.type == eReal) ? opd.r : opd.i;
                return jit_ctx.ctx.new_rvalue(jit_ctx.type, d);
        }

        UnresolvedOperand uo = opd.uo;
        if (uo.type == eVariable) {
                auto it = jit_ctx.variables.find(uo.as_variable().lexicon);
                if (it == jit_ctx.variables.end())
                        throw std::runtime_error("variable not found");

                return it->second;
        }

        if (uo.type == eBinaryGrouping)
                return __jit_parse(jit_ctx, uo.as_binary_grouping());

        throw std::runtime_error("unsupported operand type");
}

}
