#include <cctype>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include "lexer.h"
#include "utils.h"

void lexer::lex_str(tok& ret)
{
    std::ostringstream oss;
    _last_char = _is.get();
    while(_last_char != '"') {
        oss << static_cast<char>(_last_char);
        _last_char = _is.get();
    }

    _last_char = _is.get();
    ret.set_str(oss.str());
    ret.set_type(tok::literal_str);
}

void lexer::lex_id(tok& ret)
{
    std::ostringstream oss;
    oss << static_cast<char>(_last_char);
    _last_char = _is.get();
    while(std::isalnum(_last_char) || is_binop(_last_char) || _last_char == '_') {
        oss << static_cast<char>(_last_char);
        _last_char = _is.get();
    }
    ret.set_str(oss.str());

    /*if(ret.str() == "def") {
        ret.set_type(tok::def);
    } else if(ret.str() == "decl") {
        ret.set_type(tok::decl);
    } else if(ret.str() == "if") {
        ret.set_type(tok::t_if);
    } else if(ret.str() == "then") {
        ret.set_type(tok::t_then);
    } else if(ret.str() == "else") {
        ret.set_type(tok::t_else);
    } else {*/
        ret.set_type(tok::id);
    //}
}

void lexer::lex_num(tok& ret, bool is_signed)
{
    int num_type;
    std::ostringstream oss;
    bool point_found = false;

    if(is_signed)
        oss << '-';

    while(std::isdigit(_last_char) || _last_char == '.') {
        if(_last_char == '.') {
            if(point_found) {
                throw std::runtime_error("Found  more than 1 '.' in num literal");
            }
            point_found = true;
        }
        oss << static_cast<char>(_last_char);
        _last_char = _is.get();
    }

    ret.set_str(oss.str());

    if(point_found)
        num_type = tok::literal_float;
    else
        num_type = tok::literal_int;

    ret.set_type(num_type);
}

tok lexer::get_token()
{
	tok ret;
	while(std::isspace(_last_char))
		_last_char = _is.get();

    if(_last_char == '"') {
        lex_str(ret);
        return ret;
    }

	if(std::isalpha(_last_char) || is_binop(_last_char) || _last_char == '_') {
        lex_id(ret);
        return ret;
    }

    /*if(_last_char == '-') {
        if(_cur_tok.type() == '(') { //unary- must follow lparen (-n)
            //ret.set_literal_signed(true);
            _last_char = _is.get();
            lex_num(ret, true);
            //std::cout << "lexed negative literal" << std::endl;
            return ret;
        }
    }*/

    if(std::isdigit(_last_char)) {
        lex_num(ret);
        //std::cout << "lexed literal" << std::endl;
        return ret;
    }

    /*if(is_binop(_last_char) || _last_char == '=') { //must come after unary- check
        lex_binop(ret);
        return ret;
    }*/

	//if(_last_char == ';') {
    if(_last_char == *COMMENT_STR()) {
		while(_last_char != EOF && _last_char != '\n' && _last_char != '\r')
			_last_char = _is.get();

		if(_last_char != EOF)
			return get_token();
	}

	if(_last_char == EOF) {
		ret.set_type(tok::eof);
		return ret;
	}

	ret.set_type(_last_char);
    _last_char = _is.get();
	return ret;
}
