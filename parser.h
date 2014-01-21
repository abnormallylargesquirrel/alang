#ifndef PARSER_H
#define PARSER_H

//#include <map>
//#include <sstream>
#include "ast.h"
#include "lexer.h"
//#include "func_manager.h"

class parser {
 public:
 parser(lexer& lex)
     : _lex(lex) {_lex.next_token();}

    lexer& lex() {return _lex;}
    shared_ast p_sexp(void);

    /*shared_expr p_expr_int(void);
      shared_expr p_expr_float(void);
      shared_expr p_paren_expr(void);
      shared_expr p_id_expr(bool single_sym);
      shared_expr p_primary(bool single_sym = false);
      shared_expr p_expr(void);
      shared_expr p_expr_binop(int expr_prec, shared_expr lhs);
      shared_expr p_if(void);
      shared_proto p_decl(void);
      shared_proto p_proto(void);
      shared_template p_func(void);
      shared_func p_top_lvl(void);*/

 private:
    shared_ast p_str(void);
    shared_ast p_int(void);
    shared_ast p_float(void);
    shared_ast p_id(void);
    shared_ast p_sexp_list(void);
    shared_ast p_expr_list(void);
    shared_ast p_atom(void);
    lexer& _lex;
    //std::shared_ptr<func_manager> _fm;
    //func_manager& _fm;
    //std::shared_ptr<expr_sym> p_proto_param(void);

    //bool is_template_proto(const shared_proto& p);
};

#endif
