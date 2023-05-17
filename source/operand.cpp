#include "operand.hpp"

namespace fermat {

// Deep clone
Operand Operand::clone() const
{
        // Only need to clone when operand stores a pointer
        // e.g. when it is an unresolved operand
        if (is_blank() || is_constant())
                return *this;

        if (uo.type == eVariable) {
                return Operand {
                        new_ <Variable> (uo.as_variable()),
                        eVariable
                };
        }

        if (uo.type == eBinaryGrouping) {
                return Operand {
                        new_ <BinaryGrouping> (uo.as_binary_grouping().clone()),
                        eBinaryGrouping
                };
        }

        throw std::runtime_error("Operand::clone(): unknown type");
}

// Printing
std::string Operand::string(Operation *parent) const
{
        if (type == eInteger)
                return std::to_string(i);
        
        if (type == eReal)
                return std::to_string(r);

        if (type == eUnresolved) {
                // TODO: show the indices..
                if (uo.type == eVariable)
                        return static_cast <Variable *> (uo.ptr.get())->string(parent);

                if (uo.type == eBinaryGrouping)
                        return static_cast <BinaryGrouping *> (uo.ptr.get())->string(parent);
        }

        return "<?:" + std::to_string(type) + ">";
}

std::string Operand::pretty(int indent) const
{
        std::string inter = std::string(4 * indent, ' ');

        if (type == eInteger)
                return inter + "<integer:" + std::to_string(i) + ">";
        
        if (type == eReal)
                return inter + "<real:" + std::to_string(r) + ">";

        if (type == eUnresolved) {
                // TODO: show the indices..
                if (uo.type == eVariable)
                        return static_cast <Variable *> (uo.ptr.get())->pretty(indent);

                if (uo.type == eBinaryGrouping)
                        return static_cast <BinaryGrouping *> (uo.ptr.get())->pretty(indent);
        }

        return inter + "<?:" + std::to_string(type) + ">";
}

}
