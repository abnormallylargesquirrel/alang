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
#include "ast.h"
#include "token.h"

using namespace llvm;

std::nullptr_t error(const std::string& str);

class jit_engine {
public:
    //jit_engine() : _module(new Module("alang module", getGlobalContext())), _builder(IRBuilder<>(getGlobalContext())) {}
    jit_engine();

    Value *gen_val(const std::shared_ptr<ast>& node);
    Function *gen_func(const std::shared_ptr<ast>& node);

    bool run_pm() {return _pm.run(*_module);}

    void *getPointerToFunction(Function *f) {return _exec_engine->getPointerToFunction(f);}
    void freeMachineCodeForFunction(Function *f) {_exec_engine->freeMachineCodeForFunction(f);}
    void dump_module() const {_module->dump();}

    jit_engine(const jit_engine& cp) = delete;
    const jit_engine& operator=(const jit_engine& rhs) = delete;
private:
    std::shared_ptr<Module> _module;
    IRBuilder<> _builder;
    PassManager _pm;
    FunctionPassManager _fpm;
    ExecutionEngine *_exec_engine;
    std::map<std::string, Value*> _named_values;

    void init_alib();
    void init_pm();
    void init_fpm();

    str_to_num<std::int64_t> _str_to_i64;
};

#endif
