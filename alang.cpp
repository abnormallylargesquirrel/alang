#include <iostream>
#include <fstream>
#include <memory>
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "jit_engine.h"
#include "func_manager.h"
#include "utils.h"

struct main_handler {
    main_handler(parser& p, jit_engine *je, ast& root)
        : _p(p), _je(je), _root(root), _is_eof(false) {}

    void handle_decl(void)
    {
        if(shared_proto proto = _p.p_decl()) {
            //std::cout << "Parsed function declaration" << std::endl;
            //root.add_node(proto);
            proto->gen_func(_je);
        } else {
            _p.lex()->next_token();
        }
    }

    void handle_func(void)
    {
        if(shared_func func = _p.p_func()) {
            //std::cout << "Parsed function definition" << std::endl;
            //root.add_node(func);
            func->gen_func(_je);
        } else {
            _p.lex()->next_token();
        }
    }

    void handle_toplvl(void)
    {
        if(shared_func toplvl = _p.p_top_lvl()) {
            //std::cout << "Parsed top level expression" << std::endl;
            _root.add_node(toplvl);
            //toplvl->gen_func(je);
        } else {
            _p.lex()->next_token();
        }
    }

    void operator()(int type)
    {
        switch(type) {
		case tok::eof:
			_is_eof = true;
			break;
		case tok::def:
			handle_func();
			break;
        case tok::decl:
            handle_decl();
            break;
		default:
			handle_toplvl();
            break;
		}
    }

    parser& _p;
    jit_engine *_je;
    ast& _root;
	bool _is_eof;
};

void run(ast& node, jit_engine* je)
{
    node.for_nodes([&](const std::shared_ptr<ast>& n) {
            //std::cout << n->to_str() << std::endl;
            llvm::Function *f = n->gen_func(je);
            if(!f) {
                std::cout << "Failed to generate anonymous function" << std::endl;
                return;
            }
            void *pfunc = je->getPointerToFunction(f);
            void(*fp)() = (void(*)())pfunc;
            fp();
            je->freeMachineCodeForFunction(f);
            });
}

int main(int argc, char **argv)
{
    bool dump_module = false;
    if(argc > 1) {
        if(!strcmp(argv[1], "-S"))
            dump_module = true;
    }
    //func_manager fm;
    auto fm = std::make_shared<func_manager>(func_manager());
    std::ifstream ifs("file.txt");
    parser p(std::make_shared<lexer>(lexer(ifs)), fm);

    llvm::InitializeNativeTarget();
    jit_engine je(fm); //must follow InitializeNativeTarget()

    ast root;
    main_handler handler(p, &je, root);

	while(!handler._is_eof && !error_called) {
        handler(p.lex()->cur_tok().type());
	}

    //je.run_pm();
    run(root, &je);
    if(dump_module)
        je.dump_module();
	return 0;
}
