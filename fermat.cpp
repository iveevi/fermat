#include "fermat.hpp"

int main()
{
        using namespace fermat;

        // Example parsing
        // TODO: implicit multiplication via conjunction
        std::string input = "2 + 6 + 5 * (x - x) + 6/y * y + 5^(z * z) - 12 + 9 * z * 3/6";
        // std::string input = "2 * x + 3 * x";
        
        Operand result = parse(input).value();
        std::cout << "result: " << result.string() << std::endl;

        Operand simplified = simplify(result);
        std::cout << "simplified: " << simplified.string() << std::endl;

        PartiallyEvaluated pe = partially_evaluate(simplified);
        // Operand a = pe(1, 2, 3);
        // std::cout << "a: " << a.string() << std::endl;
        //
        // Operand b = pe({{"x", 1}, {"y", 2}, {"z", 3}});
        // std::cout << "b: " << b.string() << std::endl;

        // TODO: pass signature of args to emit (the return value will be
        // determined by the expression itself)
        JITFunction jftn = pe.emit();
        // std::cout << "ftn: " << jftn(1, 2, 3) << ", " << jftn(4, 5, 6) << std::endl;
}
