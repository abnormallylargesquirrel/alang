#include "hm_inference.h"
#include "func_manager.h"
#include "eval_t.h"
#include "utils.h"
#include "hm_main.h"

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

type ty_str(void)
{
    return type_operator(types::Str);
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
fresh_maker::fresh_maker(contexts& ctxs, environment& env, const std::set<type_variable>& non_generic,
        const std::map<type_variable, type>& substitution)
    : _ctxs(ctxs), _env(env), _non_generic(non_generic), _substitution(substitution) {}

type_variable fresh_maker::operator()(const type_variable& var)
{
    if(is_generic(var)) { // generic variables are duplicated, non-generic are shared
        if(!_mappings.count(var)) {
            auto it = _ctxs.find(var.id());
            if(it != _ctxs.end())
                _mappings[var] = type_variable(_ctxs, _env.unique_id(), it->second.first.begin(), it->second.first.end());
            else
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

    for(auto& i : _non_generic) {
        occurs = detail::occurs(definitive(_substitution, i), var);

        if(occurs)
            break;
    }

    return !occurs;
}

// inferencer
inferencer::inferencer(contexts& ctxs, environment& env, std::map<std::string,
        std::set<std::string>>& dependencies, func_manager& fm)
    : _fm(fm), _ctxs(ctxs), _environment(env), _dependencies(dependencies) {}

type inferencer::operator()(ast&) {return ty_void();}
type inferencer::operator()(ast_str&) {return ty_str();}
type inferencer::operator()(ast_int&) {return ty_integer();}
type inferencer::operator()(ast_float&) {return ty_float();}
//type inferencer::operator()(ast_bool&) {return ty_bool();}
type inferencer::operator()(ast_sym& id)
{
    auto id_name = id.node_str();
    if(!_environment.count(id_name))
    {
        _dependencies[id_name].insert(_cur_func_name);
        _unbound_vars = true;
        return type_variable(_environment.unique_id());
    }

    auto freshen = _environment[id_name];
    auto v = fresh_maker(_ctxs, _environment, _non_generic_variables, _substitution);

    return v(freshen);
}

type inferencer::infer_apply(ast_cons& app)
{
    if(!app.cdr()) {
        std::cout << "apply err 0" << std::endl;
        if(app.car()) {
            std::cout << "apply err 1" << std::endl;
            return app.car()->infer_type(*this);
        } else
            throw std::runtime_error("No available nodes in infer_apply");
    }

    std::vector<shared_ast> args;
    std::vector<type> arg_types;

    auto fun_type = app.car()->infer_type(*this);
    //auto arg_type = app.cdr()->infer_type(*this);
    sexp::map_effect([&](const shared_ast& a) {
            args.push_back(a);
            }, app.cdr());

    for(const auto& i : args)
        arg_types.push_back(i->infer_type(*this));

    std::reverse(arg_types.begin(), arg_types.end());

    type_variable x(_environment.unique_id());
    type lhs = x;
    //type lhs = make_function(arg_type, x);

    for(auto& i : arg_types)
        lhs = make_function(i, lhs);

    unify(lhs, fun_type, _ctxs, _substitution);

    return definitive(_substitution, x);
}

type inferencer::infer_if(ast_cons& a)
{
    if(!a.cdr())
        throw std::runtime_error("'if' expression missing condition");
    if(!a.cdr()->cdr())
        throw std::runtime_error("'if' expression missing then expression");
    if(!a.cdr()->cdr()->cdr())
        throw std::runtime_error("'if' expression missing else expression");

    auto cond_type = a.cdr()->car()->infer_type(*this);
    auto then_type = a.cdr()->cdr()->car()->infer_type(*this);
    auto else_type = a.cdr()->cdr()->cdr()->car()->infer_type(*this);

    unify(then_type, else_type, _ctxs, _substitution);
    return then_type;
}

type inferencer::infer_func(ast_cons& f)
{
    _unbound_vars = false;
    _cur_func_name = sexp::get_func_name(f);
    type_variable tmp_type(_environment.unique_id());

    {
        scoped_non_generic_variable ng(*this, sexp::get_func_name(f), tmp_type);

        std::vector<type> arg_types;
        std::vector<scoped_non_generic_variable> scope;
        scope.reserve(sexp::get_length(sexp::get_proto(f)) - 1); // number of arguments

        sexp::map_effect([&](const shared_ast& a) {
                //if(a->cdr()) {
                    type_variable arg_type(_environment.unique_id());
                    if(a->car())
                        scope.emplace_back(*this, a->car()->node_str(), arg_type);
                    else
                        scope.emplace_back(*this, a->node_str(), arg_type);
                    arg_types.push_back(arg_type);
                //}
                }, sexp::get_proto(f)->cdr());

        std::reverse(arg_types.begin(), arg_types.end());

        auto body_type = sexp::get_body(f)->infer_type(*this);
        auto function_type = body_type;

        for(auto& i : arg_types) {
            function_type = make_function(i, function_type);
        }

        type_variable x(_environment.unique_id());
        unify(x, function_type, _ctxs, _substitution);
        function_type = definitive(_substitution, x);

        unify(function_type, tmp_type, _ctxs, _substitution);
    } // destruct scoped_non_generic_variable

    auto ret = definitive(_substitution, tmp_type);
    _environment[sexp::get_func_name(f)] = ret;

    if(!_unbound_vars) {
        auto it = _dependencies.find(sexp::get_func_name(f));
        if(it != std::end(_dependencies)) {
            for(auto& i : it->second) {
                _environment[i] = _fm.lookup_ast_cons(i)->infer_type(*this);
            }
        }
    }

    return ret;
}

type inferencer::operator()(ast_cons& a)
{
    if(!a.cdr()) {
        if(a.car()) {
            return a.car()->infer_type(*this);
        } else {
            throw std::runtime_error("No possible inference method for ast_cons");
        }
    } else if(sexp::check_first_str(a, "define")) {
        return infer_func(a);
    } else if(sexp::check_first_str(a, "if")) {
        return infer_if(a);
    } else {
        //sexp::rotate_left_list(a);
        //std::cout << a.node_str() << std::endl;
        return infer_apply(a);
    }
}

type infer_type(const shared_ast& node, contexts& ctxs, environment& env,
        std::map<std::string, std::set<std::string>>& dependencies, func_manager& fm)
{
    inferencer v(ctxs, env, dependencies, fm);
    return node->infer_type(v);
}
