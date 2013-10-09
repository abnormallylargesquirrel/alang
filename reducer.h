#ifndef REDUCER_H
#define REDUCER_H

#include <memory>
#include <stack>
//#include "ast.h"
//#include "func_manager.h"

class ast;
class ast_func;
class expr_apply;
class func_manager;

class reducer {
public:
    reducer(func_manager& fm)
        : _fm(fm) {}

    void operator()(std::shared_ptr<ast>& root);
    std::shared_ptr<ast> copy_tree(const std::shared_ptr<ast>& root);

private:
    void unwind_spine(std::shared_ptr<ast>& root);
    bool find_redex(const std::string& func_name,
            std::vector<std::shared_ptr<ast>>& args);
    std::shared_ptr<ast> instantiate(const std::string& func_name,
            std::vector<std::shared_ptr<ast>>& args);
    void substitute(std::shared_ptr<ast>& expr,
            const std::vector<std::string>& names,
            const std::vector<std::shared_ptr<ast>>& args);
    //std::vector<std::shared_ptr<ast>> get_args(std::shared_ptr<ast> app);

    func_manager& _fm;
    std::stack<std::shared_ptr<ast>> _spine;
    int _stack_depth;
};

#endif
