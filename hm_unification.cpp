#include "hm_unification.h"

type_variable::type_variable() : _id() {}
type_variable::type_variable(const std::size_t i) : _id(i) {}

std::size_t type_variable::id() const {return _id;}
bool type_variable::operator==(const type_variable& other) const {return id() == other.id();}
bool type_variable::operator!=(const type_variable& other) const {return !(*this == other);}
bool type_variable::operator<(const type_variable& other) const {return id() < other.id();}
type_variable::operator std::size_t(void) const {return id();}

type_operator::type_operator(const type_operator& other)
    : std::vector<type>(other), _types(other._types), _kind(other._kind) {}
type_operator::type_operator(const std::size_t kind) : _kind(kind) {}

/*template<class Iter>
type_operator::type_operator(const std::size_t kind, Iter first, Iter last)
    : std::vector<type>(first, last), _kind(kind) {}*/

/*template<class Range>
type_operator::type_operator(const std::size_t kind, const Range& r)
    : std::vector<type>(r.begin(), r.end()), _kind(kind) {}*/

type_operator::type_operator(const std::size_t kind, std::initializer_list<type>&& types)
    : std::vector<type>(types), _kind(kind) {}
type_operator::type_operator(type_operator&& other)
    : std::vector<type>(std::move(other)), _kind(std::move(other._kind)) {}

type_operator& type_operator::operator=(const type_operator& other)
{
    std::vector<type>::operator=(other);
    _types = other._types;
    _kind = other._kind;
    return *this;
}

std::size_t type_operator::kind(void) const {return _kind;}
bool type_operator::compare_kind(const type_operator& other) const
{
    return kind() == other.kind() && size() == other.size();
}
bool type_operator::operator==(const type_operator& other) const
{
    return compare_kind(other) & std::equal(begin(), end(), other.begin());
}

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

equals_variable::equals_variable(const type_variable& x) : _x(x) {}

bool equals_variable::operator()(const type_variable& y) {return _x == y;} //const function?
bool equals_variable::operator()(const type_operator&) {return false;}

replacer::replacer(const type_variable& replaced) : _replaced(replaced) {}

void replacer::operator()(type_variable& var, const type_variable& replacement)
{
    if(var == _replaced)
        var = replacement;
}

/*template<class T>
    void replacer::operator()(type_operator& op, const T& replacement)
    {
        auto visitor = boost::apply_visitor(*this);
        auto f = std::bind(visitor, std::placeholders::_2, replacement);
        for(auto& i : op)
            f(i);
    }*/

void unifier::operator()(const type_variable& x, const type_variable& y)
{
    if(x != y)
        eliminate(x, y);
}

void unifier::operator()(const type_variable& x, const type_operator& y)
{
    if(occurs(y, x))
        throw recursive_unification(x, y);

    eliminate(x, y);
}

void unifier::operator()(const type_operator& x, const type_variable& y)
{
    if(occurs(x, y))
        throw recursive_unification(y, x);

    eliminate(y, x);
}

void unifier::operator()(const type_operator& x, const type_operator& y)
{
    if(!x.compare_kind(y))
        throw type_mismatch(x, y);

    for(auto xi = x.begin(), yi = y.begin(); xi != x.end(); xi++, yi++) {
        _stack.push_back(std::make_pair(*xi, *yi));
    }
}

void unifier::operator()(void)
{
    while(!_stack.empty()) {
        type x = std::move(_stack.back().first);
        type y = std::move(_stack.back().second);
        _stack.pop_back();

        boost::apply_visitor(*this, x, y);
    }
}

void unifier::eliminate(const type_variable& x, const type& y) //replace all occurences of x with y in stack and the substitution
{
    for(auto i = _stack.begin(); i != _stack.end(); i++) {
        replace(i->first, x, y);
        replace(i->second, x, y);
    }

    for(auto i = _substitution.begin(); i != _substitution.end(); i++) {
        replace(i->second, x, y);
    }

    _substitution[x] = y;
}
}

//template<>
void unify(const type& x, const type& y, std::map<type_variable, type>& substitution)
{
    auto c = constraint(x, y);
    detail::unifier u(&c, &c + 1, substitution);
    u();
    //unify(&c, &c + 1, substitution);
}
