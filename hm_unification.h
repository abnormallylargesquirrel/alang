#ifndef HM_UNIFICATION_H
#define HM_UNIFICATION_H

#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>

namespace hm
{
class type_variable;
class type_operator;

typedef boost::variant<
    type_variable,
    boost::recursive_wrapper<type_operator>
> type;

typedef std::pair<type, type> constraint;

class type_variable {
public:
    type_variable() : _id() {}
    type_variable(const std::size_t i) : _id(i) {}

    std::size_t id() const {return _id;}
    bool operator==(const type_variable& other) const {return id() == other.id();}
    bool operator!=(const type_variable& other) const {return !(*this == other);}
    bool operator<(const type_variable& other) const {return id() < other.id();}
    operator std::size_t(void) const {return id();}

private:
    std::size_t _id;
};

class type_operator : private std::vector<type> {
public:
    using std::vector<type>::begin;
    using std::vector<type>::end;
    using std::vector<type>::size;
    using std::vector<type>::operator[];

    type_operator(const type_operator& other)
        : std::vector<type>(other), _types(other._types), _kind(other._kind) {}
    type_operator(const std::size_t kind) : _kind(kind) {}

    template<class Iter>
        type_operator(const std::size_t kind, Iter first, Iter last)
            : std::vector<type>(first, last), _kind(kind) {}

    template<class Range>
        type_operator(const std::size_t kind, const Range& r)
            : std::vector<type>(r.begin(), r.end()), _kind(kind) {}

    type_operator(const std::size_t kind, std::initializer_list<type>&& types)
        : std::vector<type>(types), _kind(kind) {}
    type_operator(type_operator&& other)
        : std::vector<type>(std::move(other)), _kind(std::move(other._kind)) {}

    type_operator& operator=(const type_operator& other)
    {
        std::vector<type>::operator=(other);
        _types = other._types;
        _kind = other._kind;
        return *this;
    }

    const std::size_t kind(void) const {return _kind;}
    bool compare_kind(const type_operator& other) const
    {
        return kind() == other.kind() && size() == other.size();
    }
    bool operator==(const type_operator& other) const
    {
        return compare_kind(other) & std::equal(begin(), end(), other.begin());
    }

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
void replace(type& x, const type_variable& replaced, const type& replacer)
{
    if(x.which()) { //0 = type_variable, 1 = type_operator
        auto& op = boost::get<type_operator>(x);
        auto f = std::bind(replace, std::placeholders::_1, replaced, replacer);
        for(auto& i : op) //try for_each
            f(i);
    } else {
        auto& var = boost::get<type_variable>(x);
        if(var == replaced)
            x = replacer;
    }
}

bool occurs(const type& haystack, const type_variable& needle)
{
    bool result = false;
    if(haystack.which()) {
        auto& op = boost::get<type_operator>(haystack);
        auto f = std::bind(occurs, std::placeholders::_1, needle);
        result = std::any_of(op.begin(), op.end(), f);
    } else {
        auto& var = boost::get<type_variable>(haystack);
        result = (var == needle);
    }

    return result;
}

struct equals_variable : boost::static_visitor<bool> {
    equals_variable(const type_variable& x)
        : _x(x) {}

    bool operator()(const type_variable& y) {return _x == y;} //const function?
    bool operator()(const type_operator& y) {return false;}

    const type_variable& _x;
};

struct replacer : boost::static_visitor<> {
    replacer(const type_variable& replaced)
        : _replaced(replaced) {}

    void operator()(type_variable& var, const type_variable& replacement)
    {
        if(var == _replaced)
            var = replacement;
    }

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
    void operator()(const type_variable& x, const type_variable& y)
    {
        if(x != y)
            eliminate(x, y);
    }

    void operator()(const type_variable& x, const type_operator& y)
    {
        if(occurs(y, x))
            throw recursive_unification(x, y);

        eliminate(x, y);
    }

    void operator()(const type_operator& x, const type_variable& y)
    {
        if(occurs(x, y))
            throw recursive_unification(y, x);

        eliminate(y, x);
    }

    void operator()(const type_operator& x, const type_operator& y)
    {
        if(!x.compare_kind(y))
            throw type_mismatch(x, y);

        for(auto xi = x.begin(), yi = y.begin(); xi != x.end(); xi++, yi++) {
            _stack.push_back(std::make_pair(*xi, *yi));
        }
    }

    template<class Iter>
        unifier(Iter first_constraint, Iter last_constraint, std::map<type_variable, type>& substitution)
            : _stack(first_constraint, last_constraint), _substitution(substitution)
        {
            //add current substitution to the stack (possibly unnecessary)
            _stack.insert(_stack.end(), _substitution.begin(), _substitution.end());
            _substitution.clear();
        }

    void operator()(void)
    {
        while(!_stack.empty()) {
            type x = std::move(_stack.back().first);
            type y = std::move(_stack.back().second);
            _stack.pop_back();

            boost::apply_visitor(*this, x, y);
        }
    }

private:
    void eliminate(const type_variable& x, const type& y) //replace all occurences of x with y in stack and the substitution
    {
        for(auto i = _stack.begin(); i != _stack.end(); i++) { //try standard for loop
            replace(i->first, x, y);
            replace(i->second, x, y);
        }

        for(auto i = _substitution.begin(); i != _substitution.end(); i++) {
            replace(i->second, x, y);
        }

        _substitution[x] = y;
    }

    std::vector<constraint> _stack;
    std::map<type_variable, type>& _substitution;
};
} //detail

template<class Iter>
void unify(Iter first_constraint, Iter last_constraint, std::map<type_variable, type>& substitution)
{
    detail::unifier u(first_constraint, last_constraint, substitution);
    u();
}

template<class Range>
void unify(const Range& r, std::map<type_variable, type>& substitution)
{
    return unify(r.begin(), r.end(), substitution);
}

void unify(const type& x, const type& y, std::map<type_variable, type>& substitution)
{
    auto c = constraint(x, y);
    return unify(&c, &c + 1, substitution);
}

template<class Range>
std::map<type_variable, type> unify(const Range& r)
{
    std::map<type_variable, type> solutions;
    unify(r, solutions);
    return std::move(solutions);
}
} //hm

#endif
