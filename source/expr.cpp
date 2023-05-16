// Standard headers
#include <cassert>
#include <cmath>
#include <iostream>
#include <optional>
#include <stack>
#include <unordered_map>

// Local headers
#include "operand.hpp"
#include "operation.hpp"
#include "operation_impl.hpp"

// TODO: optional <Expression> parse()
struct ParsingState {
        enum {
                eStart,
                eInteger,
                eReal,
        } state;

        union {
                Integer i;
                struct {
                        Real value;
                        int point;
                        int exp;
                } real;
        };

        std::string buffer;
        int64_t index;

        char current() const {
                return buffer[index];
        }

        void advance() {
                index++;
        }

        bool end() const {
                return index >= buffer.size();
        }

        std::stack <Operand> operands;
        // Operation *current_operation;
        std::stack <Operation *> operations;
        std::stack <int64_t> scopes;

        void push(const Operand &opd) {
                operands.push(opd);
        }

        void push(Operation *op) {
                bool empty_scope = operations.empty();
                if (scopes.size() > 0)
                        empty_scope |= (operations.size() <= scopes.top());

                if (empty_scope) {
                        assert(op);
                        std::cout << "op on hold: \'" << op->lexicon << "\'" << std::endl;
                        operations.push(op);
                        return;
                }
                
                if (op)
                        std::cout << "op on hold: \'" << op->lexicon << "\'" << std::endl;

                // operations.push(op);
                Operation *prev = operations.top();
                if (!op || prev->priority >= op->priority) {
                        std::cout << "pushing op: \'" << prev->lexicon << "\'" << std::endl;

                        // TODO: error msg here...
                        assert(operands.size() >= 2);

                        // We can now add the previous operation
                        // safely along with its operands
                        Operand opda = operands.top();
                        operands.pop();

                        Operand opdb = operands.top();
                        operands.pop();

                        std::cout << "  opda: " << opda.string() << std::endl;
                        std::cout << "  opdb: " << opdb.string() << std::endl;

                        // TODO: check to mke sure the operations matches
                        // grouping -- or do we even need to specialize the
                        // groupings? -- yes, it is easier to handle
                        // smlpification and/or differentiation
                        operands.push({ new_ <Term> (prev, opdb, opda), eTerm });
                        operations.pop();
                }

                if (op)
                        operations.push(op);
        }

        void flush() {
                // Flush all operations
                // TODO: ensure that the number of operations decreases...
                if (scopes.empty()) {
                        while (!operations.empty())
                                push(nullptr);
                } else {
                        int64_t stop = scopes.top();
                        std::cout << "flushing until " << stop << " ops left" << std::endl;
                        while (!operations.empty() && operations.size() > stop)
                                push(nullptr);

                        scopes.pop();
                }
        }
};

std::optional <Operand> parse(const std::string &expression)
{
        ParsingState ps {
                .state = ParsingState::eStart,
                .buffer = expression,
                .index = 0,
                // .current_operation = nullptr,
        };

        // Build operation lexicon table
        // TODO: later need to populate from everything in a loop, e.g. if
        // custom operations are added
        std::unordered_map <std::string, OperationId> op_lexicon {
                { "+", op_add->id },
                { "-", op_sub->id },
                { "*", op_mul->id },
                { "/", op_div->id },
                { "^", op_exp->id },
        };

        auto is_legal_operator = [] (char c) {
                // NOTE: hard coded for now
                return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
        };

        while (!ps.end()) {
                char c = ps.current();
                ps.advance();

                if (std::isspace(c))
                        continue;

                while (true) {
                        auto old_state = ps.state;

                        switch (ps.state) {
                        case ParsingState::eStart:
                                if (std::isdigit(c)) {
                                        // std::cout << "digit: " << c << std::endl;
                                        ps.state = ParsingState::eInteger;
                                        ps.i = 0;
                                } else if (is_legal_operator(c)) {
                                        // TODO: mutlicharacter operators?
                                        OperationId op_id = op_lexicon[std::string(1, c)];
                                        // operands.push_back(op_id);
                                        // std::cout << "op: " << op_id << std::endl;
                                        // ps.current_operation = &g_operations[op_id];
                                        ps.push(&g_operations[op_id]);
                                } else if (std::isalpha(c)) {
                                        std::cout << "variable: " << c << std::endl;
                                        // TODO: literal constructor?
                                        ps.push({ new_ <Variable> (std::string(1, c)), eVariable });

                                        // TODO: permit underscores for variable names
                                        // and later longer grouped underscores (e.g. y_{3,4})
                                        // will also need to remember if ay indices are present in the
                                        // underscores
                                } else if (c == '(') {
                                        ps.state = ParsingState::eStart;
                                        ps.scopes.push(ps.operands.size());
                                } else if (c == ')') {
                                        ps.state = ParsingState::eStart;
                                        ps.flush();
                                } else if (c == 0) {
                                        // End
                                } else {
                                        throw std::runtime_error("parsing: unexpected character: \'" + std::string(1, c) + "\'");
                                }

                                break;
                        case ParsingState::eInteger:
                                if (std::isdigit(c)) {
                                        // std::cout << "Appending digit: " << c << std::endl;
                                        ps.i *= 10;
                                        ps.i += c - '0';
                                } else if (c == '.') {
                                        ps.real.value = ps.i;
                                        ps.real.point = 1;
                                        ps.real.exp = -1;

                                        ps.state = ParsingState::eReal;
                                } else {
                                        std::cout << "integer: " << ps.i << std::endl;
                                        ps.push(ps.i);
                                        ps.state = ParsingState::eStart;
                                }

                                break;
                        case ParsingState::eReal:
                                if (std::isdigit(c)) {
                                        // std::cout << "Appending digit: " << c << std::endl;
                                        // ps.real.value += 10;
                                        // ps.r += c - '0';
                                        // ps.r_exp -= 1;
                                        ps.real.value += (c - '0') * std::pow(10, ps.real.exp);
                                        ps.real.exp -= 1;
                                } else if (c == '.') {
                                        assert(ps.real.point == 1);
                                        // std::cout << "transfered from int: " << ps.real.value << std::endl;
                                } else {
                                        // std::cout << "real: " << ps.r << " * 10^" << ps.r_exp << std::endl;
                                        // ps.push(ps.r * std::pow(10, ps.r_exp));
                                        std::cout << "real: " << ps.real.value << std::endl;
                                        ps.push(ps.real.value);
                                        ps.state = ParsingState::eStart;
                                }

                                break;
                        default:
                                throw std::runtime_error("parsing: unsupported state");
                        }

                        if (old_state != ps.state) {
                                // std::cout << "relooping...\n";
                                continue;
                        }

                        // If we at the end of parsing, also reloop
                        if (ps.end() && c != 0) {
                                c = 0;
                                continue;
                        }

                        // If no state change or end of parsing, break
                        break;
                }

                if (ps.end())
                        ps.flush();
        }

        std::stack <Operand> stack = ps.operands;
        Operand result = stack.top();

        std::cout << "operands: " << stack.size() << std::endl;
        while (!stack.empty()) {
                std::cout << "  > " << stack.top().string() << std::endl;
                std::cout << stack.top().pretty() << std::endl;
                stack.pop();
        }

        // TODO: perform some endof parsing checks

        return result;
}
