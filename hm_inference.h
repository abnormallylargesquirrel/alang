#ifndef HM_INFERENCE_H
#define HM_INFERENCE_H

#include <map>
#include <set>
#include <algorithm>
#include <utility>
#include <boost/variant/static_visitor.hpp>
#include "hm_unification.h"
#include "ast.h"

namespace types
{
static const int ev_void = 0;
static const int ev_integer = 1;
static const int ev_float = 2;
static const int ev_bool = 3;
static const int ev_function = 4;
static const int ev_pair = 5;
}

type make_function(const type& arg, const type& result);
type ty_void(void);
type ty_integer(void);
type ty_float(void);
type ty_bool(void);
type ty_pair(const type& first, const type& second);
type definitive(const std::map<type_variable, type>& substitution, const type_variable& x);

class environment : public std::map<std::string, type> {
public:
    environment(void);
    std::size_t unique_id();
private:
    std::size_t _next_id;
};

struct fresh_maker : boost::static_visitor<type> { // make a copy of a type expression
    fresh_maker(environment& env, const std::set<type_variable>& non_generic,
            const std::map<type_variable, type>& substitution);

    type_variable operator()(const type_variable& var);
    type_operator operator()(const type_operator& op);
    type operator()(const type& x);
private:
    bool is_generic(const type_variable& var) const;

    environment& _env;
    const std::set<type_variable>& _non_generic;
    const std::map<type_variable, type>& _substitution;
    std::map<type_variable, type_variable> _mappings;
};

struct inferencer : boost::static_visitor<type> {
    inferencer(environment& env);

    type operator()(ast&);
    type operator()(expr_int&);
    type operator()(expr_float&);
    //type operator()(expr_bool&);
    type operator()(expr_sym& id);
    type operator()(expr_apply& app);
    type operator()(expr_if& e);
    type operator()(expr_binop& e);
    type operator()(ast_func& f);

    struct scoped_generic { // (and non-generic) remove destructor for non-scoped
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

    environment& _environment;
    std::set<type_variable> _non_generic_variables;
    std::map<type_variable, type> _substitution;
};

type infer_type(const shared_ast& node, environment& env);

#endif
