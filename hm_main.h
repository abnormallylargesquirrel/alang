#ifndef HM_MAIN_H
#define HM_MAIN_H

#include "hm_inference.h"

class func_manager;

class type_printer : public boost::static_visitor<type_printer&> {
public:
    type_printer(std::ostream& os);
    type_printer& operator()(const type_variable& x);
    type_printer& operator()(const type_operator& x);

    type_printer& operator<<(const type& x);
    type_printer& operator<<(std::ostream& (*fp)(std::ostream&));

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
    bool _print_parens;
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
    try_infer(contexts& ctxs, environment& e,
            std::map<std::string, std::set<std::string>>& dependencies,
            func_manager& fm, const class_env& ce);

    void operator()(const shared_ast& n) const;
    void check_insts(void);
    void propagate_contexts(void);

    contexts& _ctxs;
    environment& _env;
    std::map<std::string, std::set<std::string>>& _dependencies;
    func_manager& _fm;
    class_env _ce;
};


#endif
