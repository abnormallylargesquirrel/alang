#ifndef BINOP_H
#define BINOP_H

#include <llvm/IR/IRBuilder.h>

class expr_binop : public ast_expr { //'+' | '-' | '*' | '/' | '<' | '>'
public:
	expr_binop(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
		: ast_expr(token)
	{
		add_node(lhs);
		add_node(rhs);

		auto t1 = lhs->eval_type();
		auto t2 = rhs->eval_type();
		if(t1 == t2) {
			set_eval_type(t1);
		}
	}

    virtual Value *gen_binop(IRBuilder<> *builder) {return nullptr;}
};


class binop_add : public expr_binop {
public:
    binop_add(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    Value *gen_binop(IRBuilder<> *builder)
    {
        return builder->CreateAdd(side[0], side[1], "addtmp");
    }
};

class binop_sub : public expr_binop {
public:
    binop_sub(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    Value *gen_binop(IRBuilder<> *builder)
    {
        return builder->CreateSub(side[0], side[1], "subtmp");
    }
};

class binop_mul : public expr_binop {
public:
    binop_mul(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    Value *gen_binop(IRBuilder<> *builder)
    {
        return builder->CreateMul(side[0], side[1], "multmp");
    }
};

class binop_div : public expr_binop {
public:
    binop_div(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    Value *gen_binop(IRBuilder<> *builder)
    {
        return builder->CreateSDiv(side[0], side[1], "divtmp");
    }
};

class binop_lt : public expr_binop {
public:
    binop_lt(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    Value *gen_binop(IRBuilder<> *builder)
    {
        side[0] = builder->CreateICmpSLT(side[0], side[1], "lttmp");
        return builder->CreateSExt(side[0], Type::getInt64Ty(getGlobalContext()));
    }
};

class binop_gt : public expr_binop {
public:
    binop_gt(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    Value *gen_binop(IRBuilder<> *builder)
    {
        side[0] = builder->CreateICmpSGT(side[0], side[1], "gttmp");
        return builder->CreateSExt(side[0], Type::getInt64Ty(getGlobalContext()));
    }
};

class binop_lte : public expr_binop {
public:
    binop_lte(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    Value *gen_binop(IRBuilder<> *builder)
    {
        side[0] = builder->CreateICmpSLE(side[0], side[1], "ltetmp");
        return builder->CreateSExt(side[0], Type::getInt64Ty(getGlobalContext()));
    }
};

class binop_gte : public expr_binop {
public:
    binop_gte(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    Value *gen_binop(IRBuilder<> *builder)
    {
        side[0] = builder->CreateICmpSGE(side[0], side[1], "gtetmp");
        return builder->CreateSExt(side[0], Type::getInt64Ty(getGlobalContext()));
    }
};

class binop_eq : public expr_binop {
public:
    binop_eq(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    Value *gen_binop(IRBuilder<> *builder)
    {
        side[0] = builder->CreateICmpEQ(side[0], side[1], "eqtmp");
        return builder->CreateSExt(side[0], Type::getInt64Ty(getGlobalContext()));
    }
};


#endif
