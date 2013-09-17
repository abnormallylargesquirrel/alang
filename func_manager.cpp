#include <algorithm>
#include "func_manager.h"
#include "ast.h"

std::string func_manager::mangle_name(const ast& node)
{
    if(node.node_str().size() == 0)
        return std::string();

    std::stringstream ss;
    ss << node.node_str() << "#";
    node.for_nodes([&](const std::shared_ptr<ast>& n) {
            ss << eval_strings[static_cast<int>(n->eval_type())] << "#";
            });

    return ss.str();
}
