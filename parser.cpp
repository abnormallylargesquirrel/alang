#include <iostream>
#include <string>
#include <memory>
#include "utils.h"
#include "parser.h"
#include "lexer.h"

eval_t parser::lookup_type(const std::string& str)
{
    if(_lookup_type.find(str) == _lookup_type.end())
        return eval_t::ev_invalid;

    return _lookup_type[str];
}

shared_expr parser::p_expr_int()
{
	auto ret = std::make_shared<expr_int>(expr_int(_lex->cur_tok()));
	_lex->next_token();
	return ret;
}

shared_expr parser::p_expr_float()
{
    auto ret = std::make_shared<expr_float>(expr_float(_lex->cur_tok()));
    _lex->next_token();
    return ret;
}

shared_expr parser::p_paren_expr()
{
	_lex->next_token(); //(
	auto a = p_expr();
	if(!a)
		return nullptr;

	if(_lex->cur_tok().type() != ')')
		return error("expected ')'");

	_lex->next_token(); //)
	return a;
}

shared_expr parser::p_id_expr()
{
	tok ret = _lex->cur_tok();
	_lex->next_token(); //id

	if(_lex->cur_tok().type() != '(') { //var
		//ret.set_type(tok::var);
		return std::make_shared<expr_var>(expr_var(ret, eval_t::ev_int64));
	}

	//call
	_lex->next_token(); //(
    /*std::cout << _lex->cur_tok().type() << std::endl
        << _lex->cur_tok().literal_signed() << std::endl
        << _lex->cur_tok().to_str() << std::endl;*/
	std::vector<shared_expr> args;
	if(_lex->cur_tok().type() != ')') {
		while(1) {
			auto arg = p_expr();
			if(!arg)
				return nullptr;
			args.push_back(arg);

			if(_lex->cur_tok().type() == ')')
				break;

			if(_lex->cur_tok().type() != ',')
				return error("expected ')' or ',' in argument list");

			_lex->next_token();
		}
	}
	
	//ret.set_type(tok::call);
	_lex->next_token(); //)
    //std::cout << ret.str() << std::endl;
    if(_lookup_func_type.find(ret.str()) != _lookup_func_type.end())
        return std::make_shared<expr_call>(expr_call(ret, args, _lookup_func_type[ret.str()]));

    return error("call has no prototype");
}

shared_expr parser::p_primary()
{
	switch(_lex->cur_tok().type()) {
	case tok::id: return p_id_expr();
	case tok::literal_int: return p_expr_int();
    case tok::literal_float: return p_expr_float();
    case tok::t_if: return p_if();
	case '(': return p_paren_expr();
	default: return error("unknown token, expecting primary expression");
	}
}

shared_expr parser::p_expr()
{
	shared_expr lhs = p_primary();
	if(!lhs)
		return nullptr;

	return p_expr_binop(0, lhs);
}

shared_expr parser::p_expr_binop(int expr_prec, shared_expr lhs)
{
	while(1) {
        tok ret = _lex->cur_tok();
		int tok_prec = ret.prec();
        /*std::cout << "expr_prec:" << expr_prec << std::endl;
        std::cout << "type:" << ret.type() << std::endl;
        std::cout << "prec:" << ret.prec() << std::endl;*/
		if(tok_prec < expr_prec)
			return lhs;

		_lex->next_token(); //binop character

		shared_expr rhs = p_primary();
		if(!rhs)
			return error("could not parse rhs of binop expression");

		int next_prec = _lex->cur_tok().prec();
		if(tok_prec < next_prec) {
			rhs = p_expr_binop(tok_prec + 1, rhs);
			if(!rhs)
				return error("could not parse rhs of binop expression");
		}

        switch(ret.type()) {
            case tok::add:
                lhs = std::make_shared<binop_add>(binop_add(ret, lhs, rhs));
                break;
            case tok::sub:
                lhs = std::make_shared<binop_sub>(binop_sub(ret, lhs, rhs));
                break;
            case tok::mul:
                lhs = std::make_shared<binop_mul>(binop_mul(ret, lhs, rhs));
                break;
            case tok::div:
                lhs = std::make_shared<binop_div>(binop_div(ret, lhs, rhs));
                break;
            case tok::lt:
                lhs = std::make_shared<binop_lt>(binop_lt(ret, lhs, rhs));
                break;
            case tok::gt:
                lhs = std::make_shared<binop_gt>(binop_gt(ret, lhs, rhs));
                break;
            case tok::lte:
                lhs = std::make_shared<binop_lte>(binop_lte(ret, lhs, rhs));
                break;
            case tok::gte:
                lhs = std::make_shared<binop_gte>(binop_gte(ret, lhs, rhs));
                break;
            case tok::eq:
                lhs = std::make_shared<binop_eq>(binop_eq(ret, lhs, rhs));
                break;
            default:
                return nullptr;
        }
		//lhs = std::make_shared<ast_expr>(expr_binop(ret, lhs, rhs));
	}
}

shared_expr parser::p_if()
{
    tok ret = _lex->cur_tok();
    _lex->next_token(); //'if'

    shared_expr cond = p_expr();
    if(!cond)
        return nullptr;

    if(_lex->cur_tok().type() != tok::t_then)
        return error("expected 'then' in if statement");
    _lex->next_token(); //'then'

    shared_expr thenexpr = p_expr();
    if(!thenexpr)
        return nullptr;

    if(_lex->cur_tok().type() != tok::t_else)
        return error("expected 'else' in if statement");
    _lex->next_token(); //'else'

    shared_expr elseexpr = p_expr();
    if(!elseexpr)
        return nullptr;

    return std::make_shared<expr_if>(expr_if(ret, cond, thenexpr, elseexpr));
}

shared_proto parser::p_decl()
{
    _lex->next_token(); //'decl'
    if(_lex->cur_tok().type() != tok::id)
        return error("expected return type in forward function declaration");

    eval_t type = lookup_type(_lex->cur_tok().str());
    if(type == eval_t::ev_invalid)
        return error("invalid type specified in forward function declaration");

    _lex->next_token();
    auto p = p_proto();

    p->set_eval_type(type);
    _lookup_func_type[p->node_str()] = type;

    return p;
}

shared_proto parser::p_proto()
{
	if(_lex->cur_tok().type() != tok::id)
		return error("expected id in prototype");

	tok ret = _lex->cur_tok();

	if(_lex->next_token().type() != '(')
		return error("expected '(' in prototype");

	_lex->next_token();

	std::vector<std::shared_ptr<expr_var>> args;
	if(_lex->cur_tok().type() == tok::id) {
		while(1) {
			//const_cast<tok&>(_lex->cur_tok()).set_type(tok::var);
			args.push_back(std::make_shared<expr_var>(expr_var(_lex->cur_tok(), eval_t::ev_int64)));

			_lex->next_token();
			if(_lex->cur_tok().type() == ')') {
				break;
			}
			if(_lex->cur_tok().type() != ',') {
				return error("expected ',' or ')' in prototype");
			}

			_lex->next_token();

			if(_lex->cur_tok().type() != tok::id)
				return error("expected id in prototype");
		}
	}

	_lex->next_token();

    /*if(is_decl) {
        return std::make_shared<ast_proto>(proto_decl(ret, args));
    } else {
        return std::make_shared<ast_proto>(ast_proto(ret, args));
    }*/
	return std::make_shared<ast_proto>(ast_proto(ret, args));
}

shared_func parser::p_func()
{
	_lex->next_token(); //def
	shared_proto p = p_proto();
	if(!p)
		return nullptr;

	/*if(_lex->cur_tok().type() != '{')
		return error("expected '{' in function definition");

	_lex->next_token(); //{*/

	auto e = p_expr();
	if(!e)
		return nullptr;

    p->set_eval_type(e->eval_type());
    _lookup_func_type[p->node_str()] = p->eval_type();
    //std::cout << static_cast<int>(_lookup_func_type[p->node_str()]) << '\n';
    //std::cout << static_cast<int>(p->eval_type()) << std::endl;

	/*if(_lex->cur_tok().type() != '}')
		return error("expected '}' in function definition");

	_lex->next_token(); //}*/

	return std::make_shared<ast_func>(ast_func(p, e));
}

shared_func parser::p_top_lvl()
{
	if(shared_expr e = p_expr()) {
        auto p = std::make_shared<proto_anon>(proto_anon(tok(tok::anon_proto, ""), std::vector<std::shared_ptr<expr_var>>()));
        p->set_eval_type(eval_t::ev_void);
		return std::make_shared<func_anon>(func_anon(p, e));
	}
	return nullptr;
}
