#include "hm_inference.h"
#include "func_manager.h"
#include "eval_t.h"
#include "utils.h"

type make_function(const type& arg, const type& result)
{
    return type_operator(types::Function, {arg, result});
}

type ty_void(void)
{
    return type_operator(types::Void);
}

type ty_integer(void)
{
    return type_operator(types::Int);
}

type ty_float(void)
{
    return type_operator(types::Float);
}

type ty_bool(void)
{
    return type_operator(types::Bool);
}

type ty_pair(const type& first, const type& second)
{
    return type_operator(types::Pair, {first, second});
}

type definitive(const std::map<type_variable, type>& substitution, const type_variable& x)
{
    type result = x;

    type_variable *ptr = nullptr;
    while((ptr = boost::get<type_variable>(&result)) && substitution.count(*ptr))
    {
        result = substitution.find(*ptr)->second;
    }

    return result;
}

// environment
environment::environment() : _next_id(0) {}
std::size_t environment::unique_id() {return _next_id++;}

// fresh_maker
fresh_maker::fresh_maker(environment& env, const std::set<type_variable>& non_generic,
        const std::map<type_variable, type>& substitution)
    : _env(env), _non_generic(non_generic), _substitution(substitution) {}

type_variable fresh_maker::operator()(const type_variable& var)
{
    if(is_generic(var)) { // generic variables are duplicated, non-generic are shared
        if(!_mappings.count(var)) {
            _mappings[var] = type_variable(_env.unique_id(), var.begin(), var.end());
        }

        return _mappings[var];
    }

    return var;
}

type_operator fresh_maker::operator()(const type_operator& op)
{
    std::vector<type> types(op.size());
    std::transform(op.begin(), op.end(), types.begin(), std::ref(*this)); // make fresh type for each arg to type_operator?
    return type_operator(op.kind(), types);
}

type fresh_maker::operator()(const type& x)
{
    type result;

    if(x.which() == 0) {
        auto definitive_type = definitive(_substitution, boost::get<type_variable>(x));
        result = boost::apply_visitor(*this, definitive_type);
        //result = definitive(_substitution, result);
    } else {
        result = boost::apply_visitor(*this, x);
    }

    return result;
}

bool fresh_maker::is_generic(const type_variable& var) const
{
    bool occurs = false;

    for(auto& i : _non_generic) {
        occurs = detail::occurs(definitive(_substitution, i), var);

        if(occurs)
            break;
    }

    return !occurs;
}

// inferencer
inferencer::inferencer(environment& env, std::map<std::string,
        std::set<std::string>>& dependencies, func_manager& fm)
    : _fm(fm), _environment(env), _dependencies(dependencies) {}
type inferencer::operator()(ast&) {return ty_void();}
type inferencer::operator()(expr_int&) {return ty_integer();}
type inferencer::operator()(expr_float&) {return ty_float();}
//type operator()(expr_bool&) {return ty_bool();}

type inferencer::operator()(expr_sym& id)
{
    auto id_name = id.node_str();
    if(!_environment.count(id_name))
    {
        _dependencies[id_name].insert(_cur_func_name);
        _unbound_vars = true;
        return type_variable(_environment.unique_id());
    }

    auto freshen = _environment[id_name];
    auto v = fresh_maker(_environment, _non_generic_variables, _substitution);

    return v(freshen);
}

type inferencer::operator()(expr_apply& app)
{
    auto fun_type = app[0]->infer_type(*this); // might infer types more than once if app[0] is the function node
    auto arg_type = app[1]->infer_type(*this);

    /*type_printer pp(std::cout);
    std::cout << "(in " << _cur_func_name << ") ";
    std::cout << app.node_str() << ":";
    pp << fun_type;
    std::cout << "  ";
    pp << arg_type;
    std::cout << '\n';*/

    type_variable x(_environment.unique_id());
    auto lhs = make_function(arg_type, x);
    unify(lhs, fun_type, _substitution);

    /*std::cout << "lhs: ";
    pp << lhs;
    std::cout << "\nfun_type: ";
    pp << fun_type;
    std::cout << "\nreturn type: ";
    auto ret = definitive(_substitution, x);
    pp << ret;
    std::cout << "\n";

    std::cout << std::endl;*/
    return definitive(_substitution, x);
}

type inferencer::operator()(expr_if& e)
{
    auto cond_type = e[0]->infer_type(*this);
    auto then_type = e[1]->infer_type(*this);
    auto else_type = e[2]->infer_type(*this);

    unify(then_type, else_type, _substitution);
    return then_type;
}

type inferencer::operator()(binop_apply& app)
{
    auto fun_type = app[0]->infer_type(*this); // infer expr_apply, returns function, elem 0 is second argument to binop
    auto arg_type = app[1]->infer_type(*this);

    if(fun_type.which()) {
        auto arg2_type = boost::get<type_operator>(fun_type)[0];
        unify(arg2_type, arg_type, _substitution); // two arguments to binop are same type
    } else {
        throw std::runtime_error("Failed to get argument types for binop - inside application returned type_variable");
    }

    type_variable x(_environment.unique_id());
    auto lhs = make_function(arg_type, x);

    unify(lhs, fun_type, _substitution);
    return definitive(_substitution, x);
}

type inferencer::operator()(ast_func& f)
{
    _unbound_vars = false;
    _cur_func_name = f.node_str(); // to build dependency map
    std::cout << _cur_func_name << std::endl;
    type_variable tmp_type(_environment.unique_id()); // tmp_type is placeholder for function_type

    {
        scoped_non_generic_variable ng(*this, f.node_str(), tmp_type);

        std::vector<type> arg_types;
        std::vector<scoped_non_generic_variable> scope;
        scope.reserve(static_cast<std::size_t>(f[0]->num_nodes())); // prevent copying, constructor & destructor affect _environment

        f[0]->for_nodes([&](shared_ast& n) {
                type_variable arg_type(_environment.unique_id());
                scope.emplace_back(*this, n->node_str(), arg_type); // each element destructed at end of operator()
                arg_types.push_back(arg_type);
                });

        std::reverse(std::begin(arg_types), std::end(arg_types));

        auto body_type = f[1]->infer_type(*this);
        auto function_type = body_type;

        for(auto& i : arg_types) {
            function_type = make_function(i, function_type);
        }

        type_variable x(_environment.unique_id());
        unify(x, function_type, _substitution);
        function_type = definitive(_substitution, x);

        unify(function_type, tmp_type, _substitution);

    } // destruct scoped_non_generic_variable, make non_generic with same name

    auto ret = definitive(_substitution, tmp_type);
    _environment[f.node_str()] = ret;

    if(!_unbound_vars) {
        auto it = _dependencies.find(f.node_str());
        if(it != std::end(_dependencies)) {
            for(auto& i : it->second) {
                _environment[i] = _fm.lookup_template(i)->infer_type(*this);
            }
        }
    }

    return ret;
}

type infer_type(const shared_ast& node, environment& env,
        std::map<std::string, std::set<std::string>>& dependencies, func_manager& fm)
{
    inferencer v(env, dependencies, fm);
    return node->infer_type(v);
}
