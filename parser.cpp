//#include <iostream>
//#include <string>
#include <memory>
//#include "utils.h"
#include "parser.h"
#include "lexer.h"

shared_ast parser::p_str()
{
    auto ret = std::make_shared<ast_str>(_lex.la(0).str());
    _lex.next_token();
    return ret;
}

shared_ast parser::p_int()
{
	auto ret = std::make_shared<ast_int>(_lex.la(0).str());
	_lex.next_token();
	return ret;
}

shared_ast parser::p_float()
{
    auto ret = std::make_shared<ast_float>(_lex.la(0).str());
    _lex.next_token();
    return ret;
}

shared_ast parser::p_id()
{
    auto ret = std::make_shared<ast_sym>(_lex.la(0).str());
    _lex.next_token();
    return ret;
}

/*shared_ast parser::p_id_expr(bool single_sym)
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
}*/

shared_ast parser::p_sexp_list()
{
    auto lhs = p_sexp();
    if(_lex.la(0).type() == ')')
        return std::make_shared<ast_cons>(lhs, nullptr);

    return std::make_shared<ast_cons>(lhs, p_sexp_list());
}

shared_ast parser::p_expr_list()
{
    _lex.next_token(); //(
    if(_lex.la(0).type() == ')') {
        _lex.next_token(); //)
        return nullptr;
    }

    shared_cons ret = std::static_pointer_cast<ast_cons>(p_sexp_list());
    _lex.next_token(); //)

    return ret;
}

shared_ast parser::p_atom()
{
    switch(_lex.la(0).type()) {
        case tok::literal_str: return p_str();
        case tok::id: return p_id();
        case tok::literal_int: return p_int();
        case tok::literal_float: return p_float();
        default: throw std::runtime_error("Failed to parse atom");
    }
}

shared_ast parser::p_sexp()
{
    shared_ast ret;
    if(_lex.la(0).type() == '(')
        ret = p_expr_list();
    else
        ret = p_atom();

    return ret;
}

/*shared_ast parser::p_if()
{
    std::string ret = _lex.la(0).str();
    _lex.next_token(); //'if'

    shared_ast cond = p_expr();

    if(_lex.la(0).type() != tok::t_then)
        throw std::runtime_error("Expected 'then' in if");

    _lex.next_token(); //'then'

    shared_ast thenexpr = p_expr();

    if(_lex.la(0).type() != tok::t_else)
        throw std::runtime_error("Expected 'else' in if");

    _lex.next_token(); //'else'

    shared_ast elseexpr = p_expr();

    return std::make_shared<expr_if>(ret, cond, thenexpr, elseexpr);
}*/
