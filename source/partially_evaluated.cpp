// Standard headers
#include <algorithm>
#include <set>
#include <stack>

// Local header
#include "error.hpp"
#include "partially_evaluated.hpp"

namespace fermat {

PartiallyEvaluated partially_evaluate(const Operand &opd)
{
        assert(!opd.is_blank());

        PartiallyEvaluated pe { opd };
        pe.opd = opd.clone();

        if (opd.is_constant()) {
                warning("convert", "constant operand, opd=<" + opd.string() + ">");
                return { opd, opd };
        }

        std::set <std::string> variables;

        std::stack <Operand *> stack;
        stack.push(&pe.opd);

        auto push_address = [&](const std::string &var, Operand *address) {
                if (pe.addresses.find(var) == pe.addresses.end())
                        pe.addresses[var] = { address };
                else
                        pe.addresses[var].push_back(address);
        };

        while (!stack.empty()) {
                Operand *opd = stack.top();
                stack.pop();

                if (opd->is_constant())
                        continue;

                UnresolvedOperand uo = opd->uo;
                switch (uo.type) {
                case eVariable:
                        variables.insert(uo.as_variable().lexicon);
                        // TODO: record location
                        push_address(uo.as_variable().lexicon, opd);
                        break;
                case eBinaryGrouping:
                        BinaryGrouping &bg = uo.as_binary_grouping();
                        stack.push(&bg.opda);

                        if (!bg.degenerate())
                                stack.push(&bg.opdb);

                        break;
                }
        }

        // std::cout << "variables: " << variables.size() << std::endl;
        // for (const std::string &var : variables)
        //         std::cout << "  " << var << std::endl;

        std::vector <std::string> sorted(variables.begin(), variables.end());
        std::sort(sorted.begin(), sorted.end());

        for (int i = 0; i < sorted.size(); i++)
                pe.ordering[sorted[i]] = i;

        // std::cout << "ordering: " << pe.ordering.size() << std::endl;
        // for (const auto &pair : pe.ordering)
        //         std::cout << "  " << pair.first << " -> " << pair.second << std::endl;
        //
        // std::cout << "addresses: " << pe.addresses.size() << std::endl;
        // for (const auto &pair : pe.addresses) {
        //         std::cout << "  " << pair.first << " -> " << pair.second.size() << std::endl;
        //         for (Operand *address : pair.second)
        //                 std::cout << "    " << address->pretty(1) << " @ " << address << std::endl;
        // }

        return pe;
}

}
