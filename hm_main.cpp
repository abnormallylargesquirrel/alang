#include <utility>
#include "hm_main.h"
#include "func_manager.h"

std::string str_of_type(const type& x)
{
    std::ostringstream os;
    char next_name = 'a';
    bool print_parens = false;
    // names holds (name of tyvar, string describing type class restrictions)
    std::map<std::size_t, std::pair<std::string, std::string>> names;
    std::function<void(const type&)> of_type;

    auto of_tyvar = [&](const type_variable& _x) {
//        os << '{';
//
//        if(!names.count(_x.id())) {
//            // oss builds description of type class restrictions
//            std::ostringstream oss;
//            for(const auto& i : _x) {
//                oss << i << " ";
//            }
//            names[_x.id()] = std::to_string(next_name++);
//        }

//        os << names[_x.id()] << '}';
        os << std::get<0>(names[_x.id()]);
    };

    auto of_tyop = [&](const type_operator& _x) {
        switch(_x.kind()) {
            case types::Void:
                os << "Void";
                break;
            case types::Int:
                os << "Int";
                break;
            case types::Bool:
                os << "Bool";
                break;
            case types::Float:
                os << "Float";
                break;
            case types::Str:
                os << "String";
                break;
            case types::Function:
                {
                if(print_parens)
                    os << "(";

                bool tmp = print_parens;

                print_parens = true;
                of_type(_x[0]);
                os << " -> ";
                print_parens = false;
                of_type(_x[1]);

                if(tmp)
                    os << ")";
                break;
                }
            case types::Pair:
                os << "(";
                of_type(_x[0]);
                os << ", ";
                of_type(_x[1]);
                os << ")";
                break;
            case types::List:
                os << "[";
                of_type(_x[0]);
                os << "]";
                break;
            default:
                break;
        }
    };

    auto output_names = [&]() {
        bool print_forall = false;
        for(const auto& i : names) {
            auto map_val = std::get<1>(i);
            auto restrictions_desc = std::get<1>(map_val);
            if(restrictions_desc.size() > 0) {
                print_forall = true;
                os << "(" << restrictions_desc << std::get<0>(map_val) << ")";
            }
        }
        if(print_forall)
            os << " => ";
    };

    of_type = [&](const type& _x) {
        if(_x.which()) {
            const auto& op = boost::get<type_operator>(_x);
            of_tyop(op);
        } else {
            const auto& var = boost::get<type_variable>(_x);
            of_tyvar(var);
        }
    };

    if(x.which()) {
        const auto& op = boost::get<type_operator>(x);
        op.collect_tyvar_names(next_name, names);
        output_names();
    } else {
        const auto& var = boost::get<type_variable>(x);
        var.collect_tyvar_names(next_name, names);
        output_names();
    }

    of_type(x);
    return os.str();
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

    type_variable v9(_ctxs, _env.unique_id(), {});
    type_variable v10(_ctxs, _env.unique_id(), {});
    _env["pair"] = make_function(v9, make_function(v10, ty_pair(v9, v10)));

    type_variable v11(_ctxs, _env.unique_id(), {});
    _env["cons"] = make_function(v11, make_function(ty_list(v11), ty_list(v11)));

    type_variable v12(_ctxs, _env.unique_id(), {});
    _env["list"] = ty_list(v12);

    type_variable v13(_ctxs, _env.unique_id(), {});
    _env["head"] = make_function(ty_list(v13), v13);

    type_variable v14(_ctxs, _env.unique_id(), {});
    _env["tail"] = make_function(ty_list(v14), ty_list(v14));

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
        std::cerr << e.what() << ": " << str_of_type(e.x) << " in " << str_of_type(e.y) << std::endl;
        throw std::runtime_error("Type error");
    }
    catch(const type_mismatch& e)
    {
        std::cerr << n << " : ";
        std::cerr << e.what() << ": " << str_of_type(e.x) << " != " << str_of_type(e.y) << std::endl;
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
