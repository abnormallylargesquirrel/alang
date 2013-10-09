#ifndef AST_H
#define AST_H

#include <cstdint>
#include <memory>
#include <vector>
#include <sstream>
#include <functional>
#include <algorithm>
#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>
//#include "hm_unification.h"
#include "token.h"
#include "jit_engine.h"
#include "func_manager.h"
#include "eval_t.h"

class ast_expr;
class ast_proto;
class ast_func;

struct inferencer;

class type_variable;
class type_operator;

typedef boost::variant<
    type_variable,
    boost::recursive_wrapper<type_operator>
> type;

typedef std::shared_ptr<ast> shared_ast;
typedef std::shared_ptr<ast_expr> shared_expr;
typedef std::shared_ptr<ast_proto> shared_proto;
typedef std::shared_ptr<ast_func> shared_func;
typedef std::shared_ptr<func_template> shared_template;

class ast {
public:
	ast() {}
    ast(const std::string& name) : _node_str(name) {}
	virtual ~ast() {}

    //std::string node_str() const {return _tok.str();}
    const std::string& node_str() const {return _node_str;}
    int num_nodes() const {return static_cast<int>(_nodes.size());}
	void add_node(const shared_ast& c) {_nodes.push_back(c);}

	eval_t eval_type() const {return _eval_type;}
	void set_eval_type(eval_t type) {_eval_type = type;}

    shared_ast& operator[](unsigned int n) {return _nodes[n];}

    void for_nodes(std::function<void(const shared_ast&)> func) const
	{
		for(const auto& i : _nodes)
			func(i);
	}

    void for_nodes(std::function<void(shared_ast&)> func)
	{
		for(auto& i : _nodes)
			func(i);
	}

	virtual std::string to_str() const
    {
		std::stringstream ss;
		ss << "<'" << node_str() << "', " << eval_strings[static_cast<int>(eval_type())] << ">";
		return ss.str();
    }

    virtual void clone(shared_ast& node) {node = std::make_shared<ast>(*this);}

    virtual type infer_type(inferencer& inf);

    /*virtual llvm::Value *gen_val(jit_engine&) {return nullptr;}
    virtual llvm::Function *gen_func(jit_engine&) {return nullptr;}*/

    //virtual eval_t resolve_types(jit_engine&) {return je.resolve_types(*this);}
private:
	//tok _tok;
    std::string _node_str;
	std::vector<shared_ast> _nodes;
	eval_t _eval_type;
};

inline std::ostream& operator<<(std::ostream& os, const shared_ast& n)
{
    return os << n->node_str();
}

class ast_expr : public ast {
public:
	ast_expr(const std::string& name) : ast(name) {set_eval_type(eval_t::ev_invalid);}
};

class expr_int : public ast_expr { //tok::val_int
public:
	expr_int(const std::string& name)
		: ast_expr(name) {set_eval_type(eval_t::ev_int64);}

    virtual void clone(std::shared_ptr<expr_int>& node) {node = std::make_shared<expr_int>(*this);}
    virtual type infer_type(inferencer& inf);

    /*llvm::Value *gen_val(jit_engine&)
    {
        //std::cout << "expr_int" << std::endl;
        //return je.visitor_gen_val(*this);
        return nullptr;
    }*/
};

class expr_float : public ast_expr { //tok::val_int
public:
	expr_float(const std::string& name)
		: ast_expr(name) {set_eval_type(eval_t::ev_float);}

    virtual void clone(std::shared_ptr<expr_float>& node) {node = std::make_shared<expr_float>(*this);}
    virtual type infer_type(inferencer& inf);

    /*llvm::Value *gen_val(jit_engine&)
    {
        //std::cout << "expr_float" << std::endl;
        //return je.visitor_gen_val(*this);
        return nullptr;
    }*/
};

/*class expr_var : public ast_expr { //tok::var
public:
	expr_var(const std::string& name, eval_t ev)
		: ast_expr(name) {set_eval_type(ev);}

    llvm::Value *gen_val(jit_engine&)
    {
        //std::cout << "expr_var" << std::endl;
        return je.visitor_gen_val(*this);
    }
};*/

class expr_sym : public ast_expr {
public:
    expr_sym(const std::string& name)
        : ast_expr(name) {}

    virtual void clone(std::shared_ptr<expr_sym>& node) {node = std::make_shared<expr_sym>(*this);}
    virtual type infer_type(inferencer& inf);

    /*llvm::Value *gen_val(jit_engine&)
    {
        throw std::runtime_error("expr_sym gen_val called");
    }*/
};

class expr_call : public ast_expr { //tok::call
public:
	expr_call(const std::string& name, const std::vector<shared_expr>& args)
		: ast_expr(name)
	{
		for(const auto& i : args)
			add_node(i);
	}

    virtual void clone(std::shared_ptr<expr_call>& node) {node = std::make_shared<expr_call>(*this);}

    /*llvm::Value *gen_val(jit_engine&)
    {
        //std::cout << "expr_call" << std::endl;
        //return je.visitor_gen_val(*this);
        return nullptr;
    }*/

    //eval_t resolve_types(jit_engine&) {return je.resolve_types(*this);}
};

class expr_apply : public ast_expr {
public:
    expr_apply(const shared_expr& func, const shared_expr& arg)
        : ast_expr("apply " + func->node_str())
    {
        add_node(func);
        add_node(arg);
    }

    virtual void clone(std::shared_ptr<expr_apply>& node) {node = std::make_shared<expr_apply>(*this);}
    virtual type infer_type(inferencer& inf);

    /*std::shared_ptr<ast> function(func_manager& fm)
    {
        if(_function.which())
            return boost::get<std::shared_ptr<ast>>(_function);

        if(auto ret = std::static_pointer_cast<ast>(fm.lookup_template(boost::get<std::string>(_function))))
            return ret;

        throw std::runtime_error("Could not resolve function in application");
    }*/

    /*llvm::Value *gen_val(jit_engine&)
    {
        throw std::runtime_error("gen_val called on apply");
    }*/
};

class expr_if : public ast_expr { //tok::t_if
public:
    expr_if(const std::string& name, const shared_expr& cond, const shared_expr& thenexpr, const shared_expr& elseexpr)
        : ast_expr(name)
    {
        add_node(cond);
        add_node(thenexpr);
        add_node(elseexpr);

        /*auto t1 = thenexpr->eval_type();
        auto t2 = elseexpr->eval_type();
        if(t1 == t2) {
            set_eval_type(t1);
        }*/
    }

    virtual void clone(std::shared_ptr<expr_if>& node) {node = std::make_shared<expr_if>(*this);}
    virtual type infer_type(inferencer& inf);

    /*llvm::Value *gen_val(jit_engine&)
    {
        //std::cout << "expr_if" << std::endl;
        //return je.visitor_gen_val(*this);
        return nullptr;
    }*/

    //eval_t resolve_types(jit_engine&) {return je.resolve_types(*this);}
};

class expr_binop : public ast_expr { //'+' | '-' | '*' | '/' | '<' | '>'
public:
	expr_binop(const std::string& name, const shared_expr& lhs, const shared_expr& rhs)
		: ast_expr(name)
	{
		add_node(lhs);
		add_node(rhs);

		auto t1 = lhs->eval_type();
		auto t2 = rhs->eval_type();
		if(t1 == t2) {
			set_eval_type(t1);
		}/* else if((t1 == eval_t::ev_int64 && t2 == eval_t::ev_float)
                || (t1 == eval_t::ev_float && t2 == eval_t::ev_int64)) {
            set_eval_type(eval_t::ev_int64);
        }*/
	}

    virtual void clone(std::shared_ptr<expr_binop>& node) {node = std::make_shared<expr_binop>(*this);}
    virtual type infer_type(inferencer& inf);

    //llvm::Value *gen_val(jit_engine&) {return je.visitor_gen_val(*this);}
};


class binop_add : public expr_binop {
public:
    binop_add(const std::string& name, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(name, lhs, rhs) {}

    virtual void clone(std::shared_ptr<binop_add>& node) {node = std::make_shared<binop_add>(*this);}

    /*llvm::Value *gen_val(jit_engine&)
    {
        //std::cout << "binop_add" << std::endl;
        //return je.visitor_gen_val(*this);
        return nullptr;
    }*/
};

class binop_sub : public expr_binop {
public:
    binop_sub(const std::string& name, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(name, lhs, rhs) {}

    virtual void clone(std::shared_ptr<binop_sub>& node) {node = std::make_shared<binop_sub>(*this);}

    /*llvm::Value *gen_val(jit_engine&)
    {
        //std::cout << "binop_sub" << std::endl;
        //return je.visitor_gen_val(*this);
        return nullptr;
    }*/
};

class binop_mul : public expr_binop {
public:
    binop_mul(const std::string& name, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(name, lhs, rhs) {}

    virtual void clone(std::shared_ptr<binop_mul>& node) {node = std::make_shared<binop_mul>(*this);}

    /*llvm::Value *gen_val(jit_engine&)
    {
        //std::cout << "binop_mul" << std::endl;
        //return je.visitor_gen_val(*this);
        return nullptr;
    }*/
};

class binop_div : public expr_binop {
public:
    binop_div(const std::string& name, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(name, lhs, rhs) {}

    virtual void clone(std::shared_ptr<binop_div>& node) {node = std::make_shared<binop_div>(*this);}

    /*llvm::Value *gen_val(jit_engine&)
    {
        //std::cout << "binop_div" << std::endl;
        //return je.visitor_gen_val(*this);
        return nullptr;
    }*/
};

class binop_lt : public expr_binop {
public:
    binop_lt(const std::string& name, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(name, lhs, rhs) {}

    virtual void clone(std::shared_ptr<binop_lt>& node) {node = std::make_shared<binop_lt>(*this);}

    /*llvm::Value *gen_val(jit_engine&)
    {
        //return je.visitor_gen_val(*this);
        return nullptr;
    }*/
};

class binop_gt : public expr_binop {
public:
    binop_gt(const std::string& name, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(name, lhs, rhs) {}

    virtual void clone(std::shared_ptr<binop_gt>& node) {node = std::make_shared<binop_gt>(*this);}

    /*llvm::Value *gen_val(jit_engine&)
    {
        //return je.visitor_gen_val(*this);
        return nullptr;
    }*/
};

class binop_lte : public expr_binop {
public:
    binop_lte(const std::string& name, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(name, lhs, rhs) {}

    virtual void clone(std::shared_ptr<binop_lte>& node) {node = std::make_shared<binop_lte>(*this);}

    /*llvm::Value *gen_val(jit_engine&)
    {
        //return je.visitor_gen_val(*this);
        return nullptr;
    }*/
};

class binop_gte : public expr_binop {
public:
    binop_gte(const std::string& name, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(name, lhs, rhs) {}

    virtual void clone(std::shared_ptr<binop_gte>& node) {node = std::make_shared<binop_gte>(*this);}

    /*llvm::Value *gen_val(jit_engine&)
    {
        //return je.visitor_gen_val(*this);
        return nullptr;
    }*/
};

class binop_eq : public expr_binop {
public:
    binop_eq(const std::string& name, const shared_expr& lhs, const shared_expr& rhs)
        : expr_binop(name, lhs, rhs) {}

    virtual void clone(std::shared_ptr<binop_eq>& node) {node = std::make_shared<binop_eq>(*this);}

    /*llvm::Value *gen_val(jit_engine&)
    {
        //return je.visitor_gen_val(*this);
        return nullptr;
    }*/
};

class ast_proto : public ast { //tok::proto
public:
	ast_proto(const std::string& name, const std::vector<std::shared_ptr<expr_sym>>& args)
		: ast(name)
	{
		for(const auto& i : args)
			add_node(i);
	}

    virtual void clone(shared_proto& node) {node = std::make_shared<ast_proto>(*this);}

    /*llvm::Function *gen_func(jit_engine&)
    {
        //return je.visitor_gen_func(*this);
        return nullptr;
    }*/
};

class proto_anon : public ast_proto {
public:
    proto_anon(const std::string& name, const std::vector<std::shared_ptr<expr_sym>>& args)
        : ast_proto(name, args) {}

    virtual void clone(std::shared_ptr<proto_anon>& node) {node = std::make_shared<proto_anon>(*this);}

    /*llvm::Function *gen_func(jit_engine&)
    {
        //return je.visitor_gen_func(*this);
        return nullptr;
    }*/
};

class ast_func : public ast { //tok::def
public:
	ast_func(const shared_proto& proto, const shared_expr& body)
		: ast(proto->node_str())
	{
		add_node(proto);
		add_node(body);
	}

    virtual void clone(shared_func& node) {node = std::make_shared<ast_func>(*this);}
    virtual type infer_type(inferencer& inf);

    /*llvm::Function *gen_func(jit_engine&)
    {
        throw std::runtime_error("gen_func ast_func");
        //return je.visitor_gen_func(*this);
    }*/
};

class func_anon : public ast_func {
public:
    func_anon(const std::shared_ptr<proto_anon>& proto, const shared_expr& body)
        : ast_func(proto, body) {}

    virtual void clone(std::shared_ptr<func_anon>& node) {node = std::make_shared<func_anon>(*this);}

    /*llvm::Function *gen_func(jit_engine&)
    {
        if(llvm::Function *f = je.visitor_gen_func(*this)) {
            void *pfunc = je.getPointerToFunction(f);
            std::int64_t (*fp)() = (std::int64_t (*)())pfunc;
            fp();
            je.freeMachineCodeForFunction(f);
            return f;
        }
        return nullptr;
        //return je.visitor_gen_func(*this);
        return nullptr;
    }*/
};

class func_template : public ast_func {
public:
    func_template(const shared_proto& proto, const shared_expr& body)
        : ast_func(proto, body) {}

    virtual void clone(shared_template& node) {node = std::make_shared<func_template>(*this);}

    /*llvm::Function *gen_func(jit_engine&)
    {
        return nullptr;
    }*/
};

#endif
