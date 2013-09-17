#include <iostream>
#include <sstream>
#include "hm_unification.h"
#include "hm_syntax.h"
#include "hm_inference.h"

namespace hm
{

class pretty_printer : public boost::static_visitor<pretty_printer&> {
public:
    pretty_printer(std::ostream& os)
        : _os(os), _next_name('a') {}

    pretty_printer& operator()(const type_variable& x)
    {
        if(!_names.count(x)) {
            std::ostringstream os;
            os << _next_name++;
            _names[x] = os.str();
        }

        _os << _names[x];
        return *this;
    }

    pretty_printer& operator()(const type_operator& x)
    {
        switch(x.kind()) {
            case types::ev_integer:
                _os << "int";
                break;
            case types::ev_boolean:
                _os << "bool";
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

    pretty_printer& operator<<(const type& x)
    {
        return boost::apply_visitor(*this, x);
    }

    pretty_printer& operator<<(std::ostream& (*fp)(std::ostream&))
    {
        fp(_os);
        return *this;
    }

    template<class T>
        pretty_printer& operator<<(const T& x)
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
    pretty_printer pp(os);
    pp << type(x);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const type_operator& x)
{
    pretty_printer pp(os);
    pp << type(x);
    return os;
}

}

struct try_infer {
    try_infer(const hm::environment& e)
        : _env(e) {}

    void operator()(const syntax::node& n) const
    {
        try
        {
            auto result = hm::infer_type(n, _env);

            std::cout << n << " : ";
            hm::pretty_printer pp(std::cout);
            pp << result << std::endl;
        }
        catch(const hm::recursive_unification& e)
        {
            std::cerr << n << " : ";
            hm::pretty_printer pp(std::cerr);
            pp << e.what() << ": " << e.x << " in " << e.y << std::endl;
        }
        catch(const hm::type_mismatch& e)
        {
            std::cerr << n << " : ";
            hm::pretty_printer pp(std::cerr);
            pp << e.what() << ": " << e.x << " != " << e.y << std::endl;
        }
        catch(const std::runtime_error& e)
        {
            std::cerr << n << " : " << e.what() << std::endl;
        }
    }

    const hm::environment& _env;
};

int main()
{
    using namespace hm;
    using namespace syntax;
    environment env;
    std::vector<syntax::node> examples;

    auto var1 = type_variable(env.unique_id());
    auto var2 = type_variable(env.unique_id());
    auto var3 = type_variable(env.unique_id());

    env["pair"] = make_function(var1, make_function(var2, ty_pair(var1, var2)));
    env["true"] = ty_boolean();
    env["cond"] = make_function(
                ty_boolean(),
                make_function(
                    var3, make_function(
                        var3, var3
                    )
                )
            );

    env["zero"] = make_function(ty_integer(), ty_boolean());
    env["pred"] = make_function(ty_integer(), ty_integer());
    env["times"] = make_function(
            ty_integer(), make_function(
                ty_integer(), ty_integer()
                )
            );

    auto example =
        letrec("factorial",
                lambda("n",
                    apply(
                        apply(
                            apply(identifier("cond"),
                                apply(identifier("zero"), identifier("n"))
                                ),
                            integer_literal(1)
                            ),
                        apply(
                            apply(identifier("times"), identifier("n")),
                            apply(identifier("factorial"),
                                apply(identifier("pred"), identifier("n"))
                                )
                            )
                        )
                    ),
                apply(identifier("factorial"), integer_literal(5))
              );
    examples.push_back(example);

    auto f = try_infer(env);
    for(auto& i : examples)
        f(i);

    return 0;
}
