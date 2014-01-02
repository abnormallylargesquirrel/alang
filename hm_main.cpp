#include "hm_main.h"
#include "func_manager.h"

type_printer::type_printer(std::ostream& os)
    : _os(os), _next_name('a'), _print_parens(false) {}

type_printer& type_printer::operator()(const type_variable& x)
{
    _os << '{';
    for(const auto& i : x) {
        _os << i << " ";
    }

    if(!_names.count(x.id())) {
        std::ostringstream os;
        os << _next_name++;
        _names[x.id()] = os.str();
    }

    _os << _names[x.id()] << '}';
    return *this;
}

type_printer& type_printer::operator()(const type_operator& x)
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
        case types::Str:
            _os << "String";
            break;
        case types::Function:
            {
            if(_print_parens)
                _os << "(";

            bool tmp = _print_parens;

            _print_parens = true;
            *this << x[0];
            _os << " -> ";
            _print_parens = false;
            *this << x[1];

            if(tmp)
                _os << ")";
            break;
            }
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

type_printer& type_printer::operator<<(const type& x)
{
    return boost::apply_visitor(*this, x);
}

type_printer& type_printer::operator<<(std::ostream& (*fp)(std::ostream&))
{
    fp(_os);
    return *this;
}

try_infer::try_infer(contexts& ctxs, environment& e,
        std::map<std::string, std::set<std::string>>& dependencies,
        func_manager& fm, const class_env& ce)
    : _ctxs(ctxs), _env(e), _dependencies(dependencies), _fm(fm), _ce(ce)
{
    type_variable v0(_ctxs, _env.unique_id(), {"Num"});
    _env["+"] = make_function(v0, make_function(v0, v0));

    type_variable v1(_ctxs, _env.unique_id(), {"Num"});
    _env["-"] = make_function(v1, make_function(v1, v1));

    type_variable v2(_ctxs, _env.unique_id(), {"Num"});
    _env["*"] = make_function(v2, make_function(v2, v2));

    type_variable v3(_ctxs, _env.unique_id(), {"Num"});
    _env["/"] = make_function(v3, make_function(v3, v3));

    type_variable v4(_ctxs, _env.unique_id(), {"Ord"});
    _env["<"] = make_function(v4, make_function(v4, ty_bool()));

    type_variable v5(_ctxs, _env.unique_id(), {"Ord"});
    _env[">"] = make_function(v5, make_function(v5, ty_bool()));

    type_variable v6(_ctxs, _env.unique_id(), {"Ord", "Eq"});
    _env["<="] = make_function(v6, make_function(v6, ty_bool()));

    type_variable v7(_ctxs, _env.unique_id(), {"Ord", "Eq"});
    _env[">="] = make_function(v7, make_function(v7, ty_bool()));

    type_variable v8(_ctxs, _env.unique_id(), {"Eq"});
    _env["="] = make_function(v8, make_function(v8, ty_bool()));

    _env["^"] = make_function(ty_str(), make_function(ty_str(), ty_str()));
}

void try_infer::operator()(const shared_ast& n) const
{
    try
    {
        auto result = infer_type(n, _ctxs, _env, _dependencies, _fm);
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

void try_infer::check_insts(void) // TODO do efficiently
{
    for(const auto& i : _ctxs) { // i = key/value: size_t/pair<set<string>,set<size_t>> = for each type_variable
        for(const auto& j : i.second.second) { // j = size_t = for each instantiation
            for(const auto& k : i.second.first) { // k = string = for each class
                auto it = _ce.find(k);
                if(it != _ce.end()) {
                    auto tc = it->second;
                    if(!tc.has_type_inst(j)) {
                        std::ostringstream oss;
                        oss << "Failed instantiation: Type " << j << " is not an instance of class " << k;
                        //std::cout << "Type " << j << " is not an instance of class " << k << std::endl;
                        throw std::runtime_error(oss.str());
                    }
                }
            }
        }
    }
}

void try_infer::propagate_contexts(void)
{
    for(const auto& i : _fm) {
        auto& t = _env[i.first];
        if(t.which()) {
            auto& op = boost::get<type_operator>(t);
            op.propagate(_ctxs);
        } else {
            auto& tv = boost::get<type_variable>(t);
            tv.propagate(_ctxs);
        }
    }
}
