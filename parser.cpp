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
        //eval_t type = _fm->lookup_var(ret.str());

        //if(type == eval_t::ev_invalid)
            //return error("invalid type variable referenced");

        return std::make_shared<expr_var>(expr_var(ret, eval_t::ev_template/*type*/));
	}

	//call
	_lex->next_token(); //(
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
	
	_lex->next_token(); //)

    eval_t tmp_type = _fm->lookup_func_type(ret.str());
    //if(tmp_type != eval_t::ev_invalid)
        return std::make_shared<expr_call>(expr_call(ret, args, tmp_type));

    //return error("call has no prototype");
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

    auto p = p_proto();
    _fm->set_func_type(p->node_str(), p->eval_type());
    return p;
}

std::shared_ptr<expr_var> parser::p_proto_param()
{
    eval_t type = _fm->lookup_type(_lex->cur_tok().str());
    if(type == eval_t::ev_invalid) {
        return std::make_shared<expr_var>(expr_var(_lex->cur_tok(), eval_t::ev_template));
        //return error("invalid type specified in proto parameter");
    }

    if(_lex->next_token().type() != tok::id)
        return error("unnamed parameter in proto");

    //std::cout << eval_strings[static_cast<int>(type)] << std::endl;
    return std::make_shared<expr_var>(expr_var(_lex->cur_tok(), type));
}

shared_proto parser::p_proto()
{
	if(_lex->cur_tok().type() != tok::id)
		return error("expected id in prototype");

	tok ret = _lex->cur_tok();

	if(_lex->next_token().type() != '(')
		return error("expected '(' in prototype");

	_lex->next_token(); //(

	std::vector<std::shared_ptr<expr_var>> args;
	if(_lex->cur_tok().type() == tok::id) {
		while(1) {
            auto arg = p_proto_param();
            //_fm->set_var(arg->node_str(), arg->eval_type());
            //std::cout << eval_strings[static_cast<int>(arg->eval_type())] << ":" << std::endl;

			args.push_back(arg);

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

    /*bool params = false;
    bool tparams = false;
    for(const auto& i : args) {
        if(i->eval_type() == eval_t::ev_template)
            tparams = true;
        else
            params = true;
    }

    if(params && tparams)
        return error("mixed template and non-template parameters in proto");*/

	if(_lex->next_token().type() != ':')
        return error("expected ':' & return type at end of proto");

	if(_lex->next_token().type() != tok::id)
        return error("expected return type at end of proto");

    eval_t ret_type = _fm->lookup_type(_lex->cur_tok().str());
    if(ret_type == eval_t::ev_invalid)
        return error("invalid return type specified at end of proto");

    _lex->next_token();

    auto p = std::make_shared<ast_proto>(ast_proto(ret, args));
    /*if(tparams) {
        //return std::make_shared<proto_template>(proto_template(ret, args));
        p->set_eval_type(eval_t::ev_template);
    } else {
        p->set_eval_type(eval_t::ev_invalid);
    }*/
    p->set_eval_type(ret_type);

	return p;
}

bool parser::is_template_proto(const shared_proto& p)
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
        error("mixed template and non-template parameters in proto");

    return tparams;
}

shared_func parser::p_func()
{
	_lex->next_token(); //def
    //_fm->clear_vars();

	auto p = p_proto();
	if(!p)
		return error("failed to parse prototype after 'def'");

	/*if(_lex->cur_tok().type() != '{')
		return error("expected '{' in function definition");

	_lex->next_token(); //{*/

	auto e = p_expr();
	if(!e)
		return error("failed to parse expression in function definition");

    //_fm->clear_vars();

    _fm->set_func_type(p->node_str(), p->eval_type());

    if(is_template_proto(p)) {
        auto f = std::make_shared<func_template>(func_template(p, e));

        if(_fm->lookup_template(((*f)[0])->node_str()) != nullptr)
            return error("Template function already declared");

        _fm->set_template((*f)[0]->node_str(), f);

        return f;
    }

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
