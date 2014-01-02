#ifndef HM_UNIFICATION_H
#define HM_UNIFICATION_H

#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>

class type_variable;
class type_operator;
class type_class;

typedef boost::variant<
    type_variable,
    boost::recursive_wrapper<type_operator>
> type;

typedef std::pair<type, type> constraint;

// each type_variable id is a key, contexts is set of all class constraints, second set is all variables used to instatiate
typedef std::map<std::size_t, std::pair<std::set<std::string>, std::set<std::size_t>>> contexts;

typedef std::map<std::string, type_class> class_env; // maps class name to type_class which describes instances & superclasses
//typedef std::map<std::string, std::pair<std::size_t, type_class>> class_env;

class type_class {
public:
    type_class(std::initializer_list<std::string>&& super, std::initializer_list<std::size_t>&& insts)
        : _superclasses(super), _instances(insts) {}

    bool is_subclass_of(std::string super) const
    {
        return !(std::find(_superclasses.begin(), _superclasses.end(), super) == _superclasses.end());
    }
    bool has_type_inst(std::size_t ty) const
    {
        return !(std::find(_instances.begin(), _instances.end(), ty) == _instances.end());
    }

    void add_super(std::string super)
    {
        _superclasses.push_back(super);
    }

    void add_inst(std::size_t inst)
    {
        _instances.push_back(inst);
    }

private:
    //std::map<std::string, std::size_t
    std::vector<std::string> _superclasses;
    std::vector<std::size_t> _instances;
};

class type_variable {
public:
    type_variable(void) : _id() {}
    type_variable(const std::size_t i) : _id(i) {}
    template<class Iter>
    type_variable(contexts& ctxs, const std::size_t i, Iter first, Iter last)
        : _id(i)
    {
        ctxs[i].first.insert(first, last);
    }
    type_variable(contexts& ctxs, const std::size_t i, std::set<std::string>&& ctx)
        : _id(i)
    {
        ctxs[i].first.insert(ctx.begin(), ctx.end());
    }

    std::size_t id() const;
    bool operator==(const type_variable& other) const;
    bool operator!=(const type_variable& other) const;
    operator std::size_t(void) const;

    const std::set<std::string>& ctx(void) const
    {
        return _ctx;
    }

    /*void add_inst(std::size_t n)
    {
        _check_insts.insert(n);
    }*/

    void propagate(contexts& ctxs);
    void check_insts(const class_env& ce);

    std::set<std::string>::iterator begin(void);
    std::set<std::string>::iterator end(void);

    std::set<std::string>::iterator begin() const;
    std::set<std::string>::iterator end() const;

private:
    std::size_t _id;
    std::set<std::string> _ctx;
    //std::vector<std::size_t> _check_insts;
    std::set<std::size_t> _check_insts;
};

class type_operator : private std::vector<type> {
public:
    using std::vector<type>::begin;
    using std::vector<type>::end;
    using std::vector<type>::size;
    using std::vector<type>::operator[];

    type_operator(const type_operator& other);
    type_operator(const std::size_t kind);

    template<class Iter>
    type_operator(const std::size_t kind, Iter first, Iter last)
        : std::vector<type>(first, last), _kind(kind) {}

    template<class Range>
    type_operator(const std::size_t kind, const Range& r)
        : std::vector<type>(r.begin(), r.end()), _kind(kind) {}

    type_operator(const std::size_t kind, std::initializer_list<type>&& types);
    type_operator(type_operator&& other);

    type_operator& operator=(const type_operator& other);

    std::size_t kind(void) const;
    bool compare_kind(const type_operator& other) const;
    bool operator==(const type_operator& other) const;

    void propagate(contexts& ctxs);
    void check_insts(const class_env& ce);

private:
    //std::vector<type> _types;
    std::size_t _kind;
};

struct type_mismatch : std::runtime_error {
    type_mismatch(const type& xx, const type& yy)
        : std::runtime_error("type mismatch"), x(xx), y(yy) {}
    virtual ~type_mismatch(void) throw() {}

    type x;
    type y;
};

struct recursive_unification : std::runtime_error {
    recursive_unification(const type& xx, const type& yy)
        : std::runtime_error("recursive unification"), x(xx), y(yy) {}

    virtual ~recursive_unification(void) throw() {}

    type x;
    type y;
};

namespace detail
{
void replace(type& x, const type_variable& replaced, const type& replacer);
bool occurs(const type& haystack, const type_variable& needle);

class unifier : public boost::static_visitor<> {
public:
    void operator()(const type_variable& x, type_variable& y);
    void operator()(const type_variable& x, const type_operator& y);
    void operator()(const type_operator& x, const type_variable& y);
    void operator()(const type_operator& x, const type_operator& y);

    template<class Iter>
    unifier(Iter first_constraint, Iter last_constraint, contexts& ctxs,
            std::map<type_variable, type>& substitution)
        : _ctxs(ctxs), _stack(first_constraint, last_constraint), _substitution(substitution)
    {
        //add current substitution to the stack
        _stack.insert(_stack.end(), _substitution.begin(), _substitution.end());
        _substitution.clear();
    }

    void operator()(void);

private:
    void eliminate(const type_variable& x, const type& y);

    contexts& _ctxs;
    std::vector<constraint> _stack;
    std::map<type_variable, type>& _substitution;
};
} //detail

void unify(const type& x, const type& y, contexts& ctxs, std::map<type_variable, type>& substitution);

#endif
