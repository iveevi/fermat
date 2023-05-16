#include "operand.hpp"

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

                if (uo.type == eFactor)
                        return static_cast <Factor *> (uo.ptr.get())->string(parent);

                if (uo.type == eTerm)
                        return static_cast <Term *> (uo.ptr.get())->string(parent);

                if (uo.type == eExpression)
                        return static_cast <Expression *> (uo.ptr.get())->string(parent);
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
                // TODO: switch
                if (uo.type == eVariable)
                        return static_cast <Variable *> (uo.ptr.get())->pretty(indent);

                if (uo.type == eFactor)
                        return static_cast <Factor *> (uo.ptr.get())->pretty(indent);

                if (uo.type == eTerm)
                        return static_cast <Term *> (uo.ptr.get())->pretty(indent);

                if (uo.type == eExpression)
                        return static_cast <Expression *> (uo.ptr.get())->pretty(indent);
        }

        return inter + "<?:" + std::to_string(type) + ">";
}
