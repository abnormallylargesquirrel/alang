#include <iostream>
#include <fstream>
#include <memory>
#include <cstring>
//#include <queue>
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "func_manager.h"
//#include "reducer.h"
//#include "jit_engine.h"
#include "utils.h"

#include "hm_inference.h"
#include "hm_unification.h"
#include "hm_main.h"

struct main_handler {
    main_handler(parser& p, func_manager& fm)
        : _p(p), _fm(fm), _is_eof(false) {}

    /*void handle_func(void)
    {
        auto func = _p.p_func();

        if(_fm.lookup_template(((*func)[0])->node_str()) != nullptr)
            throw std::runtime_error("Template function already declared");

        _fm.set_template((*func)[0]->node_str(), func);
        //std::cout << "Parsed function definition" << std::endl;
        //root.add_node(func);
        //func->gen_func(_je);
    }*/

    /*void handle_toplvl(void)
    {
        shared_func toplvl = _p.p_top_lvl();
        //std::cout << "Parsed top level expression" << std::endl;
        _root.add_node(toplvl);
        //toplvl->gen_func(je);
    }*/

    void operator()(int type)
    {
        switch(type) {
		case tok::eof:
			_is_eof = true;
			break;
		case '(':
            {
            shared_cons sexp = std::static_pointer_cast<ast_cons>(_p.p_sexp());
            if(sexp) {
                if(sexp::check_kwd(*sexp, "define")) {
                    _fm.set_ast_cons(sexp::get_func_name(*sexp), sexp);
                }
            }
            }
			break;
		default:
            {
            std::ostringstream oss("Invalid token: ");
            oss << _p.lex().la(0).type();
            throw std::runtime_error(oss.str());
            }
		}
    }

    parser& _p;
    //jit_engine& _je;
    //ast& _root;
    func_manager& _fm;
	bool _is_eof;
};

/*void run(ast& node, jit_engine& je)
{
    node.for_nodes([&](const std::shared_ptr<ast>& n) {
            llvm::Function *f = n->gen_func(je);
            auto pfunc = je.getPointerToFunction(f);
            auto fp = (void(*)())pfunc;
            fp();
            je.freeMachineCodeForFunction(f);
            });
}*/

int main(int argc, char **argv)
{
    bool dump_module = false;
    if(argc > 1) {
        if(!strcmp(argv[1], "-S"))
            dump_module = true;
    }

    std::ifstream ifs("sfile.txt");
    lexer l(ifs);
    parser p(l);

    func_manager fm;
    /*reducer r(fm);

    llvm::InitializeNativeTarget();
    //jit_engine je(fm); // must follow InitializeNativeTarget()

    ast root;*/
    contexts ctxs;
    environment env;
    std::map<std::string, std::set<std::string>> dependencies;
    try_infer infer(ctxs, env, dependencies, fm);

    main_handler handler(p, fm);
    try
    {
        while(!handler._is_eof) {
            handler(l.la(0).type());
        }

        for(const auto& i : fm)
            std::cout << i.second->node_str() << std::endl;

        //je.run_pm();
        //run(root, je);

        //print_node(fm.lookup_template("func"));

        for(auto& i : fm)
            infer(i.second);
        //infer((*root[0])[1]);

        infer.propagate_contexts();

        //for(auto& i : env)
            //std::cout << i.first << " " << i.second << std::endl;
        for(auto& i : fm)
            std::cout << i.first << " " << env[i.first]<< std::endl;

        //std::cout << root[0]->node_str() << std::endl;
        //r((*root[0])[1]); // pass expression, not function, returned expression replaces function body
        //print_node(root[0]);
    }
    catch(const std::runtime_error& e)
    {
        std::cerr << e.what() << std::endl;
    }

    //if(dump_module)
        //je.dump_module();
	return 0;
}
