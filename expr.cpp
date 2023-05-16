#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <stack>
#include <unordered_map>
#include <variant>
#include <vector>

using Integer = long long int;
using Real = long double;

enum : int64_t {
        eBlank,

        eInteger,
        eReal,
        eUnresolved,

        eVariable,
        eFunction,
        eFactor,
        eTerm,
        eExpression,
};

struct Factor;
struct Term;
struct Expression;

struct Operation;

struct UnresolvedOperand {
        void *ptr;
        int64_t type;
};

// Operands are:
//   integers
//   real numbers
//   variables
//   functions
//   factors
//   terms
//   expressions (parenthesized)
struct Operand {
        Operand() = default;

        // Constructor for constants
        Operand(Integer i) : base { .i = i }, type { eInteger } {}
        Operand(Real r) : base { .r = r }, type { eReal } {}

        // Constructor for unresolved operands
        Operand(Factor *f) : base {
                .uo = { .ptr = f, .type = eFactor }
        }, type { eUnresolved } {}

        Operand(Term *t) : base {
                .uo = { .ptr = t, .type = eTerm }
        }, type { eUnresolved } {}

        Operand(Expression *e) : base {
                .uo = { .ptr = e, .type = eExpression }
        }, type { eUnresolved } {}

        // Properties
        bool constant() const {
                return (type == eInteger || type == eReal);
        }

        bool blank() const {
                return (type == eBlank);
        }

        // bool resolved

        // Printing
        std::string string(Operation * = nullptr) const;
        std::string pretty(int = 0) const;

        // Special constructors
        static Operand zero() {
                return Operand { 0ll };
        }

        static Operand one() {
                return Operand { 1ll };
        }

        union {
                Integer i;
                Real r;
                UnresolvedOperand uo;
        } base;

        int64_t type = eBlank;
};

using OperandVector = std::vector <Operand>;

using VariableId = int64_t;
using Variable = VariableId;

using FunctionId = int64_t;
using Function = std::pair <FunctionId, void *>;

using OperationId = int64_t;

enum Classifications : uint64_t {
        eOperationNone,
        eOperationCommutative,
        eOperationAssociative,
        eOperationDistributive,
        eOperationLinear,
};

inline Classifications operator|(Classifications lhs, Classifications rhs)
{
        return static_cast <Classifications>
                (static_cast <uint64_t> (lhs)
                | static_cast <uint64_t> (rhs));
}

enum Priority {
        ePriorityAdditive = 0,
        ePriorityMultiplicative = 1,
        ePriorityExponential = 2,
};

struct Operation {
        OperationId id;
        std::string lexicon;
        Priority priority; // TODO: refactor to precedence
        Classifications classifications;
};

static OperationId getid() {
        static OperationId id = 0;
        return id++;
}

static std::vector <Operation> g_operations {
        {
                getid(), "+", ePriorityAdditive,
                eOperationCommutative | eOperationAssociative | eOperationDistributive | eOperationLinear,
        },

        {
                getid(), "-", ePriorityAdditive,
                eOperationLinear,
        },

        {
                getid(), "*", ePriorityMultiplicative,
                eOperationCommutative | eOperationAssociative | eOperationDistributive | eOperationLinear,
        },

        {
                getid(), "/", ePriorityMultiplicative,
                eOperationLinear,
        },

        {
                getid(), "^", ePriorityExponential,
                eOperationNone,
        },
};

static Operation *op_add = &g_operations[0];
static Operation *op_sub = &g_operations[1];
static Operation *op_mul = &g_operations[2];
static Operation *op_div = &g_operations[3];
static Operation *op_exp = &g_operations[4];

using __operation_function = std::function <Operand (const OperandVector &)>;

// TODO: overload manager to resolve between different groups of items
inline Operand __operation_function_add(const OperandVector &vec)
{
        Operand a = vec[0];
        Operand b = vec[1];

        Operand res;
        if (a.type == eInteger && b.type == eInteger)
                res = Operand { a.base.i + b.base.i };
        else if (a.type == eInteger && b.type == eReal)
                res = Operand { static_cast <Real> (a.base.i) + b.base.r };
        else if (a.type == eReal && b.type == eInteger)
                res = Operand { a.base.r + static_cast <Real> (b.base.i) };
        else if (a.type == eReal && b.type == eReal)
                res = Operand { a.base.r + b.base.r };
        else
                throw std::runtime_error("add: unsupported operand types "
                        + std::to_string(a.type) + " and " + std::to_string(b.type));

        return res;
}

inline Operand __operation_function_sub(const OperandVector &vec)
{
        Operand a = vec[0];
        Operand b = vec[1];

        Operand res;
        if (a.type == eInteger && b.type == eInteger)
                res = Operand { a.base.i - b.base.i };
        else if (a.type == eInteger && b.type == eReal)
                res = Operand { static_cast <Real> (a.base.i) - b.base.r };
        else if (a.type == eReal && b.type == eInteger)
                res = Operand { a.base.r - static_cast <Real> (b.base.i) };
        else if (a.type == eReal && b.type == eReal)
                res = Operand { a.base.r - b.base.r };
        else
                throw std::runtime_error("sub: unsupported operand types "
                        + std::to_string(a.type) + " and " + std::to_string(b.type));

        return res;
}

inline Operand __operation_function_mul(const OperandVector &vec)
{
        Operand a = vec[0];
        Operand b = vec[1];

        Operand res;
        if (a.type == eInteger && b.type == eInteger)
                res = Operand { a.base.i * b.base.i };
        else if (a.type == eInteger && b.type == eReal)
                res = Operand { static_cast <Real> (a.base.i) * b.base.r };
        else if (a.type == eReal && b.type == eInteger)
                res = Operand { a.base.r * static_cast <Real> (b.base.i) };
        else if (a.type == eReal && b.type == eReal)
                res = Operand { a.base.r * b.base.r };
        else
                throw std::runtime_error("mul: unsupported operand types "
                        + std::to_string(a.type) + " and " + std::to_string(b.type));

        return res;
}

inline Operand __operation_function_div(const OperandVector &vec)
{
        Operand a = vec[0];
        Operand b = vec[1];

        Operand res;
        if (a.type == eInteger && b.type == eInteger)
                res = Operand { a.base.i / b.base.i };
        else if (a.type == eInteger && b.type == eReal)
                res = Operand { static_cast <Real> (a.base.i) / b.base.r };
        else if (a.type == eReal && b.type == eInteger)
                res = Operand { a.base.r / static_cast <Real> (b.base.i) };
        else if (a.type == eReal && b.type == eReal)
                res = Operand { a.base.r / b.base.r };
        else
                throw std::runtime_error("div: unsupported operand types "
                        + std::to_string(a.type) + " and " + std::to_string(b.type));

        return res;
}

inline Operand __operation_function_exp(const OperandVector &vec)
{
        Real a;
        Real b;

        if (vec[0].type == eInteger)
                a = static_cast <Real> (vec[0].base.i);
        else
                a = vec[0].base.r;

        if (vec[1].type == eInteger)
                b = static_cast <Real> (vec[1].base.i);
        else
                b = vec[1].base.r;

        return std::pow(a, b);
}

static std::unordered_map <OperationId, __operation_function> g_operation_functions {
        { op_add->id, __operation_function_add },
        { op_sub->id, __operation_function_sub },
        { op_mul->id, __operation_function_mul },
        { op_div->id, __operation_function_div },
        { op_exp->id, __operation_function_exp },
};

Operand opftn(Operation *op, const OperandVector &vec)
{
        return g_operation_functions[op->id](vec);
}

struct BinaryGrouping {
        // A binary grouping is either a single term (null op)
        // or an operation with two operands
        Operation *op = nullptr;
        Operand opda;
        Operand opdb;

        BinaryGrouping() = default;
        BinaryGrouping(Operand opda_) : opda { opda_ } {}
        BinaryGrouping(Operation *op_, Operand opda_, Operand opdb_)
                : op { op_ }, opda { opda_ }, opdb { opdb_ } {}

        bool degenerate() const {
                return (op == nullptr);
        }

        std::string string(Operation *parent = nullptr) const {
                if (degenerate())
                        return opda.string(op);

                std::string inter = opda.string(op) + op->lexicon + opdb.string(op);

                if (parent && op->priority <= parent->priority)
                        return "(" + inter + ")";

                return inter;
        }

        std::string pretty(int indent = 0) const {
                if (degenerate())
                        return opda.pretty(indent);

                std::string inter = std::string(4 * indent, ' ') + "<op:" + op->lexicon + ">";
                std::string sub1 = "\n" + opda.pretty(indent + 1);
                std::string sub2 = "\n" + opdb.pretty(indent + 1);

                if (op->classifications & eOperationCommutative) {
                        if (sub1.length() > sub2.length())
                                std::swap(sub1, sub2);
                }

                return inter + sub1 + sub2;
        }
};

struct Factor : BinaryGrouping {
        Factor() = default;
        Factor(Operand opda_) : BinaryGrouping { opda_ } {}
        Factor(Operation *op_, Operand opda_, Operand opdb_)
                : BinaryGrouping { op_, opda_, opdb_ } {}
};

// TODO: move to its constructor...
bool factor_operation(Operation operation)
{
        return (operation.priority == ePriorityExponential);
}

struct Term : BinaryGrouping {
        Term() = default;
        Term(Operand opda_) : BinaryGrouping { opda_ } {}
        Term(Operation *op_, Operand opda_, Operand opdb_)
                : BinaryGrouping { op_, opda_, opdb_ } {}
};

bool term_operation(Operation operation)
{
        return (operation.priority == ePriorityMultiplicative);
}

struct Expression : BinaryGrouping {
        Expression() = default;
        Expression(Operand opda_) : BinaryGrouping { opda_ } {}
        Expression(Operation *op_, Operand opda_, Operand opdb_)
                : BinaryGrouping { op_, opda_, opdb_ } {}
};

bool expression_operation(Operation operation)
{
        return (operation.priority == ePriorityAdditive);
}

std::string Operand::string(Operation *parent) const
{
        if (type == eInteger)
                return std::to_string(base.i);
        
        if (type == eReal)
                return std::to_string(base.r);

        if (type == eUnresolved) {
                if (base.uo.type == eFactor)
                        return static_cast <Factor *> (base.uo.ptr)->string(parent);

                if (base.uo.type == eTerm)
                        return static_cast <Term *> (base.uo.ptr)->string(parent);

                if (base.uo.type == eExpression)
                        return static_cast <Expression *> (base.uo.ptr)->string(parent);
        }

        return "<?:" + std::to_string(type) + ">";
}

std::string Operand::pretty(int indent) const
{
        std::string inter = std::string(4 * indent, ' ');

        if (type == eInteger)
                return inter + "<integer:" + std::to_string(base.i) + ">";
        
        if (type == eReal)
                return inter + "<real:" + std::to_string(base.r) + ">";

        if (type == eUnresolved) {
                if (base.uo.type == eFactor)
                        return static_cast <Factor *> (base.uo.ptr)->pretty(indent);

                if (base.uo.type == eTerm)
                        return static_cast <Term *> (base.uo.ptr)->pretty(indent);

                if (base.uo.type == eExpression)
                        return static_cast <Expression *> (base.uo.ptr)->pretty(indent);
        }

        return inter + "<?:" + std::to_string(type) + ">";
}

std::pair <Operand, Operand> constant_eval(const Operand &);
std::pair <Operand, Operand> constant_eval(const Factor &);

template <typename Grouping>
std::pair <Operand, Operand> constant_eval(const BinaryGrouping &);

// NOTE: Return as [constant, unresolved] terms
std::pair <Operand, Operand> constant_eval(const Factor &factor)
{
        // TODO: Separate into constant and non-constant parts
        // and resolve possible unresolved operands

        // NOTE: for just evaluate as normal

        // If the factor is a constant, return it
        // otherwise apply the operation
        if (factor.degenerate())
                return constant_eval(factor.opda);

        assert(factor.op->id == op_exp->id);

        // Will need to upcast to reals anyway
        Operand res = opftn(factor.op, { factor.opda, factor.opdb });
        return { res, {} };
}

template <typename Grouping>
std::pair <Operand, Operand> constant_eval(const BinaryGrouping &bg)
{
        if (bg.degenerate())
                return constant_eval(bg.opda);

        auto [opda1, opda2] = constant_eval(bg.opda);
        auto [opdb1, opdb2] = constant_eval(bg.opdb);

        Operand opd_constant;
        Operand opd_unresolved;

        if (!opda1.blank() && !opdb1.blank())
                opd_constant = opftn(bg.op, { opda1, opdb1 });
        else if (opda1.blank() && !opdb1.blank())
                opd_constant = opdb1;
        else if (!opda1.blank() && opdb1.blank())
                opd_constant = opda1;

        if (!opda2.blank() && !opdb2.blank())
                opd_unresolved = new Grouping { bg.op, opda2, opdb2 };
        else if (opda2.blank() && !opdb2.blank())
                opd_unresolved = opdb2;
        else if (!opda2.blank() && opdb2.blank())
                opd_unresolved = opda2;

        return { opd_constant, opd_unresolved };
}

std::pair <Operand, Operand> constant_eval(const Operand &opd)
{
        // NOTE: Defer or resolve the operand
        if (opd.constant())
                return { opd, {} };

        UnresolvedOperand uo = opd.base.uo;
        switch (uo.type) {
        case eFactor:
                return constant_eval(*static_cast <Factor *> (uo.ptr));
        case eTerm:
                return constant_eval <Term> (*static_cast <Term *> (uo.ptr));
        case eExpression:
                return constant_eval <Expression> (*static_cast <Expression *> (uo.ptr));
        }

        throw std::runtime_error("constant_eval: unknown operand type");
}

// TODO: optional <Expression> parse()
struct ParsingState {
        enum {
                eStart,
                eInteger,
                eReal,
        } state;

        union {
                Integer i;
                Real r;
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

        void push(const Operand &opd) {
                operands.push(opd);
        }

        void push(Operation *op) {
                if (operations.empty()) {
                        operations.push(op);
                        return;
                }

                // operations.push(op);
                Operation *prev = operations.top();
                if (!op || prev->priority >= op->priority) {
                        // We can now add the previous operation
                        // safely along with its operands
                        Operand opda = operands.top();
                        operands.pop();

                        Operand opdb = operands.top();
                        operands.pop();

                        operands.push(new Term { prev, opdb, opda });
                        operations.pop();
                }

                if (op)
                        operations.push(op);
        }

        void flush() {
                // Flush all operations
                // TODO: ensure that the number of operations decreases...
                while (!operations.empty())
                        push(nullptr);
        }
};

int main()
{
        // Test expression 1 + 2 * 3^7/4
        OperandVector operands {
                1ll, 2ll, 3ll, 7ll, 4ll,
        };

        std::vector <Factor> factors {
                { op_exp, operands[2], operands[3] },
        };

        std::cout << "factor: " << factors[0].string() << std::endl;
        std::cout << "eval: " << constant_eval(factors[0]).first.string() << std::endl;

        Term t1 { op_mul, operands[1], &factors[0]};
        std::cout << "eval t1: " << constant_eval <Term> (t1).first.string() << std::endl;

        Term t2 { op_div, &t1, operands[4] };
        std::cout << "eval t2: " << constant_eval <Expression> (t2).first.string() << std::endl;

        Expression e1 { op_add, operands[0], &t2 };
        std::cout << "eval e1: " << constant_eval <Expression> (e1).first.string() << std::endl;

        std::cout << e1.string() << std::endl;

        // Example parsing
        std::string input = "1 + 2 * 3^7/4";
        
        ParsingState ps {
                .state = ParsingState::eStart,
                .buffer = input,
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

        // auto is_legal_identifier = [] (char c) {
        //         return std::isalnum(c) || c == '_';
        // };

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
                                        std::cout << "digit: " << c << std::endl;
                                        ps.state = ParsingState::eInteger;
                                        ps.i = 0;
                                } else if (is_legal_operator(c)) {
                                        // std::cout << "op char: " << c << std::endl;
                                        OperationId op_id = op_lexicon[std::string(1, c)];
                                        // operands.push_back(op_id);
                                        std::cout << "op: " << op_id << std::endl;
                                        // ps.current_operation = &g_operations[op_id];
                                        ps.push(&g_operations[op_id]);
                                } else if (c == 0) {
                                        // End
                                } else {
                                        throw std::runtime_error("parsing: unexpected character: \'" + std::string(1, c) + "\'");
                                }

                                break;
                        case ParsingState::eInteger:
                                if (std::isdigit(c)) {
                                        std::cout << "Appending digit: " << c << std::endl;
                                        ps.i *= 10;
                                        ps.i += c - '0';
                                } else {
                                        std::cout << "integer: " << ps.i << std::endl;
                                        ps.push(ps.i);
                                        ps.state = ParsingState::eStart;
                                }

                                break;
                        default:
                                throw std::runtime_error("parsing: unsupported state");
                        }

                        if (old_state != ps.state) {
                                std::cout << "relooping...\n";
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

        std::cout << "Value: " << constant_eval(result).first.string() << std::endl;
}
