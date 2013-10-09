#include "utils.h"
#include "ast.h"
#include "eval_t.h"

bool is_binop(int c)
{
    char c2 = static_cast<char>(c);
    if(c2 == '+' || c2 == '-' || c2 == '*' || c2 == '/' || c2 == '<' || c2 == '>')
        return true;

    return false;
}

bool is_binop(const std::string& s)
{
    if(s == "==" || s == "<" || s == "<=" || s == ">" || s == ">=" ||
        s == "+" || s == "-" || s == "*" || s == "/")
        return true;

    return false;
}

bool is_cmp(int c)
{
    char c2 = static_cast<char>(c);
    if(c2 == '>' || c2 == '<' || c2 == '=')
        return true;

    return false;
}

namespace {
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
}

int get_num_args(const std::shared_ptr<ast_func>& f)
{
    return (*f)[0]->num_nodes();
}
