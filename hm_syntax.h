#ifndef HM_SYNTAX_H
#define HM_SYNTAX_H

#include <iostream>
#include <boost/variant.hpp>

namespace syntax
{

class integer_literal {
public:
    integer_literal(const int v)
        : _value(v) {}

    int value(void) const {return _value;}

private:
    int _value;
};

inline std::ostream& operator<<(std::ostream& os, const integer_literal& il)
{
    return os << il.value();
}

class identifier {
public:
    identifier(const std::string& name)
        : _name(name) {}

    const std::string& name(void) const {return _name;}

private:
    std::string _name;
};

inline std::ostream& operator<<(std::ostream& os, const identifier& i)
{
    return os << i.name();
}

class apply;
class lambda;
class let;
class letrec;

typedef boost::variant<
    integer_literal,
    identifier,
    boost::recursive_wrapper<apply>,
    boost::recursive_wrapper<lambda>,
    boost::recursive_wrapper<let>,
    boost::recursive_wrapper<letrec>
> node;

class apply {
public:
    apply(node&& fn, node&& arg)
        : _fn(std::move(fn)), _arg(std::move(arg)) {}

    apply(const node& fn, const node& arg)
        : _fn(fn), _arg(arg) {}

    const node& function(void) const {return _fn;}
    const node& argument(void) const {return _arg;}

private:
    node _fn, _arg;
};

inline std::ostream& operator<<(std::ostream& os, const apply& a)
{
    return os << "(" << a.function() << " " << a.argument() << ")";
}

class lambda {
public:
    lambda(const std::string& param, node&& body)
        : _param(param), _body(std::move(body)) {}

    lambda(const std::string& param, const node& body)
        : _param(param), _body(body) {}

    const std::string& parameter(void) const {return _param;}
    const node& body(void) const {return _body;}

private:
    std::string _param;
    node _body;
};

inline std::ostream& operator<<(std::ostream& os, const lambda& l)
{
    return os << "(fn " << l.parameter() << " => " << l.body() << ")";
}

class let {
public:
    let(const std::string& name, node&& def, node&& body)
        : _name(name), _definition(std::move(def)), _body(std::move(body)) {}

    let(const std::string& name, const node& def, const node& body)
        : _name(name), _definition(def), _body(body) {}

    const std::string& name(void) const {return _name;}
    const node& definition(void) const {return _definition;}
    const node& body(void) const {return _body;}

private:
    std::string _name;
    node _definition, _body;
};

inline std::ostream& operator<<(std::ostream& os, const let& l)
{
    return os << "(let " << l.name() << " = " << l.definition() << " in " << l.body() << ")";
}

class letrec {
public:
    letrec(const std::string& name, node&& def, node&& body)
        : _name(name), _definition(std::move(def)), _body(std::move(body)) {}

    letrec(const std::string& name, const node& def, const node& body)
        : _name(name), _definition(def), _body(body) {}

    const std::string& name(void) const {return _name;}
    const node& definition(void) const {return _definition;}
    const node& body(void) const {return _body;}

private:
    std::string _name;
    node _definition, _body;
};

inline std::ostream& operator<<(std::ostream& os, const letrec& l)
{
    return os << "(letrec " << l.name() << " = " << l.definition() << " in " << l.body() << ")";
}

}

#endif
