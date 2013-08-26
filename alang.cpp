#include <iostream>
#include <fstream>
#include <memory>
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "jit_engine.h"
#include "utils.h"
//#include "symbol.h"

/*
LL(k)

expr_int ::= "number"
paren_expr ::= '(' expr ')'
id_expr ::= "id"
expr_var ::= id_expr

primary ::= expr_int | paren_expr | id_expr
binop_rhs ::= ('+' | '-' | '*' | '/' | '<' | '>' primary)*
expr ::= primary binop_rhs | ""

decl ::= 'decl' proto
proto ::= id_expr '(' id_expr* ')'
func ::= 'def' proto '{' expr '}'

call ::= id_expr '(' expr* ')'

toplvl ::= expr

top ::= func | toplvl
*/

void handle_decl(parser& p, ast& root)
{
    if(shared_proto f = p.p_decl()) {
        //std::cout << "Parsed function declaration\n";
        root.add_node(f);
    } else {
        p.lex()->next_token();
    }
}

void handle_func(parser& p, ast& root)
{
	if(shared_func f = p.p_func()) {
		//std::cout << "Parsed function definition\n";
		root.add_node(f);
        /*if(Function *lf = je.gen_func(f)) {
            lf->dump();
        }*/
	} else {
		p.lex()->next_token();
	}
}

void handle_toplvl(parser& p, ast& root)
{
	if(shared_func t = p.p_top_lvl()) {
		//std::cout << "Parsed top level expression\n";
		root.add_node(t);
        /*if(Function *lf = je.gen_func(t)) {
            lf->dump();
        }*/
	} else {
		p.lex()->next_token();
	}
}


/*std::string tree_to_str(const std::shared_ptr<ast>& node)
{
	std::stringstream ss;
	node->for_nodes([&](const std::shared_ptr<ast>& n) {
            ss << "\n[" << n->to_str();
            ss << ' ' << tree_to_str(n);
            });
	ss << ']';

	return ss.str();
}*/

void run_jit(const std::shared_ptr<ast>& node, jit_engine& je)
{
    node->for_nodes([&](const std::shared_ptr<ast>& n){
            if(n->node_type() == tok::def) {
                if(!(*n)[0]->node_str().empty()) {
                    if(!je.gen_func(n)) {
                        std::cout << "Error: gen_func in tok::def" << std::endl;
                        return;
                    }
                } else {
                    if(Function *func = je.gen_func(n)) {
                        void *pfunc = je.getPointerToFunction(func);
                        std::int64_t (*fp)() = (std::int64_t (*)())pfunc;
                        fp();
                        je.freeMachineCodeForFunction(func);
                    }
                }
            } else if(n->node_type() == tok::decl) {
                if(!je.gen_func(n)) {
                    std::cout << "Error: gen_func in tok::decl" << std::endl;
                    return;
                }
            }
            run_jit(n, je);
            });
}

int main()
{
    std::ifstream ifs("file.txt");
    parser p(std::make_shared<lexer>(lexer(ifs)));
	ast root;

    InitializeNativeTarget();
    jit_engine je;

	bool is_eof = false;
	while(!is_eof) {
		switch(p.lex()->cur_tok().type()) {
		case tok::eof:
			is_eof = true;
			break;
		case tok::def:
			handle_func(p, root);
			break;
        case tok::decl:
            handle_decl(p, root);
            break;
		case tok::var:
		case tok::call:
		case tok::proto:
		case tok::literal_int:
		default:
			handle_toplvl(p, root);
		}
	}

    je.run_pm();
    run_jit(std::make_shared<ast>(root), je);
    je.dump_module();
	return 0;
}
