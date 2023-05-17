#include "fermat.hpp"

int main()
{
        using namespace fermat;

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
        // for (const Operand &opd : items) {
        //         std::cout << "Constant? " << is_constant(opd) << "\n";
        //         std::cout << opd.pretty(1) << std::endl;
        // }

        Operand f = fold(e.op, items);
        std::cout << "folded: " << f.string() << std::endl;
        std::cout << f.pretty(1) << std::endl;

        // Operand fs = __simplificiation_fold(e);
        // std::cout << "simplified folded: " << fs.string() << std::endl;
        // std::cout << fs.pretty(1) << std::endl;

        PartiallyEvaluated pe = partially_evaluate(f);
        Operand a = pe(1, 2, 3);
        std::cout << "a: " << a.string() << std::endl;

        Operand b = pe({{"x", 1}, {"y", 2}, {"z", 3}});
        std::cout << "b: " << b.string() << std::endl;

        // TODO: pass signature of args to emit (the return value will be
        // determined by the expression itself)
        JITFunction jftn = pe.emit();
        std::cout << "ftn: " << jftn(1, 2, 3) << ", " << jftn(4, 5, 6) << std::endl;
}
