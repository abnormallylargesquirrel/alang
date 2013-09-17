#ifndef CODEGEN_H
#define CODEGEN_H

#include <map>
#include <stack>
#include <cstdint>
#include <sstream>
#include <vector>
#include <set>
#include <unordered_set>
#include <memory>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/PassManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include "utils.h"
#include "token.h"
#include "eval_t.h"
#include "func_manager.h"

class ast;
class expr_float;
class expr_int;
class expr_var;
class expr_call;
class expr_if;
class binop_add;
class binop_sub;
class binop_mul;
class binop_div;
class binop_lt;
class binop_gt;
class binop_lte;
class binop_gte;
class binop_eq;
class ast_proto;
class proto_anon;
class ast_func;
class func_anon;
class func_template;

std::nullptr_t error(const std::string& str);

typedef std::map<std::string, llvm::Value*> scope;
/*struct scope {
    std::map<std::string, llvm::Value*> local_values;
    std::map<std::string, std::string> local_funcs;
};*/

class jit_engine {
public:
    jit_engine(const std::shared_ptr<func_manager>& fm);

    bool run_pm() {return _pm.run(*_module);}

    void *getPointerToFunction(llvm::Function *f) {return _exec_engine->getPointerToFunction(f);}
    void freeMachineCodeForFunction(llvm::Function *f) {_exec_engine->freeMachineCodeForFunction(f);}
    void dump_module() const {_module->dump();}

    llvm::Value *visitor_gen_val(const expr_float& node);
    llvm::Value *visitor_gen_val(const expr_int& node);
    llvm::Value *visitor_gen_val(const expr_var& node);
    llvm::Value *visitor_gen_val(expr_call& node);
    llvm::Value *visitor_gen_val(const expr_if& node);
    llvm::Value *visitor_gen_val(const binop_add& node);
    llvm::Value *visitor_gen_val(const binop_sub& node);
    llvm::Value *visitor_gen_val(const binop_mul& node);
    llvm::Value *visitor_gen_val(const binop_div& node);
    llvm::Value *visitor_gen_val(const binop_lt& node);
    llvm::Value *visitor_gen_val(const binop_gt& node);
    llvm::Value *visitor_gen_val(const binop_lte& node);
    llvm::Value *visitor_gen_val(const binop_gte& node);
    llvm::Value *visitor_gen_val(const binop_eq& node);
    llvm::Function *visitor_gen_func(const ast_proto& node);
    llvm::Function *visitor_gen_func(const proto_anon& node);
    llvm::Function *visitor_gen_func(const ast_func& node);
    llvm::Function *visitor_gen_func(const func_anon& node);
    //llvm::Function *visitor_gen_func(func_template& node);

    eval_t resolve_types(const expr_if& node);
    eval_t resolve_types(expr_call& node);
    eval_t resolve_types(const ast& node);

    //std::map<std::string, llvm::Value*>& local_values(void) {return _scopes.top().local_values;}
    //std::map<std::string, std::string>& local_funcs(void) {return _scopes.top().local_funcs;}
    scope& local_values(void) {return _scopes.top();}

    void push_scope(void)
    {
        _scopes.push(scope());
    }
    void pop_scope(void)
    {
        _scopes.pop();
    }

    void set_var(const std::string& str, eval_t t)
    {
        _lookup_var[str] = t;
    }

    void clear_vars(void)
    {
        _lookup_var.clear();
    }

    jit_engine(const jit_engine& cp) = delete;
    const jit_engine& operator=(const jit_engine& rhs) = delete;
private:
    std::unique_ptr<llvm::Module> _module;
    llvm::IRBuilder<> _builder;
    llvm::PassManager _pm;
    llvm::FunctionPassManager _fpm;
    llvm::ExecutionEngine *_exec_engine;

    std::shared_ptr<func_manager> _fm;

    //std::map<std::string, llvm::Value*> _named_values;
    std::map<eval_t, llvm::Type*> _lookup_llvm_type;
    std::map<std::string, eval_t> _lookup_var;
    std::stack<scope> _scopes;

    eval_t lookup_var(const std::string& str);
    llvm::Type* lookup_llvm_type(eval_t t);
    eval_t lookup_eval_type(llvm::Type *t);

    llvm::Function *build_proto(const ast_proto& node, llvm::Function *f);
    llvm::Function *build_func(const ast_func& node);
    void cast_int64(llvm::Value*& v);
    void cast_float(llvm::Value*& v);

    void init_alib();
    void init_fpm();

    std::vector<llvm::Value*> nodes_to_vals(const ast& node, bool do_cast = false);

    str_to_num<std::int64_t> _str_to_i64;
    str_to_num<double> _str_to_double;
};

#endif
