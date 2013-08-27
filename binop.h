#ifndef BINOP_H
#define BINOP_H

#include <llvm/IR/IRBuilder.h>

class expr_binop;

class binop_add : public expr_binop {
public:
    binop_add(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}
};

class binop_sub : public expr_binop {
public:
    binop_sub(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}
};

class binop_mul : public expr_binop {
public:
    binop_mul(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}
};

class binop_div : public expr_binop {
public:
    binop_div(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}
};

class binop_lt : public expr_binop {
public:
    binop_lt(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}
};

class binop_gt : public expr_binop {
public:
    binop_gt(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}
};

class binop_lte : public expr_binop {
public:
    binop_lte(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}
};

class binop_gte : public expr_binop {
public:
    binop_gte(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}
};


#endif
