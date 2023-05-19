#include <benchmark/benchmark.h>
#include <fermat.hpp>

constexpr const char *input = "2 + 6 + 5 * (x - x) + 6/y * y + 5^(z * z) - 12";

static void parsing(benchmark::State &state) {
        for (auto _ : state)
             fermat::parse(input);
}

BENCHMARK(parsing);

static void evaluate_partially_evaluated(benchmark::State &state)
{
        fermat::Operand result = fermat::parse(input).value();
        fermat::PartiallyEvaluated pe = fermat::partially_evaluate(result);

        assert(pe.ordering.size() == 3);
        for (auto _ : state)
                pe(1, 2, 3);
}

BENCHMARK(evaluate_partially_evaluated);

static void evaluate_jit_base(benchmark::State &state)
{
        fermat::Operand result = fermat::parse(input).value();
        fermat::PartiallyEvaluated pe = fermat::partially_evaluate(result);
        fermat::JITFunction jftn = pe.emit();

        for (auto _ : state)
                jftn(1, 2, 3);
}

BENCHMARK(evaluate_jit_base);

static void evaluate_jit_simplified(benchmark::State &state)
{
        fermat::Operand result = fermat::parse(input).value();
        fermat::detail::simplification_context sctx;
        result = fermat::simplify(result, sctx);

        fermat::PartiallyEvaluated pe = fermat::partially_evaluate(result);
        fermat::JITFunction jftn = pe.emit();

        for (auto _ : state)
                jftn(3);
}

BENCHMARK(evaluate_jit_simplified);

static void evaluate_jit_optimized(benchmark::State &state)
{
        fermat::Operand result = fermat::parse(input).value();
        fermat::PartiallyEvaluated pe = fermat::partially_evaluate(result);
        fermat::JITFunction jftn = pe.emit(fermat::O3);

        for (auto _ : state)
                jftn(1, 2, 3);
}

BENCHMARK(evaluate_jit_optimized);

BENCHMARK_MAIN();
