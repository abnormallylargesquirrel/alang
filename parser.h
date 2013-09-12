#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <sstream>
#include "ast.h"
#include "lexer.h"
#include "func_manager.h"

class parser {
public:
	parser(const std::shared_ptr<lexer>& lex, const std::shared_ptr<func_manager>& fm)
        : _lex(lex), _fm(fm) {_lex->next_token();}

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

private:
	std::shared_ptr<lexer> _lex;
	std::shared_ptr<func_manager> _fm;
    std::shared_ptr<expr_var> p_proto_param(void);
};

#endif
