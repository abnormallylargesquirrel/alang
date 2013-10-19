#ifndef HM_MAIN_H
#define HM_MAIN_H

#include "hm_inference.h"

class func_manager;

class type_printer : public boost::static_visitor<type_printer&> {
public:
    type_printer(std::ostream& os)
        : _os(os), _next_name('a') {}

    type_printer& operator()(const type_variable& x)
    {
        _os << '{';
        for(const auto& i : x) {
            switch(i) {
                case classes::Num:
                    _os << "Num ";
                    break;
                case classes::Ord:
                    _os << "Ord ";
                    break;
                case classes::Eq:
                    _os << "Eq ";
                    break;
                default:
                    break;
            }
        }
        if(!_names.count(x.id())) {
            std::ostringstream os;
            os << _next_name++;
            _names[x.id()] = os.str();
        }

        _os << _names[x.id()] << '}';
        return *this;
    }

    type_printer& operator()(const type_operator& x)
    {
        switch(x.kind()) {
            case types::Void:
                _os << "Void";
                break;
            case types::Int:
                _os << "Int";
                break;
            case types::Bool:
                _os << "Bool";
                break;
            case types::Float:
                _os << "Float";
                break;
            case types::Function:
                _os << "(";
                *this << x[0];
                _os << " -> ";
                *this << x[1];
                _os << ")";
                break;
            case types::Pair:
                _os << "(";
                *this << x[0];
                _os << " * ";
                *this << x[1];
                _os << ")";
                break;
            default:
                break;
        }

        return *this;
    }

    type_printer& operator<<(const type& x)
    {
        return boost::apply_visitor(*this, x);
    }

    type_printer& operator<<(std::ostream& (*fp)(std::ostream&))
    {
        fp(_os);
        return *this;
    }

    template<class T>
        type_printer& operator<<(const T& x)
        {
            _os << x;
            return *this;
        }

private:
    std::ostream& _os;
    std::map<std::size_t, std::string> _names;
    char _next_name;
};

inline std::ostream& operator<<(std::ostream& os, const type_variable& x)
{
    type_printer pp(os);
    pp << type(x);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const type_operator& x)
{
    type_printer pp(os);
    pp << type(x);
    return os;
}

struct try_infer {
    try_infer(environment& e, std::map<std::string, std::set<std::string>>& dependencies, func_manager& fm)
        : _env(e), _dependencies(dependencies), _fm(fm)
    {
        type_variable v0(_env.unique_id(), {classes::Num});
        _env["+"] = make_function(v0, make_function(v0, v0));

        type_variable v1(_env.unique_id(), {classes::Num});
        _env["-"] = make_function(v1, make_function(v1, v1));

        type_variable v2(_env.unique_id(), {classes::Num});
        _env["*"] = make_function(v2, make_function(v2, v2));

        type_variable v3(_env.unique_id(), {classes::Num});
        _env["/"] = make_function(v3, make_function(v3, v3));

        type_variable v4(_env.unique_id(), {classes::Ord});
        _env["<"] = make_function(v4, make_function(v4, ty_bool()));

        type_variable v5(_env.unique_id(), {classes::Ord});
        _env[">"] = make_function(v5, make_function(v5, ty_bool()));

        type_variable v6(_env.unique_id(), {classes::Ord, classes::Eq});
        _env["<="] = make_function(v6, make_function(v6, ty_bool()));

        type_variable v7(_env.unique_id(), {classes::Ord, classes::Eq});
        _env[">="] = make_function(v7, make_function(v7, ty_bool()));

        type_variable v8(_env.unique_id(), {classes::Eq});
        _env["=="] = make_function(v8, make_function(v8, ty_bool()));
    }

    void operator()(const shared_ast& n) const
    {
        try
        {
            auto result = infer_type(n, _env, _dependencies, _fm);
        }
        catch(const recursive_unification& e)
        {
            std::cerr << n << " : ";
            type_printer pp(std::cerr);
            pp << e.what() << ": " << e.x << " in " << e.y << std::endl;
            throw std::runtime_error("Type error");
        }
        catch(const type_mismatch& e)
        {
            std::cerr << n << " : ";
            type_printer pp(std::cerr);
            pp << e.what() << ": " << e.x << " != " << e.y << std::endl;
            throw std::runtime_error("Type error");
        }
        catch(const std::runtime_error& e)
        {
            std::cerr << n << " : " << e.what() << std::endl;
            throw std::runtime_error("Type error");
        }
    }

    environment& _env;
    std::map<std::string, std::set<std::string>>& _dependencies;
    func_manager& _fm;
};


#endif
