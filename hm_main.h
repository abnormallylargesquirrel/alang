#ifndef HM_MAIN_H
#define HM_MAIN_H

#include "hm_inference.h"

class type_printer : public boost::static_visitor<type_printer&> {
public:
    type_printer(std::ostream& os)
        : _os(os), _next_name('a') {}

    type_printer& operator()(const type_variable& x)
    {
        if(!_names.count(x)) {
            std::ostringstream os;
            os << _next_name++;
            _names[x] = os.str();
        }

        _os << _names[x];
        return *this;
    }

    type_printer& operator()(const type_operator& x)
    {
        switch(x.kind()) {
            case types::ev_void:
                _os << "void";
                break;
            case types::ev_integer:
                _os << "int";
                break;
            case types::ev_bool:
                _os << "bool";
                break;
            case types::ev_float:
                _os << "float";
                break;
            case types::ev_function:
                _os << "(";
                *this << x[0];
                _os << " -> ";
                *this << x[1];
                _os << ")";
                break;
            case types::ev_pair:
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
    std::map<type_variable, std::string> _names;
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
    try_infer(environment& e)
        : _env(e) {}

    void operator()(const shared_ast& n) const
    {
        try
        {
            auto result = infer_type(n, _env);

            std::cout << n << " : ";
            type_printer pp(std::cout);
            pp << result << std::endl;
        }
        catch(const recursive_unification& e)
        {
            std::cerr << n << " : ";
            type_printer pp(std::cerr);
            pp << e.what() << ": " << e.x << " in " << e.y << std::endl;
        }
        catch(const type_mismatch& e)
        {
            std::cerr << n << " : ";
            type_printer pp(std::cerr);
            pp << e.what() << ": " << e.x << " != " << e.y << std::endl;
        }
        catch(const std::runtime_error& e)
        {
            std::cerr << n << " : " << e.what() << std::endl;
        }
    }

    environment& _env;
};


#endif
