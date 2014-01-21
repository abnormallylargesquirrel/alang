#ifndef HM_MAIN_H
#define HM_MAIN_H

#include "hm_inference.h"

class func_manager;

std::string str_of_type(const type& x);

inline std::ostream& operator<<(std::ostream& os, const type_variable& x)
{
    os << str_of_type(type(x));
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const type_operator& x)
{
    os << str_of_type(type(x));
    return os;
}

// TODO do real dependency analysis
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
