#include <iostream>
#include <string>
#include <memory>
#include "utils.h"
#include "parser.h"
#include "lexer.h"

shared_expr parser::p_expr_int()
{
	auto ret = std::make_shared<expr_int>(expr_int(_lex->cur_tok()));
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
		ret.set_type(tok::var);
		return std::make_shared<ast_expr>(expr_var(ret, eval_t::integer));
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
	
	ret.set_type(tok::call);
	_lex->next_token(); //)
	return std::make_shared<ast_expr>(expr_call(ret, args, eval_t::integer));
}

shared_expr parser::p_primary()
{
	switch(_lex->cur_tok().type()) {
	case tok::id:
		return p_id_expr();
	case tok::literal_int:
		return p_expr_int();
    case tok::t_if:
        return p_if();
	case '(':
		return p_paren_expr();
	default:
		return error("unknown token, expecting primary expression");
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
		int tok_prec = tok_precedence();
		if(tok_prec < expr_prec)
			return lhs;

		tok ret = _lex->cur_tok();
		_lex->next_token(); //binop character
		tok debug_ = _lex->cur_tok();

		shared_expr rhs = p_primary();
		if(!rhs)
			return nullptr;

		int next_prec = tok_precedence();
		if(tok_prec < next_prec) {
			rhs = p_expr_binop(tok_prec + 1, rhs);
			if(!rhs)
				return nullptr;
		}
		lhs = std::make_shared<ast_expr>(expr_binop(ret, lhs, rhs));
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

    return std::make_shared<ast_expr>(expr_if(ret, cond, thenexpr, elseexpr));
}

shared_proto parser::p_decl()
{
    _lex->next_token(); //'decl'
    return p_proto(true);
}

shared_proto parser::p_proto(bool is_decl)
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
			const_cast<tok&>(_lex->cur_tok()).set_type(tok::var);
			args.push_back(std::make_shared<expr_var>(expr_var(_lex->cur_tok(), eval_t::integer)));

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

    if(is_decl) {
        ret.set_type(tok::decl);
    } else {
	    ret.set_type(tok::proto);
    }
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

	/*if(_lex->cur_tok().type() != '}')
		return error("expected '}' in function definition");

	_lex->next_token(); //}*/

	return std::make_shared<ast_func>(ast_func(p, e));
}

shared_func parser::p_top_lvl()
{
	if(shared_expr e = p_expr()) {
		shared_proto p = std::make_shared<ast_proto>(ast_proto(tok(tok::proto, ""), std::vector<std::shared_ptr<expr_var>>()));
		return std::make_shared<ast_func>(ast_func(p, e));
	}
	return nullptr;
}

int parser::tok_precedence()
{
	//int tok_type = _lex->cur_tok().type();
    if(_lex->cur_tok().type() != tok::binop)
        return -1;

	//if(!is_ascii(tok_type))
		//return -1;

	int ret = _binop_precedence[_lex->cur_tok().str()[0]];
	if(ret <= 0)
		return -1;

	return ret;
}
