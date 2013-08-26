#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <sstream>
#include "ast.h"
#include "lexer.h"

//extern int cur_tok;
//extern std::int64_t cur_num;
//extern std::string cur_str;
//extern std::ifstream is;

/*namespace
{
template<class T>
struct str_to_num {
	T operator()(const std::string& str) {
		T num;
		std::stringstream ss(str);
		ss >> num;
		return num;
	}
};
}*/

class parser {
public:
	parser(const std::shared_ptr<lexer>& lex)
	{
		_lex = lex;
		_lex->next_token();

        _binop_precedence['='] = 5;
		_binop_precedence['<'] = 10;
		_binop_precedence['>'] = 10;
		_binop_precedence['+'] = 20;
		_binop_precedence['-'] = 20;
		_binop_precedence['*'] = 40;
		_binop_precedence['/'] = 40;
	}

	std::shared_ptr<lexer> lex() {return _lex;}

	shared_expr p_expr_int(void);
	shared_expr p_paren_expr(void);
	shared_expr p_id_expr(void);
	shared_expr p_primary(void);
	shared_expr p_expr(void);
	shared_expr p_expr_binop(int expr_prec, shared_expr lhs);
    shared_expr p_if(void);
    shared_proto p_decl(void);
	shared_proto p_proto(bool is_decl = false);
	shared_func p_func(void);
	shared_func p_top_lvl(void);

	int tok_precedence(void);

private:
	std::map<char, int> _binop_precedence;
	//str_to_num<std::int64_t> _str_to_i64;
	std::shared_ptr<lexer> _lex;
};

#endif
