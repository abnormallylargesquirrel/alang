#include <algorithm>
#include <unordered_set>
#include "func_manager.h"
#include "ast.h"

eval_t func_manager::resolve_types(const std::shared_ptr<ast>& node)
{
    if(node->num_nodes() == 0) {
        if(lookup_var(node->node_str()) != eval_t::ev_invalid) {
            return _lookup_var[node->node_str()];
        }
        if(node->eval_type() == eval_t::ev_template) {
            return eval_t::ev_invalid;
        }

        return node->eval_type();
    }

    //std::vector<eval_t> nodes_types;
    std::unordered_set<int> nodes_types;
    node->for_nodes([&](const std::shared_ptr<ast>& n) {
            eval_t t = resolve_types(n);
            n->set_eval_type(t);
            if(t != eval_t::ev_invalid && t != eval_t::ev_template) {
                nodes_types.insert(static_cast<int>(t));
            } else {
                error("Resolve_types failed to determine types");
            }
            });

    //return *std::max_element(std::begin(nodes_types), std::end(nodes_types)); //implicit number conversions, template->valid type
    if(nodes_types.size() > 1)
        error("Multiple types present in resolve_types container");

    return static_cast<eval_t>(*(nodes_types.begin()));
}

std::string func_manager::mangle_name(const std::shared_ptr<ast>& node)
{
    return mangle_name(node.get());
}

std::string func_manager::mangle_name(const ast *node)
{
    if(node->node_str().size() == 0)
        return std::string();

    std::stringstream ss;
    ss << node->node_str() << "#";
    node->for_nodes([&](const std::shared_ptr<ast>& n) {
            ss << eval_strings[static_cast<int>(n->eval_type())] << "#";
            });

    return ss.str();
}
