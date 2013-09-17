#ifndef HM_INFERENCE_H
#define HM_INFERENCE_H

#include <map>
#include <set>
#include <algorithm>
#include <utility>
#include <boost/variant/static_visitor.hpp>
#include "hm_unification.h"
#include "hm_syntax.h"

namespace hm
{
namespace types
{
static const int ev_integer = 0;
static const int ev_boolean = 1;
static const int ev_function = 2;
static const int ev_pair = 3;
}

type make_function(const type& arg, const type& result)
{
    return type_operator(types::ev_function, {arg, result});
}

type ty_integer(void)
{
    return type_operator(types::ev_integer);
}

type ty_boolean(void)
{
    return type_operator(types::ev_boolean);
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

class environment : public std::map<std::string, type> {
public:
    environment()
        : _next_id(0) {}

    std::size_t unique_id() {return _next_id++;}

private:
    std::size_t _next_id;
};

struct fresh_maker : boost::static_visitor<type> {
    fresh_maker(environment& env, const std::set<type_variable>& non_generic,
            const std::map<type_variable, type>& substitution)
        : _env(env), _non_generic(non_generic), _substitution(substitution) {}

    result_type operator()(const type_variable& var)
    {
        if(is_generic(var))
        {
            if(!_mappings.count(var))
            {
                _mappings[var] = type_variable(_env.unique_id());
            }

            return _mappings[var];
        }

        return var;
    }

    result_type operator()(const type_operator& op)
    {
        std::vector<type> types(op.size());
        std::transform(op.begin(), op.end(), types.begin(), std::ref(*this));
        return type_operator(op.kind(), types);
    }

    result_type operator()(const type& x)
    {
        result_type result;

        if(x.which() == 0) {
            auto definitive_type = definitive(_substitution, boost::get<type_variable>(x));
            result = boost::apply_visitor(*this, definitive_type);
        } else {
            result = boost::apply_visitor(*this, x);
        }

        return result;
    }

private:
    bool is_generic(const type_variable& var) const
    {
        bool occurs = false;

        for(auto& i : _non_generic) { //try standard for on iter
            occurs = detail::occurs(definitive(_substitution, i), var);

            if(occurs)
                break;
        }

        return !occurs;
    }

    environment& _env;
    const std::set<type_variable>& _non_generic;
    const std::map<type_variable, type>& _substitution;
    std::map<type_variable, type_variable> _mappings;
};

//struct inferencer: change functions to one argument->return value
struct inferencer : boost::static_visitor<type> {
    inferencer(const environment& env)
        : _environment(env) {}

    result_type operator()(const syntax::integer_literal) {return ty_integer();}
    result_type operator()(const syntax::identifier& id)
    {
        if(!_environment.count(id.name()))
        {
            auto what = std::string("Undefined symbol ") + id.name();
            throw std::runtime_error(what);
        }

        auto freshen_me = _environment[id.name()];
        auto v = fresh_maker(_environment, _non_generic_variables, _substitution);
        return v(freshen_me);
    }

    result_type operator()(const syntax::apply& app)
    {
        auto fun_type = boost::apply_visitor(*this, app.function());
        auto arg_type = boost::apply_visitor(*this, app.argument());

        auto x = type_variable(_environment.unique_id());
        auto lhs = make_function(arg_type, x);

        unify(lhs, fun_type, _substitution);
        return definitive(_substitution, x);
    }

    result_type operator()(const syntax::lambda& lambda)
    {
        auto arg_type = type_variable(_environment.unique_id());
        auto s = scoped_non_generic_variable(this, lambda.parameter(), arg_type);
        auto body_type = boost::apply_visitor(*this, lambda.body());
        auto x = type_variable(_environment.unique_id());

        unify(x, make_function(arg_type, body_type), _substitution);
        return definitive(_substitution, x);
    }

    result_type operator()(const syntax::let& let)
    {
        auto defn_type = boost::apply_visitor(*this, let.definition());
        auto s = scoped_generic(this, let.name(), defn_type);

        auto result = boost::apply_visitor(*this, let.body());
        return result;
    }

    result_type operator()(const syntax::letrec& letrec)
    {
        auto new_type = type_variable(_environment.unique_id());
        auto s = scoped_non_generic_variable(this, letrec.name(), new_type);
        auto definition_type = boost::apply_visitor(*this, letrec.definition());

        unify(new_type, definition_type, _substitution);
        auto result = boost::apply_visitor(*this, letrec.body());
        return result;
    }

    struct scoped_generic {
        scoped_generic(inferencer *inf, const std::string& name, const type& t)
            : _environment(inf->_environment)
        {
            auto iter = _environment.find(name);
            if(iter != _environment.end()) {
                _restore = std::make_tuple(true, iter, iter->second);
                iter->second = t;
            } else {
                auto kv = std::make_pair(name, t);
                _restore = std::make_tuple(false, _environment.insert(kv).first, type());
            }
        }

        ~scoped_generic()
        {
            if(std::get<0>(_restore)) {
                auto iter = std::get<1>(_restore);
                auto val = std::get<2>(_restore);

                iter->second = val;
            } else {
                auto iter = std::get<1>(_restore);
                _environment.erase(iter);
            }
        }

        environment& _environment;
        std::tuple<bool, environment::iterator, type> _restore;
    };

    struct scoped_non_generic_variable : scoped_generic {
        scoped_non_generic_variable(inferencer *inf, const std::string& name, const type_variable& var)
            : scoped_generic(inf, name, var),
            _non_generic(inf->_non_generic_variables),
            _erase_me(_non_generic.insert(var))
        {}

        ~scoped_non_generic_variable()
        {
            if(_erase_me.second)
                _non_generic.erase(_erase_me.first);
        }

        std::set<type_variable>& _non_generic;
        std::pair<std::set<type_variable>::iterator, bool> _erase_me;
    };

    environment _environment;
    std::set<type_variable> _non_generic_variables;
    std::map<type_variable, type> _substitution;
};

type infer_type(const syntax::node& node, const environment& env)
{
    auto v = inferencer(env);
    //auto old = std::clog.rdbuf(0);
    auto result = boost::apply_visitor(v, node);
    //std::clog.rdbuf(old);
    return result;
}

}
#endif
