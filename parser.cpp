#include <iostream>
#include <string>
#include <memory>
#include "utils.h"
#include "parser.h"
#include "lexer.h"

shared_expr parser::p_expr_int()
{
	auto ret = std::make_shared<expr_int>(_lex.la(0).str());
	_lex.next_token();
	return ret;
}

shared_expr parser::p_expr_float()
{
    auto ret = std::make_shared<expr_float>(_lex.la(0).str());
    _lex.next_token();
    return ret;
}

shared_expr parser::p_paren_expr()
{
	_lex.next_token(); //(
	auto a = p_expr();

	if(_lex.la(0).type() != ')')
        throw std::runtime_error("Expected ')' in paren expression");

	_lex.next_token(); //)
	return a;
}

shared_expr parser::p_id_expr(bool single_sym)
{
    std::string ret = _lex.la(0).str();
	_lex.next_token(); //id

    auto sym = std::make_shared<expr_sym>(ret);
    if(single_sym)
        return sym;

    //apply
    std::shared_ptr<expr_apply> app;
    int type = _lex.la(0).type();
    if(type == tok::id || type == tok::literal_int
            || type == tok::literal_float || type == tok::t_if || type == '(') {
        //std::cout << "first apply" << std::endl;
        //std::cout << sym->node_str() << std::endl;
        auto e = p_primary(true);
        //std::cout << e->node_str() << std::endl;
        app = std::make_shared<expr_apply>(sym, e);
        type = _lex.la(0).type();
    } else {
        return sym;
    }
    while(type == tok::id || type == tok::literal_int
            || type == tok::literal_float || type == tok::t_if || type == '(') {
        auto e = p_primary(true);
        //std::cout << e->node_str() << std::endl;
        app = std::make_shared<expr_apply>(app, e);
        type = _lex.la(0).type();
    }

    return app;
}

shared_expr parser::p_primary(bool single_sym)
{
	switch(_lex.la(0).type()) {
	case tok::id: return p_id_expr(single_sym);
	case tok::literal_int: return p_expr_int();
    case tok::literal_float: return p_expr_float();
    case tok::t_if: return p_if();
	case '(': return p_paren_expr();
    default: throw std::runtime_error("Unknown token, expecting primary expression");
	}
}

shared_expr parser::p_expr()
{
	shared_expr lhs = p_primary();
	return p_expr_binop(0, lhs);
}

shared_expr parser::p_expr_binop(int expr_prec, shared_expr lhs)
{
	while(1) {
        tok ret_tok = _lex.la(0);
        std::string ret = ret_tok.str();
		int tok_prec = ret_tok.prec();

		if(tok_prec < expr_prec)
			return lhs;

		_lex.next_token(); //binop character

		shared_expr rhs = p_primary();

		int next_prec = _lex.la(0).prec();
		if(tok_prec < next_prec) {
			rhs = p_expr_binop(tok_prec + 1, rhs);
		}

        eval_t op_type;
        if(ret.back() == '.')
            op_type = eval_t::ev_float;
        else
            op_type = eval_t::ev_int;

        switch(ret_tok.type()) {
            case tok::add:
                lhs = std::make_shared<binop_add>(ret, lhs, rhs, op_type);
                break;
            case tok::sub:
                lhs = std::make_shared<binop_sub>(ret, lhs, rhs, op_type);
                break;
            case tok::mul:
                lhs = std::make_shared<binop_mul>(ret, lhs, rhs, op_type);
                break;
            case tok::div:
                lhs = std::make_shared<binop_div>(ret, lhs, rhs, op_type);
                break;
            case tok::lt:
                lhs = std::make_shared<binop_lt>(ret, lhs, rhs, op_type);
                break;
            case tok::gt:
                lhs = std::make_shared<binop_gt>(ret, lhs, rhs, op_type);
                break;
            case tok::lte:
                lhs = std::make_shared<binop_lte>(ret, lhs, rhs, op_type);
                break;
            case tok::gte:
                lhs = std::make_shared<binop_gte>(ret, lhs, rhs, op_type);
                break;
            case tok::eq:
                lhs = std::make_shared<binop_eq>(ret, lhs, rhs, op_type);
                break;
            default:
                throw std::runtime_error("Unknown token type as binary operator");
        }
	}
}

shared_expr parser::p_if()
{
    std::string ret = _lex.la(0).str();
    _lex.next_token(); //'if'

    shared_expr cond = p_expr();

    if(_lex.la(0).type() != tok::t_then)
        throw std::runtime_error("Expected 'then' in if");

    _lex.next_token(); //'then'

    shared_expr thenexpr = p_expr();

    if(_lex.la(0).type() != tok::t_else)
        throw std::runtime_error("Expected 'else' in if");

    _lex.next_token(); //'else'

    shared_expr elseexpr = p_expr();

    return std::make_shared<expr_if>(ret, cond, thenexpr, elseexpr);
}

shared_proto parser::p_decl()
{
    _lex.next_token(); //'decl'

    auto p = p_proto();
    //_fm.set_func_type(p->node_str(), p->eval_type());
    return p;
}

/*std::shared_ptr<expr_sym> parser::p_proto_param()
{
    tok ret = _lex.la(0);
    if(ret.type() != tok::id)
        throw std::runtime_error("Invalid parameter name in prototype");

    return std::make_shared<expr_sym>(ret.str());
}*/

shared_proto parser::p_proto()
{
	if(_lex.la(0).type() != tok::id)
        throw std::runtime_error("Expected identifier in prototype");

    std::string ret = _lex.la(0).str();

	std::vector<std::shared_ptr<expr_sym>> args;
    while(_lex.next_token().type() == tok::id) {
        args.push_back(std::make_shared<expr_sym>(_lex.la(0).str())); // or p_proto_param if type annotated
    }

	if(_lex.la(0).type() != ':') {
        throw std::runtime_error("Expected ':' at end of prototype");
    }

	//if(_lex.next_token().type() != tok::id)
        //throw std::runtime_error("Expected return type at end of prototype");

    //eval_t ret_type = _fm.lookup_type(_lex.la(0).str());
    //if(ret_type == eval_t::ev_invalid)
        //throw std::runtime_error("Invalid return type specified at end of prototype");

    _lex.next_token();

	return std::make_shared<ast_proto>(ret, args);
}

/*bool parser::is_template_proto(const shared_proto& p)
{
    bool params = false;
    bool tparams = false;
    p->for_nodes([&](const std::shared_ptr<ast>& n) {
            if(n->eval_type() == eval_t::ev_template)
                tparams = true;
            else
                params = true;
            });

    if(params && tparams)
        throw std::runtime_error("Mixed template and non-template parameters in prototype");

    return tparams;
}*/

std::shared_ptr<func_template> parser::p_func()
{
	_lex.next_token(); //def

	auto p = p_proto();
	auto e = p_expr();

    // assume all functions are generic
    //auto f = std::make_shared<func_template>(p, e);

    // check if already declared in handler
    /*if(_fm.lookup_template(((*f)[0])->node_str()) != nullptr)
        throw std::runtime_error("Template function already declared");

    _fm.set_template((*f)[0]->node_str(), f);*/
    return std::make_shared<func_template>(p, e);
}

shared_func parser::p_top_lvl()
{
	shared_expr e = p_expr();
    auto p = std::make_shared<proto_anon>(std::string(), std::vector<std::shared_ptr<expr_sym>>());
    //p->set_eval_type(eval_t::ev_void);
    return std::make_shared<func_anon>(p, e);
}
