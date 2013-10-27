#ifndef FUNC_MANAGER_H
#define FUNC_MANAGER_H

#include <map>
#include <memory>
#include <llvm/IR/Function.h>
//#include "eval_t.h"
//#include "ast.h"

class ast;
class ast_cons;

class func_manager {
    typedef std::map<std::string, std::shared_ptr<ast_cons>> ast_cons_map;
public:
    func_manager()
    {
        //_lookup_type["int"] = eval_t::ev_int;
        //_lookup_type["float"] = eval_t::ev_float;
        //_lookup_type["void"] = eval_t::ev_void;
    }

    /*eval_t lookup_type(const std::string& str)
    {
        auto it = _lookup_type.find(str);
        if(it != _lookup_type.end())
            return it->second;

        return eval_t::ev_invalid;
    }*/

    /*eval_t lookup_func_type(const std::string& str)
    {
        auto it = _lookup_func_type.find(str);
        if(it != _lookup_func_type.end())
            return it->second;

        return eval_t::ev_invalid;
    }

    void set_func_type(const std::string& str, eval_t t)
    {
        _lookup_func_type[str] = t;
    }*/

    /*llvm::Function *lookup_func(const std::string& str)
    {
        auto it = _lookup_func.find(str);
        if(it != _lookup_func.end())
            return it->second;

        return nullptr;
    }

    void set_func(const std::string& str, llvm::Function *f)
    {
        _lookup_func[str] = f;
    }*/

    std::shared_ptr<ast_cons> lookup_ast_cons(const std::string& str)
    {
        auto it = _lookup_ast_cons.find(str);
        if(it != _lookup_ast_cons.end())
            return it->second;

        return nullptr;
    }

    void set_ast_cons(const std::string& str, const std::shared_ptr<ast>& f)
    {
        if(f->tag() == tags::tcons)
            _lookup_ast_cons[str] = std::static_pointer_cast<ast_cons>(f);
        else
            throw std::runtime_error("Tried to add invalid ast to func_manager");
    }

    /*std::string func_alias(const std::string& caller_name, const std::string& callee_name)
    {
        auto it = _func_alias.find(caller_name);
        if(it == _func_alias.end())
            return std::string();

        auto it2 = it->second.find(callee_name);
        if(it2 == it->second.end())
            return std::string();

        return it2->second;
    }

    void set_func_alias(const std::string& caller_name, const std::string& callee_name, const std::string& replacement)
    {
        (_func_alias[caller_name])[callee_name] = replacement;
    }*/

    ast_cons_map::iterator begin(void)
    {
        return _lookup_ast_cons.begin();
    }

    ast_cons_map::iterator end(void)
    {
        return _lookup_ast_cons.end();
    }

    std::string mangle_name(const ast& node);
private:
    //std::map<std::string, eval_t> _lookup_type;
    //std::map<std::string, eval_t> _lookup_func_type;
    ast_cons_map _lookup_ast_cons;
    //std::map<std::string, llvm::Function*> _lookup_func;
    //std::map<std::string, std::map<std::string, std::string>> _func_alias;
};

#endif
