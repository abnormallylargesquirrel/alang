#ifndef LEXER_H
#define LEXER_H

#include <fstream>
#include <memory>
#include "token.h"

class lexer {
public:
	//lexer(std::shared_ptr<std::ifstream>& is) : _is(is) {}
    lexer(std::ifstream& is) : _is(is), _cur_tok(), _last_char(' ') {}
	
	const tok& next_token() {return _cur_tok = get_token();}
	const tok& cur_tok() {return _cur_tok;}

private:
	tok get_token(void);
    void lex_num(tok& ret, bool is_signed = false);
    void lex_binop(tok& ret);
    void lex_id(tok& ret);

	//std::shared_ptr<std::ifstream> _is;
    std::ifstream& _is;
	tok _cur_tok;
    int _last_char;
};

#endif
