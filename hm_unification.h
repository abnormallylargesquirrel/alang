#ifndef HM_UNIFICATION_H
#define HM_UNIFICATION_H

#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>

class type_variable;
class type_operator;

typedef boost::variant<
    type_variable,
    boost::recursive_wrapper<type_operator>
> type;

typedef std::pair<type, type> constraint;

class type_variable {
public:
    type_variable();
    type_variable(const std::size_t i);

    std::size_t id() const;
    bool operator==(const type_variable& other) const;
    bool operator!=(const type_variable& other) const;
    bool operator<(const type_variable& other) const;
    operator std::size_t(void) const;

private:
    std::size_t _id;
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

private:
    std::vector<type> _types;
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

struct equals_variable : boost::static_visitor<bool> {
    equals_variable(const type_variable& x);

    bool operator()(const type_variable& y);
    bool operator()(const type_operator&);

    const type_variable& _x;
};

struct replacer : boost::static_visitor<> {
    replacer(const type_variable& replaced);

    void operator()(type_variable& var, const type_variable& replacement);

    template<class T>
    void operator()(type_operator& op, const T& replacement)
    {
        auto visitor = boost::apply_visitor(*this);
        auto f = std::bind(visitor, std::placeholders::_2, replacement);
        for(auto& i : op)
            f(i);
    }

    const type_variable& _replaced;
};

class unifier : public boost::static_visitor<> {
public:
    void operator()(const type_variable& x, const type_variable& y);
    void operator()(const type_variable& x, const type_operator& y);
    void operator()(const type_operator& x, const type_variable& y);
    void operator()(const type_operator& x, const type_operator& y);

    template<class Iter>
    unifier(Iter first_constraint, Iter last_constraint, std::map<type_variable, type>& substitution)
        : _stack(first_constraint, last_constraint), _substitution(substitution)
    {
        //add current substitution to the stack
        _stack.insert(_stack.end(), _substitution.begin(), _substitution.end());
        _substitution.clear();
    }

    void operator()(void);

private:
    void eliminate(const type_variable& x, const type& y);

    std::vector<constraint> _stack;
    std::map<type_variable, type>& _substitution;
};
} //detail

/*template<class Iter>
void unify(Iter first_constraint, Iter last_constraint, std::map<type_variable, type>& substitution)
{
    detail::unifier u(first_constraint, last_constraint, substitution);
    u();
}

template<class Range>
void unify(const Range& r, std::map<type_variable, type>& substitution)
{
    unify(r.begin(), r.end(), substitution);
}*/

//template<>
void unify(const type& x, const type& y, std::map<type_variable, type>& substitution);

/*template<class Range>
std::map<type_variable, type> unify(const Range& r)
{
    std::map<type_variable, type> solutions;
    unify(r, solutions);
    return std::move(solutions);
}*/

#endif
