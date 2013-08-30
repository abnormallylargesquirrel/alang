#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <sstream>
#include "ast.h"
#include "lexer.h"

class parser {
public:
	parser(const std::shared_ptr<lexer>& lex)
	{
		_lex = lex;
		_lex->next_token();

        _lookup_type["i64"] = eval_t::ev_int64;
        _lookup_type["float"] = eval_t::ev_float;
        _lookup_type["void"] = eval_t::ev_void;

        /*_binop_precedence['='] = 5;
		_binop_precedence['<'] = 10;
		_binop_precedence['>'] = 10;
		_binop_precedence['+'] = 20;
		_binop_precedence['-'] = 20;
		_binop_precedence['*'] = 40;
		_binop_precedence['/'] = 40;*/
	}

	std::shared_ptr<lexer> lex() {return _lex;}

	shared_expr p_expr_int(void);
	shared_expr p_expr_float(void);
	shared_expr p_paren_expr(void);
	shared_expr p_id_expr(void);
	shared_expr p_primary(void);
	shared_expr p_expr(void);
	shared_expr p_expr_binop(int expr_prec, shared_expr lhs);
    shared_expr p_if(void);
    shared_proto p_decl(void);
	shared_proto p_proto(void);
	shared_func p_func(void);
	shared_func p_top_lvl(void);

	//int tok_precedence(void);

private:
	//std::map<char, int> _binop_precedence;
    std::map<std::string, eval_t> _lookup_type;
    std::map<std::string, eval_t> _lookup_func_type;
	std::shared_ptr<lexer> _lex;
    eval_t lookup_type(const std::string& str);
};

#endif
