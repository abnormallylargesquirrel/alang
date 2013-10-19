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

typedef boost::variant<
    type_variable,
    boost::recursive_wrapper<type_operator>
> type;

typedef std::pair<type, type> constraint;

class type_variable {
public:
    type_variable();
    type_variable(const std::size_t i);
    template<class Iter>
    type_variable(const std::size_t i, Iter first, Iter last)
        : _id(i), _ctx(first, last) {}
    type_variable(const std::size_t i, std::set<std::size_t>&& ctx);

    std::size_t id() const;
    void set_id(std::size_t id);
    bool operator==(const type_variable& other) const;
    bool operator!=(const type_variable& other) const;
    operator std::size_t(void) const;

    std::set<std::size_t>::iterator begin(void);
    std::set<std::size_t>::iterator end(void);

    std::set<std::size_t>::iterator begin() const;
    std::set<std::size_t>::iterator end() const;

    template<class Iter>
    void insert(Iter first, Iter last)
    {
        _ctx.insert(first, last);
    }

private:
    std::size_t _id;
    std::set<std::size_t> _ctx;
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

class unifier : public boost::static_visitor<> {
public:
    void operator()(const type_variable& x, type_variable& y);
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

void unify(const type& x, const type& y, std::map<type_variable, type>& substitution);

#endif
