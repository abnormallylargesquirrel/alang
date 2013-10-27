#include "utils.h"
#include "ast.h"
#include "eval_t.h"

bool is_binop(int c)
{
    char c2 = static_cast<char>(c);
    if(c2 == '+' || c2 == '-' || c2 == '*' || c2 == '/' || c2 == '<' || c2 == '>' || c2 == '=')
        return true;

    return false;
}

namespace sexp
{
void map_effect(std::function<void(const shared_ast&)> f, shared_ast list)
{
    while(list) {
        if(list->car())
            f(list->car());
        else
            f(list);
        list = list->cdr();
    }
}

std::size_t get_length(const shared_ast& a, std::size_t accum)
{
    if(a->tag() != tags::tcons)
        return accum;
    if(!a->cdr())
        return accum;

    return get_length(a->cdr(), accum + 1);
}

// ensure a is func
shared_ast get_body(const ast& a)
{
    return a.cdr()->cdr();
}

// ensure a is func
shared_ast get_proto(const ast& a)
{
    return a.cdr()->car();
}

// ensure a is func
std::string get_func_name(const ast& a)
{
    auto ret = get_proto(a);
    if(ret->car())
        return ret->car()->node_str(); // > 0 parameters

    return ret->node_str(); // no parameters
}

std::string get_first_str(const ast& a)
{
    if(a.tag() != tags::tcons)
        return std::string();

    return a.car()->node_str();
}

bool check_kwd(const ast& a, const std::string& s)
{
    if(a.tag() != tags::tcons)
        return false;

    return get_first_str(a) == s;
}
}

/*namespace {
void print_nodes(std::queue<std::shared_ptr<ast>>& nodes)
{
    if(nodes.empty())
        return;

    std::queue<std::shared_ptr<ast>> next;
    while(!nodes.empty()) {
        auto front = nodes.front();
        if(front->num_nodes() > 0) {
        front->for_nodes([&](const std::shared_ptr<ast>& n) {
                next.push(n);
                std::cout << '<' << n->node_str() << ',' << n.get() << '>' << ' ';
                //std::cout << '<' << n->node_str() << '>' << ' ';
                });
        } else {
            std::cout << "[]";
        }
        nodes.pop();

        if(!nodes.empty())
            std::cout << " :: ";
    }
    std::cout << std::endl;

    print_nodes(next);
}
}

void print_node(const std::shared_ptr<ast>& node)
{
    std::queue<std::shared_ptr<ast>> q;
    q.push(node);
    print_nodes(q);
}*/

/*int get_num_args(const std::shared_ptr<ast_func>& f)
{
    return (*f)[0]->num_nodes();
}*/
