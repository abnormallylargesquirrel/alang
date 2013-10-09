#ifndef LEXER_H
#define LEXER_H

#include <fstream>
#include <queue>
#include <memory>
#include "token.h"

class lexer {
public:
    lexer(std::ifstream& is) : _is(is), _tok_cache(), _cur_tok(), _last_char(' ') {}
	
	const tok& next_token(bool read_cache = true)
    {
        if(read_cache) {
            if(!_tok_cache.empty()) {
                _cur_tok = std::move(_tok_cache.front());
                _tok_cache.pop_front();
                return _cur_tok;
            }
        }
        return _cur_tok = get_token();
    }
    const tok& la(std::size_t n)
    {
        if(n == 0)
            return _cur_tok;

        if(n > _tok_cache.size()) {
            std::size_t to_cache = n - _tok_cache.size();
            for(std::size_t i = 0; i < to_cache; i++)
                cache_token();

            return _tok_cache.back();
        }

        return _tok_cache[n - 1];
    }
	//const tok& cur_tok(void) const {return _cur_tok;}

private:
	tok get_token(void);
    void cache_token(void)
    {
        //tok tmp = next_token(false);
        _tok_cache.push_back(next_token(false));
    }
    void lex_num(tok& ret, bool is_signed = false);
    void lex_binop(tok& ret);
    void lex_id(tok& ret);

    std::ifstream& _is;
    std::deque<tok> _tok_cache;
	tok _cur_tok;
    int _last_char;
};

#endif
