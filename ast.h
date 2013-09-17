#ifndef AST_H
#define AST_H

#include <cstdint>
#include <memory>
#include <vector>
#include <sstream>
#include <functional>
#include <algorithm>
#include "token.h"
#include "jit_engine.h"
#include "func_manager.h"
#include "eval_t.h"

class ast_expr;
class ast_proto;
class ast_func;

typedef std::shared_ptr<ast_expr> shared_expr;
typedef std::shared_ptr<ast_proto> shared_proto;
typedef std::shared_ptr<ast_func> shared_func;

class ast {
public:
	ast() {}
	ast(const tok& token) : _tok(token) {}
	virtual ~ast() {}

	int node_type() const {return _tok.type();}
    std::string node_str() const {return _tok.str();}
    int num_nodes() const {return static_cast<int>(_nodes.size());}
	void add_node(const std::shared_ptr<ast>& c) {_nodes.push_back(c);}

	eval_t eval_type() const {return _eval_type;}
	void set_eval_type(eval_t type) {_eval_type = type;}

    const std::shared_ptr<ast>& operator[](unsigned int n) const {return _nodes[n];}

    void for_nodes(std::function<void(const std::shared_ptr<ast>&)> func) const
	{
		for(const auto& i : _nodes)
			func(i);
	}

	virtual std::string to_str() const
    {
		std::stringstream ss;
		ss << "<'" << _tok.str() << "', " << _tok.type() << ", " << eval_strings[static_cast<int>(_eval_type)] << ">";
		return ss.str();
    }

    virtual llvm::Value *gen_val(jit_engine*) {return nullptr;}
    virtual llvm::Function *gen_func(jit_engine*) {return nullptr;}

    virtual eval_t resolve_types(jit_engine& je) {return je.resolve_types(*this);}
private:
	tok _tok;
	std::vector<std::shared_ptr<ast>> _nodes;
	eval_t _eval_type;
};

class ast_expr : public ast {
public:
	ast_expr(const tok& token) : ast(token) {set_eval_type(eval_t::ev_invalid);}

	/*virtual std::string to_str() const
	{
		if(eval_type() == eval_t::ev_int64) {
			return ast::to_str() + "<ev_int64>";
		}
		return ast::to_str();
	}*/
};

class expr_int : public ast_expr { //tok::val_int
public:
	expr_int(const tok& token)
		: ast_expr(token) {set_eval_type(eval_t::ev_int64);}

    llvm::Value *gen_val(jit_engine *je)
    {
        //std::cout << "expr_int" << std::endl;
        return je->visitor_gen_val(*this);
    }
};

class expr_float : public ast_expr { //tok::val_int
public:
	expr_float(const tok& token)
		: ast_expr(token) {set_eval_type(eval_t::ev_float);}

    llvm::Value *gen_val(jit_engine *je)
    {
        //std::cout << "expr_float" << std::endl;
        return je->visitor_gen_val(*this);
    }
};

class expr_var : public ast_expr { //tok::var
public:
	expr_var(const tok& token, eval_t ev)
		: ast_expr(token) {set_eval_type(ev);}

    llvm::Value *gen_val(jit_engine *je)
    {
        //std::cout << "expr_var" << std::endl;
        return je->visitor_gen_val(*this);
    }
};

class expr_call : public ast_expr { //tok::call
public:
	expr_call(const tok& token, const std::vector<shared_expr>& args, eval_t ev)
		: ast_expr(token)
	{
		for(const auto& i : args)
			add_node(i);

		set_eval_type(ev);
	}

    llvm::Value *gen_val(jit_engine *je)
    {
        //std::cout << "expr_call" << std::endl;
        return je->visitor_gen_val(*this);
    }

    eval_t resolve_types(jit_engine& je) {return je.resolve_types(*this);}
};

class expr_if : public ast_expr { //tok::t_if
public:
    expr_if(const tok& token, const shared_expr& cond, const shared_expr& thenexpr, const shared_expr& elseexpr)
        : ast_expr(token)
    {
        add_node(cond);
        add_node(thenexpr);
        add_node(elseexpr);

        auto t1 = thenexpr->eval_type();
        auto t2 = elseexpr->eval_type();
        if(t1 == t2) {
            set_eval_type(t1);
        } //else if(t1 
    }

    llvm::Value *gen_val(jit_engine *je)
    {
        //std::cout << "expr_if" << std::endl;
        return je->visitor_gen_val(*this);
    }

    eval_t resolve_types(jit_engine& je) {return je.resolve_types(*this);}
};

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
		} else if((t1 == eval_t::ev_int64 && t2 == eval_t::ev_float)
                || (t1 == eval_t::ev_float && t2 == eval_t::ev_int64)) {
            set_eval_type(eval_t::ev_int64);
        }
	}

    //llvm::Value *gen_val(jit_engine *je) {return je->visitor_gen_val(*this);}
};


class binop_add : public expr_binop {
public:
    binop_add(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    llvm::Value *gen_val(jit_engine *je)
    {
        //std::cout << "binop_add" << std::endl;
        return je->visitor_gen_val(*this);
    }
};

class binop_sub : public expr_binop {
public:
    binop_sub(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    llvm::Value *gen_val(jit_engine *je)
    {
        //std::cout << "binop_sub" << std::endl;
        return je->visitor_gen_val(*this);
    }
};

class binop_mul : public expr_binop {
public:
    binop_mul(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    llvm::Value *gen_val(jit_engine *je)
    {
        //std::cout << "binop_mul" << std::endl;
        return je->visitor_gen_val(*this);
    }
};

class binop_div : public expr_binop {
public:
    binop_div(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    llvm::Value *gen_val(jit_engine *je)
    {
        //std::cout << "binop_div" << std::endl;
        return je->visitor_gen_val(*this);
    }
};

class binop_lt : public expr_binop {
public:
    binop_lt(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    llvm::Value *gen_val(jit_engine *je)
    {
        return je->visitor_gen_val(*this);
    }
};

class binop_gt : public expr_binop {
public:
    binop_gt(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    llvm::Value *gen_val(jit_engine *je)
    {
        return je->visitor_gen_val(*this);
    }
};

class binop_lte : public expr_binop {
public:
    binop_lte(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    llvm::Value *gen_val(jit_engine *je)
    {
        return je->visitor_gen_val(*this);
    }
};

class binop_gte : public expr_binop {
public:
    binop_gte(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    llvm::Value *gen_val(jit_engine *je)
    {
        return je->visitor_gen_val(*this);
    }
};

class binop_eq : public expr_binop {
public:
    binop_eq(const tok& token, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(token, lhs, rhs) {}

    llvm::Value *gen_val(jit_engine *je)
    {
        return je->visitor_gen_val(*this);
    }
};

class ast_proto : public ast { //tok::proto
public:
	ast_proto(const tok& token, const std::vector<std::shared_ptr<expr_var>>& args)
		: ast(token)
	{
		for(const auto& i : args)
			add_node(i);
	}

    llvm::Function *gen_func(jit_engine *je)
    {
        return je->visitor_gen_func(*this);
    }
};

class proto_anon : public ast_proto {
public:
    proto_anon(const tok& token, const std::vector<std::shared_ptr<expr_var>>& args)
        : ast_proto(token, args) {}

    llvm::Function *gen_func(jit_engine *je)
    {
        return je->visitor_gen_func(*this);
    }
};

/*class proto_template : public ast_proto {
public:
    proto_template(const tok& token, const std::vector<std::shared_ptr<expr_var>>& args)
        : ast_proto(token, args) {set_eval_type(eval_t::ev_template);}

        llvm::Function *gen_func(jit_engine *je)
    {
        return je->visitor_gen_func(*this);
    }
};*/

class ast_func : public ast { //tok::def
public:
	ast_func(const shared_proto& proto, const shared_expr& body)
		: ast(tok(tok::def, ""))
	{
		add_node(proto);
		add_node(body);
	}

    llvm::Function *gen_func(jit_engine *je)
    {
        std::cout << "gen_func ast_func" << std::endl;
        return je->visitor_gen_func(*this);
    }
};

class func_anon : public ast_func {
public:
    func_anon(const std::shared_ptr<proto_anon>& proto, const shared_expr& body)
        : ast_func(proto, body) {}

    llvm::Function *gen_func(jit_engine *je)
    {
        if(llvm::Function *f = je->visitor_gen_func(*this)) {
            /*void *pfunc = je->getPointerToFunction(f);
            std::int64_t (*fp)() = (std::int64_t (*)())pfunc;
            fp();
            je->freeMachineCodeForFunction(f);*/
            return f;
        }
        return nullptr;
    }
};

class func_template : public ast_func {
public:
    func_template(const std::shared_ptr<ast_proto>& proto, const shared_expr& body)
        : ast_func(proto, body) {}

    llvm::Function *gen_func(jit_engine*)
    {
        //return je->visitor_gen_func(*this);
        return nullptr;
    }
};

#endif
