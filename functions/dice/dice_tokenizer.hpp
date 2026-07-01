#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <climits>
#include <cfloat>
#include <cmath>
#include <fmt/core.h>

#include <cctype>
#include <random>
using u32 = uint32_t;
using engine = std::mt19937;

static engine &drng()
{
    thread_local static engine gen(std::random_device{}());
    return gen;
}

static int dice_random(int maxi)
{
    if (maxi <= 0)
        return 0;
    return std::uniform_int_distribution<int>(1, maxi)(drng());
}

static double dice_random_f()
{
    return std::uniform_real_distribution<double>(0.0, 1.0)(drng());
}

namespace dice_tokenizer {
    enum class TokenType {
        NUM, PLUS, MINUS, MUL, DIV, POW,
        D, MIN_FUNC, MAX_FUNC,
        LPAREN, RPAREN, COMMA, END
    };

    struct Token {
        TokenType type;
        int value;
    };
    
    enum class OpType {
        NUMBER,
        ADD, SUB, NEG, MUL, DIV, POW,
        D_UNARY,
        D_BINARY,
        MINFUNC,
        MAXFUNC,
        MINLISTFUNC,
        MAXLISTFUNC,
        LISTFUNCRENDERING
    };

    enum class DiceMode {
        SUM,
        MIN,
        MAX
    };

    template<typename T>
    std::string myToString(T num) {
        return std::to_string(num);
    }

    std::string myToString(const double& num) {
        int inum = int(num + 0.4999);
        if (num < 0) {
            inum = int(num - 0.4999);
        }
        if (std::abs(num - inum) < 0.0001) {
            return std::to_string(inum);
        } else {
            return std::to_string(num);
        }
    }

    inline int getPreority(const OpType &op) {
        switch (op) {
            case OpType::ADD:
            case OpType::SUB:
            case OpType::NEG:
                return 1;
            case OpType::MUL:
            case OpType::DIV:
                return 2;
            case OpType::POW:
                return 3;
            case OpType::MINFUNC:
            case OpType::MAXFUNC:
                return 4;
            case OpType::D_UNARY:
            case OpType::D_BINARY:
                return 5;
            case OpType::MINLISTFUNC:
            case OpType::MAXLISTFUNC:
                return 5;
            default:
                return 0;
        }
    }

    template<typename T>
    inline std::string opToStringName(const OpType &op, const T val) {
        switch (op) {
            case OpType::NUMBER: return myToString(val);
            case OpType::ADD: return "ADD";
            case OpType::SUB: return "SUB";
            case OpType::NEG: return "NEG";
            case OpType::MUL: return "MUL";
            case OpType::DIV: return "DIV";
            case OpType::POW: return "POW";
            case OpType::D_UNARY: return "D_UNARY";
            case OpType::D_BINARY: return "D_BINARY";
            case OpType::MINFUNC: return "MINFUNC";
            case OpType::MAXFUNC: return "MAXFUNC";
            case OpType::MINLISTFUNC: return "MINLISTFUNC";
            case OpType::MAXLISTFUNC: return "MAXLISTFUNC";
            case OpType::LISTFUNCRENDERING: return "LISTFUNCRENDERING";
        }
        return "UNKNOWN";
    }

    template<typename T>
    inline std::string opToString(const OpType &op, const T val) {
        switch (op) {
            case OpType::NUMBER: return myToString(val);
            case OpType::ADD: return "+";
            case OpType::SUB: return "-";
            case OpType::NEG: return "-";
            case OpType::MUL: return "*";
            case OpType::DIV: return "/";
            case OpType::POW: return "^";
            case OpType::D_UNARY: return "d";
            case OpType::D_BINARY: return "d";
            case OpType::MINFUNC: return "min";
            case OpType::MAXFUNC: return "max";
            case OpType::MINLISTFUNC: return "min";
            case OpType::MAXLISTFUNC: return "max";
            case OpType::LISTFUNCRENDERING: return "";
        }
        return "UNKNOWN";
    }

    struct ExpTree {
        OpType op;
        int value;
        double calcVal;
        std::string renderedStr;
        ExpTree* left;
        ExpTree* right;

        ExpTree(int v) : op(OpType::NUMBER), value(v), calcVal(v), left(nullptr), right(nullptr) {}
        ExpTree(OpType op, ExpTree* l = nullptr, ExpTree* r = nullptr)
            : op(op), value(0), calcVal(0), left(l), right(r) {}

        std::string output() {
            switch (op) {
                case OpType::NUMBER:
                    return myToString(calcVal);

                case OpType::D_UNARY:
                    return fmt::format("d{}", right->output());

                case OpType::NEG:
                    return fmt::format("-{}", right->output());

                case OpType::MINFUNC:
                    return fmt::format("min({})", right->output());

                case OpType::MAXFUNC:
                    return fmt::format("max({})", right->output());

                case OpType::MINLISTFUNC:
                case OpType::MAXLISTFUNC: {
                    std::vector<ExpTree*> args = collectFuncArgs();
                    std::string joined;
                    for (size_t i = 0; i < args.size(); ++i) {
                        if (i) joined += ",";
                        joined += args[i]->output();
                    }
                    return fmt::format("{}({})", opToString(op, value), joined);
                }
                
                case OpType::LISTFUNCRENDERING:
                    return fmt::format("{}", renderedStr);

                default:
                    std::string leftStr = left ? left->output() : "";
                    std::string rightStr = right ? right->output() : "";
                    if (getPreority(op) > 1) {
                        if (left && left->op != OpType::NUMBER && getPreority(left->op) < getPreority(op)) {
                            leftStr = "(" + leftStr + ")";
                        }
                        if (right && right->op != OpType::NUMBER && getPreority(right->op) < getPreority(op)) {
                            rightStr = "(" + rightStr + ")";
                        }
                    }
                    return fmt::format("{}{}{}", leftStr, opToString(op, value), rightStr);
            }
        }

        std::vector<ExpTree*> collectFuncArgs() {
            std::vector<ExpTree*> args;
            if (left) {
                if (left->op == op) {
                    auto leftArgs = left->collectFuncArgs();
                    args.insert(args.end(), leftArgs.begin(), leftArgs.end());
                } else {
                    args.push_back(left);
                }
            }
            if (right) {
                if (right->op == op) {
                    auto rightArgs = right->collectFuncArgs();
                    args.insert(args.end(), rightArgs.begin(), rightArgs.end());
                } else {
                    args.push_back(right);
                }
            }
            return args;
        }

        std::string outputPostfix() {
            std::string result = "";
            if (left) result += left->outputPostfix() + " ";
            if (right) result += right->outputPostfix() + " ";

            if (op == OpType::NUMBER)
                result += myToString(value);
            else
                result += opToString(op, value);

            return result;
        }

        void parseVal() {
            left ? left->parseVal() : void();
            right ? right->parseVal() : void();
            if (op == OpType::NUMBER) {
                calcVal = value;
            } else {
                calcVal = 0;
            }
        }

        void removeChildren() {
            if (left) {
                left->removeChildren();
                delete left;
                left = nullptr;
            }
            if (right) {
                right->removeChildren();
                delete right;
                right = nullptr;
            }
        }

        /**
         * Recursively evaluate the expression tree where possible, reducing it to a simpler form. This does not perform dice rolls, only arithmetic simplification.
         * @return true if the node was reduced to a number
         */
        bool reduce() {
            renderedStr = "";
            if (op == OpType::NUMBER) {
                renderedStr = myToString(calcVal);
                return true;
            }

            if (op == OpType::LISTFUNCRENDERING) {
                op = OpType::NUMBER;
                renderedStr = myToString(calcVal);
                return true;
            }

            if (op == OpType::MINLISTFUNC || op == OpType::MAXLISTFUNC) {
                std::vector<ExpTree*> args = collectFuncArgs();
                bool allReduced = true;
                for (auto arg : args) {
                    if (!arg->reduce()) {
                        allReduced = false;
                    }
                }
                renderedStr = fmt::format("{}(", opToString(op, calcVal));
                for (size_t i = 0; i < args.size(); ++i) {
                    if (i) renderedStr += ",";
                    renderedStr += args[i]->renderedStr;
                }
                renderedStr += ")";
                if (allReduced) {
                    double result = op == OpType::MINLISTFUNC ? DBL_MAX : -DBL_MAX;
                    for (auto arg : args) {
                        if (op == OpType::MINLISTFUNC) {
                            result = std::min(result, arg->calcVal);
                        } else {
                            result = std::max(result, arg->calcVal);
                        }
                    }
                    calcVal = result;
                    op = OpType::LISTFUNCRENDERING;
                    renderedStr += "=" + myToString(calcVal);
                    removeChildren();
                }
                return false;
            }

            bool leftReduced = left ? left->reduce() : true;
            bool rightReduced = right ? right->reduce() : true;

            std::string leftStr = left ? left->renderedStr : "";
            std::string rightStr = right ? right->renderedStr : "";
            if (getPreority(op) > 1) {
                if (left && (left->op != OpType::NUMBER && getPreority(left->op) < getPreority(op) || getPreority(left->op) == getPreority(OpType::D_BINARY))) {
                    leftStr = "(" + leftStr + ")";
                }
                if (right && (right->op != OpType::NUMBER && getPreority(right->op) < getPreority(op) || getPreority(right->op) == getPreority(OpType::D_BINARY))) {
                    rightStr = "(" + rightStr + ")";
                }
            }
            renderedStr = fmt::format("{}{}{}", leftStr, opToString(op, calcVal), rightStr);

            if (leftReduced && rightReduced) {
                double leftVal = left ? left->calcVal : 0;
                double rightVal = right ? right->calcVal : 0;

                switch (op) {
                    case OpType::ADD: calcVal = leftVal + rightVal; break;
                    case OpType::SUB: calcVal = leftVal - rightVal; break;
                    case OpType::NEG: calcVal = -rightVal; break;
                    case OpType::MUL: calcVal = leftVal * rightVal; break;
                    case OpType::DIV: calcVal = rightVal != 0 ? leftVal / rightVal : 0; break;
                    case OpType::POW: calcVal = pow(leftVal, rightVal); break;
                    case OpType::MINFUNC: calcVal = rightVal; break;
                    case OpType::MAXFUNC: calcVal = rightVal; break;
                    default: return false;
                }

                op = OpType::NUMBER;
                renderedStr = myToString(calcVal);
                delete left;
                delete right;
                left = nullptr;
                right = nullptr;

                return true;
            }

            return false;
        }

        /**
         * Perform dice rolls in the expression tree. This modifies the tree in-place, replacing dice operations with their rolled values.
         * Only processes nodes that on the bottom.
         * @return true if any dice were rolled
         */
        bool doDice(DiceMode mode = DiceMode::SUM) {
            if (op == OpType::LISTFUNCRENDERING) {
                return true;
            }
            renderedStr = "";
            if (op == OpType::D_UNARY) {
                if (right->op != OpType::NUMBER) {
                    bool ret = right->doDice(mode);
                    // right is already dereferenced above, so the null-check is
                    // redundant (cppcheck nullPointerRedundantCheck).
                    if (right->op != OpType::NUMBER && getPreority(right->op) < getPreority(op) || getPreority(right->op) == getPreority(OpType::D_BINARY)) {
                        renderedStr = "d(" + right->renderedStr + ")";
                    } else {
                        renderedStr = "d" + right->renderedStr;
                    }
                    return ret;
                }
                int sides = right->calcVal < 0 ? right->calcVal - 0.4999 : right->calcVal + 0.4999;

                int rval = dice_random(sides);
                calcVal = rval;
                renderedStr = fmt::format("(d{}={})", sides, rval);
                op = OpType::NUMBER;
                delete right;
                right = nullptr;
                return true;
            }

            if (op == OpType::MINFUNC || op == OpType::MAXFUNC) {
                if (!right) {
                    return false;
                }
                DiceMode childMode = op == OpType::MINFUNC ? DiceMode::MIN : DiceMode::MAX;
                if (right->op != OpType::NUMBER) {
                    bool ret = right->doDice(childMode);
                    if (right->op != OpType::NUMBER) {
                        renderedStr = fmt::format("{}({})", op == OpType::MINFUNC ? "min" : "max", right->renderedStr);
                    } else {
                        renderedStr = fmt::format("{}", right->renderedStr);
                    }
                    return ret;
                }

                calcVal = right->calcVal;
                renderedStr = myToString(calcVal);
                op = OpType::NUMBER;
                delete right;
                right = nullptr;
                return true;
            }

            if (op == OpType::MINLISTFUNC || op == OpType::MAXLISTFUNC) {
                if (!left && !right) {
                    return false;
                }
                
                std::vector<ExpTree*> args = collectFuncArgs();
                bool result = false;
                for (auto arg : args) {
                    if (arg->op != OpType::NUMBER) {
                        result |= arg->doDice();
                    }
                }
                renderedStr = fmt::format("{}(", op == OpType::MINLISTFUNC ? "min" : "max");
                for (size_t i = 0; i < args.size(); ++i) {
                    if (i) renderedStr += ",";
                    renderedStr += args[i]->renderedStr;
                }
                renderedStr += ")";
                return result;
            }

            if (op == OpType::D_BINARY) {
                if (!left || !right || left->op != OpType::NUMBER || right->op != OpType::NUMBER) {
                    if (!left || !right) {
                        return false;
                    }
                    bool ret1 = left->doDice(mode);
                    bool ret2 = right->doDice(mode);
                    std::string leftStr = left ? left->renderedStr : "";
                    std::string rightStr = right ? right->renderedStr : "";
                    if (left && (left->op != OpType::NUMBER && getPreority(left->op) < getPreority(op) || getPreority(left->op) == getPreority(OpType::D_BINARY))) {
                        leftStr = "(" + leftStr + ")";
                    }
                    if (right && (right->op != OpType::NUMBER && getPreority(right->op) < getPreority(op) || getPreority(right->op) == getPreority(OpType::D_BINARY))) {
                        rightStr = "(" + rightStr + ")";
                    }
                    renderedStr = fmt::format("{}d{}", leftStr, rightStr);
                    return ret1 || ret2;
                }

                int num = left->calcVal < 0 ? left->calcVal - 0.4999 : left->calcVal + 0.4999;
                int sides = right->calcVal < 0 ? right->calcVal - 0.4999 : right->calcVal + 0.4999;

                double rval = 0;
                if (mode == DiceMode::SUM) {
                    if (num >= 100) {
                        if (sides >= 1) {
                            double mean = num * (sides + 1.0) / 2.0;
                            double stddev = sqrt(num * (1ll * sides * sides - 1) / 12.0);
                            std::normal_distribution<double> dist(mean, stddev);
                            rval = std::round(dist(drng()));
                            if (rval < num) rval = num;
                            if (rval > 1ll * num * sides) rval = 1ll * num * sides;
                        }
                        renderedStr = fmt::format("({}d{}={})", num, sides, rval);
                    } else if (num >= 20) {
                        for (int i = 0; i < num; ++i) {
                            rval += dice_random(sides);
                        }
                        renderedStr = fmt::format("({}d{}={})", num, sides, rval);
                    } else {
                        for (int i = 0; i < num; ++i) {
                            int rrval = dice_random(sides);
                            rval += rrval;
                            renderedStr += myToString(rrval);
                            if (i < num - 1) renderedStr += "+";  
                        }
                        if (num <= 1) {
                            renderedStr = fmt::format("({}d{}={})", num, sides, rval);
                        } else {
                            renderedStr = fmt::format("({}d{}={}=({}))", num, sides, rval, renderedStr);
                        }
                    }
                } else {
                    int best = mode == DiceMode::MIN ? INT_MAX : INT_MIN;
                    if (num >= 100) {
                        if (sides >= 1) {
                            double rrval = dice_random_f();
                            if (mode == DiceMode::MAX) {
                                rval = std::ceil(pow(rrval, 1.0 / num) * sides);
                            } else {
                                rval = std::max(1.0, std::floor((1.0 - pow(rrval, 1.0 / num)) * sides));
                            }
                        } else {
                            rval = 0;
                        }
                        renderedStr = fmt::format("({}d{}{}={:.0f})", num, sides, mode == DiceMode::MIN ? "min" : "max", rval);
                    } else if (num >= 20) {
                        for (int i = 0; i < num; ++i) {
                            int rrval = dice_random(sides);
                            if (mode == DiceMode::MIN) {
                                best = std::min(best, rrval);
                            } else {
                                best = std::max(best, rrval);
                            }
                        }
                        rval = best == INT_MAX || best == INT_MIN ? 0 : best;
                        renderedStr = fmt::format("({}d{}{}={:.0f})", num, sides, mode == DiceMode::MIN ? "min" : "max", rval);
                    } else {
                        std::string rolls;
                        for (int i = 0; i < num; ++i) {
                            int rrval = dice_random(sides);
                            if (i) rolls += ",";
                            rolls += myToString(rrval);
                            if (mode == DiceMode::MIN) {
                                best = std::min(best, rrval);
                            } else {
                                best = std::max(best, rrval);
                            }
                        }
                        rval = best == INT_MAX || best == INT_MIN ? 0 : best;
                        if (num <= 1) {
                            renderedStr = fmt::format("({}d{}{}={:.0f})", num, sides, mode == DiceMode::MIN ? "min" : "max", rval);
                        } else {
                            renderedStr = fmt::format("({}d{}{}={:.0f}=({}))", num, sides, mode == DiceMode::MIN ? "min" : "max", rval, rolls);
                        }
                    }
                }
                calcVal = rval;

                op = OpType::NUMBER;
                delete left;
                delete right;
                left = nullptr;
                right = nullptr;
                return true;
            }

            bool leftDone = left ? left->doDice(mode) : false;
            bool rightDone = right ? right->doDice(mode) : false;

            std::string leftStr = left ? left->renderedStr : "";
            std::string rightStr = right ? right->renderedStr : "";
            if (left && left->op != OpType::NUMBER && getPreority(left->op) < getPreority(op)) {
                leftStr = "(" + leftStr + ")";
            }
            if (right && right->op != OpType::NUMBER && getPreority(right->op) < getPreority(op)) {
                rightStr = "(" + rightStr + ")";
            }
            renderedStr = fmt::format("{}{}{}", leftStr, opToString(op, calcVal), rightStr);

            return leftDone || rightDone;
        }
    };

    std::vector<std::string> reduceAll(ExpTree* node) {
        std::vector<std::string> steps;
        if (!node) return steps;
        node->parseVal();

        while (true) {
            bool reduced = node->reduce();
            if (reduced) {
                steps.push_back(node->renderedStr);
                break;
            }
            bool diced = node->doDice();
            steps.push_back(node->renderedStr);
            if (!diced) {
                fmt::print("Warning: Could not reduce or do dice for node: {}\n", node->renderedStr);
                throw std::runtime_error("Failed to reduce or do dice for node");
            }
        }

        return steps;
    }

    class Lexer {
        std::string s;
        int pos = 0;

    public:
        Lexer(const std::string& str) : s(str) {}

        Token next() {
            while (pos < s.size() && isspace(s[pos])) pos++;

            if (pos >= s.size()) return {TokenType::END, 0};

            char c = s[pos];

            if (isdigit(c)) {
                int val = 0;
                while (pos < s.size() && isdigit(s[pos])) {
                    val = val * 10 + (s[pos++] - '0');
                }
                return {TokenType::NUM, val};
            }

            if (std::isalpha(c)) {
                std::string ident;
                while (pos < s.size() && std::isalpha(s[pos])) {
                    ident.push_back(std::tolower(s[pos++]));
                }
                if (ident == "d") {
                    return {TokenType::D, 0};
                }
                if (ident == "min") {
                    return {TokenType::MIN_FUNC, 0};
                }
                if (ident == "max") {
                    return {TokenType::MAX_FUNC, 0};
                }
                throw std::runtime_error("Invalid identifier");
            }

            pos++;
            switch (c) {
                case '+': return {TokenType::PLUS, 0};
                case '-': return {TokenType::MINUS, 0};
                case '*': return {TokenType::MUL, 0};
                case '/': return {TokenType::DIV, 0};
                case '^': return {TokenType::POW, 0};
                case '(': return {TokenType::LPAREN, 0};
                case ')': return {TokenType::RPAREN, 0};
                case ',': return {TokenType::COMMA, 0};
            }

            throw std::runtime_error("Invalid character");
        }
    };
    class Parser {
        // expr        = add_sub
        // add_sub     = mul_div ((+|-) mul_div)*
        // mul_div     = power ((*|/) power)*
        // power       = d_level (^ d_level)*
        // d_level     = (+|-) d_level | (d d_level) | primary (d d_level)*
        // primary     = number | '('expr')' | min'('expr_list')' | max'('expr_list')' | min'('expr')' | max'('expr')'
        // expr_list   = add_sub (',' add_sub)*
        Lexer lexer;
        Token cur;

        void next() { cur = lexer.next(); }

    public:
        Parser(const std::string& s) : lexer(s) {
            next();
        }

        ExpTree* parse() {
            return parseAddSub();
        }

    private:
        ExpTree* parseAddSub() {
            auto node = parseMulDiv();
            while (cur.type == TokenType::PLUS || cur.type == TokenType::MINUS) {
                TokenType op = cur.type;
                next();
                auto right = parseMulDiv();
                node = new ExpTree(
                    op == TokenType::PLUS ? OpType::ADD : OpType::SUB,
                    node, right
                );
            }
            return node;
        }

        ExpTree* parseMulDiv() {
            auto node = parsePower();
            while (cur.type == TokenType::MUL || cur.type == TokenType::DIV) {
                TokenType op = cur.type;
                next();
                auto right = parsePower();
                node = new ExpTree(
                    op == TokenType::MUL ? OpType::MUL : OpType::DIV,
                    node, right
                );
            }
            return node;
        }

        ExpTree* parsePower() {
            auto node = parseDLevel();
            while (cur.type == TokenType::POW) {
                next();
                auto right = parseDLevel();
                node = new ExpTree(OpType::POW, node, right);
            }
            return node;
        }

        ExpTree* parseDLevel() {
            if (cur.type == TokenType::PLUS) {
                next();
                return parseDLevel();
            }
            if (cur.type == TokenType::MINUS) {
                next();
                auto right = parseDLevel();
                return new ExpTree(OpType::NEG, nullptr, right);
            }
            ExpTree* node = nullptr;

            if (cur.type == TokenType::D) {
                next();
                auto right = parseDLevel();
                node = new ExpTree(OpType::D_UNARY, nullptr, right);
            } else {
                node = parsePrimary();
            }

            while (cur.type == TokenType::D) {
                next();
                auto right = parseDLevel();
                node = new ExpTree(OpType::D_BINARY, node, right);
            }

            return node;
        }

        ExpTree* parsePrimary() {
            if (cur.type == TokenType::NUM) {
                int val = cur.value;
                next();
                return new ExpTree(val);
            }

            if (cur.type == TokenType::LPAREN) {
                next();
                auto node = parseAddSub();
                if (cur.type != TokenType::RPAREN)
                    throw std::runtime_error("Missing )");
                next();
                return node;
            }

            if (cur.type == TokenType::MIN_FUNC || cur.type == TokenType::MAX_FUNC) {
                OpType func = cur.type == TokenType::MIN_FUNC ? OpType::MINFUNC : OpType::MAXFUNC;
                next();
                if (cur.type != TokenType::LPAREN) {
                    throw std::runtime_error("Missing (");
                }
                next();
                auto node = parseAddSub();
                while (cur.type == TokenType::COMMA) {
                    if (func == OpType::MINFUNC) func = OpType::MINLISTFUNC;
                    else if (func == OpType::MAXFUNC) func = OpType::MAXLISTFUNC;
                    next();
                    auto nextArg = parseAddSub();
                    node = new ExpTree(func, node, nextArg);
                }
                if (cur.type != TokenType::RPAREN) {
                    throw std::runtime_error("Missing )");
                }
                next();
                if (func == OpType::MINFUNC || func == OpType::MAXFUNC) {
                    return new ExpTree(func, nullptr, node);
                } else {
                    return node;
                }
            }

            throw std::runtime_error("Invalid expression");
        }
    };
};