#ifndef CODEGEN_H
#define CODEGEN_H

#include <map>
#include <cstdint>
#include <sstream>
#include <vector>
#include <set>
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
//#include "ast.h"
#include "token.h"

using namespace llvm;

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

std::nullptr_t error(const std::string& str);

class jit_engine {
public:
    jit_engine();

    //Value *gen_val(const std::shared_ptr<ast>& node);
    //Function *gen_func(const std::shared_ptr<ast>& node);

    bool run_pm() {return _pm.run(*_module);}

    void *getPointerToFunction(Function *f) {return _exec_engine->getPointerToFunction(f);}
    void freeMachineCodeForFunction(Function *f) {_exec_engine->freeMachineCodeForFunction(f);}
    void dump_module() const {_module->dump();}

    Value *visitor_gen_val(const expr_float *node);
    Value *visitor_gen_val(const expr_int *node);
    Value *visitor_gen_val(const expr_var *node);
    Value *visitor_gen_val(const expr_call *node);
    Value *visitor_gen_val(const expr_if *node);
    Value *visitor_gen_val(const binop_add *node);
    Value *visitor_gen_val(const binop_sub *node);
    Value *visitor_gen_val(const binop_mul *node);
    Value *visitor_gen_val(const binop_div *node);
    Value *visitor_gen_val(const binop_lt *node);
    Value *visitor_gen_val(const binop_gt *node);
    Value *visitor_gen_val(const binop_lte *node);
    Value *visitor_gen_val(const binop_gte *node);
    Value *visitor_gen_val(const binop_eq *node);
    Function *visitor_gen_func(const ast_proto *node);
    Function *visitor_gen_func(const proto_anon *node);
    Function *visitor_gen_func(const ast_func *node);
    Function *visitor_gen_func(const func_anon *node);

    jit_engine(const jit_engine& cp) = delete;
    const jit_engine& operator=(const jit_engine& rhs) = delete;
private:
    std::unique_ptr<Module> _module;
    IRBuilder<> _builder;
    PassManager _pm;
    FunctionPassManager _fpm;
    ExecutionEngine *_exec_engine;
    std::map<std::string, Value*> _named_values;
    Function *build_proto(const ast_proto *node, Function *f);
    Function *build_func(const ast_func *node);

    void init_alib();
    void init_fpm();

    std::vector<Value*> nodes_to_vals(const ast *node);

    str_to_num<std::int64_t> _str_to_i64;
    str_to_num<double> _str_to_double;
};

#endif
