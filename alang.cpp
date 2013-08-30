#include <iostream>
#include <fstream>
#include <memory>
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "jit_engine.h"
#include "utils.h"

void handle_decl(parser& p, jit_engine *je)
{
    if(shared_proto proto = p.p_decl()) {
        //std::cout << "Parsed function declaration\n";
        //root.add_node(f);
        proto->gen_func(je);
    } else {
        p.lex()->next_token();
    }
}

void handle_func(parser& p, jit_engine *je)
{
	if(shared_func func = p.p_func()) {
		//std::cout << "Parsed function definition\n";
		//root.add_node(f);
        func->gen_func(je);
	} else {
		p.lex()->next_token();
	}
}

void handle_toplvl(parser& p, jit_engine *je)
{
    //std::cout << "toplvl" << std::endl;
	if(shared_func toplvl = p.p_top_lvl()) {
		//std::cout << "Parsed top level expression\n";
		//root.add_node(t);
        toplvl->gen_func(je);
	} else {
		p.lex()->next_token();
	}
}

/*void run(const std::shared_ptr<ast>& node, jit_engine* je)
{
    node->for_nodes([&](const std::shared_ptr<ast>& n) {
            std::cout << n->to_str() << std::endl;
            run_jit(n, je);
            if(!n->gen_func(je))
                return;
            });
}*/

int main()
{
    std::ifstream ifs("file.txt");
    parser p(std::make_shared<lexer>(lexer(ifs)));
	//ast root;

    InitializeNativeTarget();
    jit_engine je;

	bool is_eof = false;
	while(!is_eof) {
		switch(p.lex()->cur_tok().type()) {
		case tok::eof:
			is_eof = true;
			break;
		case tok::def:
			handle_func(p, &je);
			break;
        case tok::decl:
            handle_decl(p, &je);
            break;
		default:
            if(error_called) {
                return 1;
            }
			handle_toplvl(p, &je);
		}
	}

    //je.run_pm();
    //run(std::make_shared<ast>(root), &je);
    je.dump_module();
	return 0;
}
