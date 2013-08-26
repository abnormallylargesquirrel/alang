#ifndef AST_H
#define AST_H

#include <cstdint>
#include <memory>
#include <vector>
#include <sstream>
#include <functional>
#include <algorithm>
#include "token.h"

class ast_expr;
class ast_proto;
class ast_func;

typedef std::shared_ptr<ast_expr> shared_expr;
typedef std::shared_ptr<ast_proto> shared_proto;
typedef std::shared_ptr<ast_func> shared_func;

enum class eval_t {invalid, integer};

class ast {
public:
	ast() {}
	ast(const tok& token) : _tok(token) {}
	virtual ~ast() {}

	int node_type() const {return _tok.type();}
    std::string node_str() const {return _tok.str();}
    bool node_literal_signed() const {return _tok.literal_signed();}
    int num_nodes() const {return static_cast<int>(_nodes.size());}
	void add_node(const std::shared_ptr<ast>& c) {_nodes.push_back(c);}

    const std::shared_ptr<ast>& operator[](unsigned int n) const {return _nodes[n];}

    //template<class T>
    //void for_nodes(T func) const
    void for_nodes(std::function<void(const std::shared_ptr<ast>&)> func) const
	{
		for(const auto& i : _nodes)
			func(i);
	}

	virtual std::string to_str() const {return _tok.to_str();}
private:
	tok _tok;
	std::vector<std::shared_ptr<ast>> _nodes;
};

class ast_expr : public ast {
public:
	ast_expr(const tok& token) : ast(token), _eval_type(eval_t::invalid) {}
	virtual ~ast_expr() {}

	eval_t eval_type() {return _eval_type;}
	void set_eval_type(eval_t type) {_eval_type = type;}

	virtual std::string to_str() const
	{
		if(_eval_type == eval_t::integer) {
			return ast::to_str() + "<ev_int>";
		}
		return ast::to_str();
	}
private:
	eval_t _eval_type;
};

class expr_int : public ast_expr { //tok::val_int
public:
	expr_int(const tok& token)
		: ast_expr(token) {set_eval_type(eval_t::integer);}
};

class expr_var : public ast_expr { //tok::var
public:
	expr_var(const tok& token, eval_t ev)
		: ast_expr(token) {set_eval_type(ev);}
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
		}
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
        }
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
};

class ast_func : public ast { //tok::def
public:
	ast_func(const shared_proto& proto, const shared_expr& body)
		: ast(tok(tok::def, ""))
	{
		add_node(proto);
		add_node(body);
	}
};

#endif
