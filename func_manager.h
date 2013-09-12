#ifndef FUNC_MANAGER_H
#define FUNC_MANAGER_H

#include <map>
#include <memory>
#include "eval_t.h"
//#include "ast.h"

class ast;
class func_template;

class func_manager {
public:
    func_manager()
    {
        _lookup_type["i64"] = eval_t::ev_int64;
        _lookup_type["dbl"] = eval_t::ev_float;
        _lookup_type["void"] = eval_t::ev_void;
    }

    eval_t lookup_type(const std::string& str)
    {
        if(_lookup_type.find(str) == _lookup_type.end())
            return eval_t::ev_invalid;

        return _lookup_type[str];
    }

    eval_t lookup_var(const std::string& str)
    {
        if(_lookup_var.find(str) == _lookup_var.end())
            return eval_t::ev_invalid;

        return _lookup_var[str];
    }

    eval_t lookup_func_type(const std::string& str)
    {
        if(_lookup_func_type.find(str) == _lookup_func_type.end())
            return eval_t::ev_invalid;

        return _lookup_func_type[str];
    }

    std::shared_ptr<func_template> lookup_template(const std::string& str)
    {
        if(_lookup_template.find(str) == _lookup_template.end())
            return nullptr;

        return _lookup_template[str];
    }

    void set_var(const std::string& str, eval_t t)
    {
        _lookup_var[str] = t;
    }

    void set_func_type(const std::string& str, eval_t t)
    {
        _lookup_func_type[str] = t;
    }

    void set_template(const std::string& str, const std::shared_ptr<func_template> f)
    {
        _lookup_template[str] = f;
    }

    void clear_vars(void)
    {
        _lookup_var.clear();
    }

    eval_t resolve_types(const std::shared_ptr<ast>& node);
    std::string mangle_name(const ast *node);
    std::string mangle_name(const std::shared_ptr<ast>& node);

private:
    std::map<std::string, eval_t> _lookup_type;
    std::map<std::string, eval_t> _lookup_func_type;
    std::map<std::string, eval_t> _lookup_var;
    std::map<std::string, std::shared_ptr<func_template>> _lookup_template;
};

#endif
