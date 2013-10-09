#include "hm_inference.h"

type make_function(const type& arg, const type& result)
{
    return type_operator(types::ev_function, {arg, result});
}

type ty_void(void)
{
    return type_operator(types::ev_void);
}

type ty_integer(void)
{
    return type_operator(types::ev_integer);
}

type ty_float(void)
{
    return type_operator(types::ev_float);
}

type ty_bool(void)
{
    return type_operator(types::ev_bool);
}

type ty_pair(const type& first, const type& second)
{
    return type_operator(types::ev_pair, {first, second});
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
            _mappings[var] = type_variable(_env.unique_id());
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

    for(auto& i : _non_generic) { //try standard for on iter
        occurs = detail::occurs(definitive(_substitution, i), var);

        if(occurs)
            break;
    }

    return !occurs;
}

// inferencer
inferencer::inferencer(environment& env) : _environment(env) {}
type inferencer::operator()(ast&) {return ty_void();}
type inferencer::operator()(expr_int&) {return ty_integer();}
type inferencer::operator()(expr_float&) {return ty_float();}
//type operator()(expr_bool&) {return ty_bool();}

type inferencer::operator()(expr_sym& id)
{
    if(!_environment.count(id.node_str()))
    {
        auto what = "Undefined symbol: " + id.node_str();
        throw std::runtime_error(what);
    }

    //auto freshen = definitive(_substitution, _environment[id.node_str()]);
    auto freshen = _environment[id.node_str()];
    auto v = fresh_maker(_environment, _non_generic_variables, _substitution);

    return v(freshen);
}

type inferencer::operator()(expr_apply& app)
{
    auto fun_type = app[0]->infer_type(*this); // look at func_template, not string
    auto arg_type = app[1]->infer_type(*this);

    type_variable x(_environment.unique_id());
    auto lhs = make_function(arg_type, x);

    unify(lhs, fun_type, _substitution);
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

type inferencer::operator()(expr_binop& e)
{
    auto arg0_type = e[0]->infer_type(*this);
    auto arg1_type = e[1]->infer_type(*this);

    unify(arg0_type, arg1_type, _substitution);
    return arg0_type;
}

type inferencer::operator()(ast_func& f)
{
    type_variable tmp_type(_environment.unique_id()); // tmp_type is placehodler for function_type
    {
        scoped_non_generic_variable ng(this, f[0]->node_str(), tmp_type);

        std::vector<type> arg_types;
        std::vector<scoped_non_generic_variable> scope;
        scope.reserve(static_cast<std::size_t>(f[0]->num_nodes())); // prevent copying, constructor & destructor affect _environment

        f[0]->for_nodes([&](shared_ast& n) {
                type_variable arg_type(_environment.unique_id());
                scope.emplace_back(this, n->node_str(), arg_type); // each element destructed at end of operator()
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
    _environment[f[0]->node_str()] = ret;
    return ret;
}

type infer_type(const shared_ast& node, environment& env)
{
    inferencer v(env);
    return node->infer_type(v);
}
