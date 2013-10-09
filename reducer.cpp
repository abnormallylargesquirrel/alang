#include <vector>
#include "reducer.h"
#include "ast.h"
#include "func_manager.h"
#include "utils.h"

void reducer::operator()(std::shared_ptr<ast>& root)
{
    unwind_spine(root);
    auto func_name = _spine.top()->node_str();
    std::vector<std::shared_ptr<ast>> args;
    if(find_redex(func_name, args)) { // reduce
        // std::cout << app.get() << std::endl
            // << args.size() << std::endl;
        root = instantiate(func_name, args);
    }
    // else already in WHNF
}

void reducer::unwind_spine(std::shared_ptr<ast>& root)
{
    if(root->num_nodes() != 2)
        throw std::runtime_error("Unwind_spine passed node with no child nodes");

    std::shared_ptr<ast> left = root;;
    _stack_depth = 0;
    _spine.push(root);

    do {
        left = (*left)[0];
        _spine.push(left);
        _stack_depth++;
    } while(left->num_nodes() == 2);

    // left == _spine.top()
}

bool reducer::find_redex(const std::string& func_name,
        std::vector<std::shared_ptr<ast>>& args) // expr_sym on top of _spine (after unwind_spine)
{
    if(auto func = _fm.lookup_template(func_name)) {
        int num_args = get_num_args(func);
        if(_stack_depth < num_args) {
            return false; // already in WHNF
        }

        for(int i = 0; i < num_args; i++) {
            _spine.pop();
            auto arg = (*(_spine.top()))[1];
            args.push_back(arg);
            _stack_depth--;
        }

        //std::cout << _spine.top().get() << std::endl;
        //return _spine.top();
        return true;
    } else {
        // TODO lookup built-in/strict function, go up stack number of arguments
        std::cout << func_name << ':';
        throw std::runtime_error("Built-in functions not yet implemented");
    }
}

std::shared_ptr<ast> reducer::copy_tree(const std::shared_ptr<ast>& root)
{
    std::shared_ptr<ast> node;
    root->clone(node);

    node->for_nodes([&](std::shared_ptr<ast>& n) {n = copy_tree(n);});

    return node;
}

/*std::vector<std::shared_ptr<ast>> reducer::get_args(std::shared_ptr<ast> app)
{
    std::vector<std::shared_ptr<ast>> ret;

    do {
        ret.push_back((*app)[1]);
        app = (*app)[0];
    } while(app->num_nodes() == 2);

    return ret;
}*/

void reducer::substitute(std::shared_ptr<ast>& expr,
        const std::vector<std::string>& names,
        const std::vector<std::shared_ptr<ast>>& args)
{
    expr->for_nodes([&](std::shared_ptr<ast>& n) {
            for(unsigned int i = 0; i < names.size(); i++) {
                if(n->node_str() == names[i]) {
                    n = args[i];
                }
            }
            substitute(n, names, args);
            });
}

// parameter names (from func_name) and args make up "environment"
std::shared_ptr<ast> reducer::instantiate(const std::string& func_name, // name for lookup_template
        std::vector<std::shared_ptr<ast>>& args)
{
    std::vector<std::string> names;
    auto func_template = _fm.lookup_template(func_name);
    (*func_template)[0]->for_nodes([&](const std::shared_ptr<ast>& n) {
            names.push_back(n->node_str());
            }); // names corresponds to args, names[0] is name for args[0]

    if(!is_unique_vector(names))
        throw std::runtime_error("Renamed parameter in prototype");

    auto instance = copy_tree((*func_template)[1]); // only copy function body
    substitute(instance, names, args);

    return instance;
}
