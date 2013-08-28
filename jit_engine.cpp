#include "jit_engine.h"
#include "ast.h"
#include "alib.h"

void jit_engine::init_alib()
{
    auto ft = FunctionType::get(Type::getInt64Ty(getGlobalContext()), std::vector<Type*>(1, Type::getInt64Ty(getGlobalContext())), false);
    auto func = Function::Create(ft, Function::ExternalLinkage, "printI64", _module.get());
    _exec_engine->addGlobalMapping(func, (void*)&printI64);
}

void jit_engine::init_pm()
{
    _pm.add(createInstructionCombiningPass());
    _pm.add(createGlobalOptimizerPass());
    _pm.add(createFunctionInliningPass());
}

void jit_engine::init_fpm()
{
    //DataLayout dl = *_exec_engine->getDataLayout();
    //_fpm.add(&dl);
    _fpm.add(new DataLayout(*_exec_engine->getDataLayout()));
    _fpm.add(createBasicAliasAnalysisPass());
    _fpm.add(createInstructionCombiningPass());
    _fpm.add(createReassociatePass());
    _fpm.add(createGVNPass());
    _fpm.add(createCFGSimplificationPass());
    _fpm.doInitialization();
}

jit_engine::jit_engine()
    : _module(std::shared_ptr<Module>(new Module("alang module", getGlobalContext()))),
    _builder(IRBuilder<>(getGlobalContext())),
    _fpm(_module.get()),
    _exec_engine(EngineBuilder(_module.get()).create())
{
    //std::string errorstr;
    //_exec_engine = EngineBuilder(_module.get()).setErrorStr(&errorstr).create();
    //_fpm = FunctionPassManager(_module.get());
    if(!_exec_engine) {
        std::cout << "Failed to create ExecutionEngine" << std::endl;
    }

    init_pm();
    init_fpm();

    init_alib();
    //_module->setTargetTriple("i686-pc-linux-gnu");
}

std::vector<Value*> jit_engine::nodes_to_vals(const ast *node)
{
    std::vector<Value*> ret;
    bool failed = false;
    node->for_nodes([&](const std::shared_ptr<ast>& n) {
            ret.push_back(n->gen_val(this));
            if(!ret.back())
                failed = true;
            });

    if(failed)
        ret.clear();

    return ret;
}

Value *jit_engine::visitor_gen_val(const expr_float *node)
{
    return ConstantFP::get(getGlobalContext(), APFloat(_str_to_double(node->node_str())));
}

Value *jit_engine::visitor_gen_val(const expr_int *node)
{
    return ConstantInt::getSigned(Type::getInt64Ty(getGlobalContext()), _str_to_i64(node->node_str()));
}

Value *jit_engine::visitor_gen_val(const expr_var *node)
{
    Value *v = _named_values[node->node_str()];
    return v ? v : error("Unknown variable referenced");
}

Value *jit_engine::visitor_gen_val(const expr_call *node)
{
    Function *target_func = _module.get()->getFunction(node->node_str());
    if(!target_func)
        return error("Unknown function referenced");

    if(static_cast<int>(target_func->arg_size()) != node->num_nodes())
        return error("Incorrect number of function arguments");

    std::vector<Value*> vargs = nodes_to_vals(node);

    return _builder.CreateCall(target_func, vargs, "calltmp");
}

Value *jit_engine::visitor_gen_val(const expr_if *node)
{
    Value *condv = ((*node)[0])->gen_val(this);
    if(!condv)
        return nullptr;

    condv = _builder.CreateICmpNE(condv, ConstantInt::get(getGlobalContext(), APInt(64, 0, true)), "ifcond");

    Function *f = _builder.GetInsertBlock()->getParent(); //current block->current function

    BasicBlock *thenBB = BasicBlock::Create(getGlobalContext(), "then", f); //inserted into end of current function
    BasicBlock *elseBB = BasicBlock::Create(getGlobalContext(), "else");
    BasicBlock *mergeBB = BasicBlock::Create(getGlobalContext(), "ifcont");

    _builder.CreateCondBr(condv, thenBB, elseBB);
    _builder.SetInsertPoint(thenBB);

    Value *thenv = ((*node)[1])->gen_val(this);
    if(!thenv)
        return nullptr;

    _builder.CreateBr(mergeBB); //all basic blocks must be terminated (return/branch)
    thenBB = _builder.GetInsertBlock(); //in case of nested control

    f->getBasicBlockList().push_back(elseBB);
    _builder.SetInsertPoint(elseBB);

    Value *elsev = ((*node)[2])->gen_val(this);
    if(!elsev)
        return nullptr;

    _builder.CreateBr(mergeBB);
    elseBB = _builder.GetInsertBlock();

    f->getBasicBlockList().push_back(mergeBB);
    _builder.SetInsertPoint(mergeBB);

    PHINode *pn = _builder.CreatePHI(Type::getInt64Ty(getGlobalContext()), 2, "iftmp");
    pn->addIncoming(thenv, thenBB);
    pn->addIncoming(elsev, elseBB);
    return pn;
}

Value *jit_engine::visitor_gen_val(const binop_add *node)
{
    //std::cout << "visitor_gen_val binop_add" << std::endl;
    std::vector<Value*> v = nodes_to_vals(node);
    return _builder.CreateAdd(v[0], v[1], "addtmp");
}

Value *jit_engine::visitor_gen_val(const binop_sub *node)
{
    std::vector<Value*> v = nodes_to_vals(node);
    return _builder.CreateSub(v[0], v[1], "subtmp");
}

Value *jit_engine::visitor_gen_val(const binop_mul *node)
{
    std::vector<Value*> v = nodes_to_vals(node);
    return _builder.CreateMul(v[0], v[1], "multmp");
}

Value *jit_engine::visitor_gen_val(const binop_div *node)
{
    std::vector<Value*> v = nodes_to_vals(node);
    return _builder.CreateSDiv(v[0], v[1], "divtmp");
}

Value *jit_engine::visitor_gen_val(const binop_lt *node)
{
    std::vector<Value*> v = nodes_to_vals(node);
    v[0] = _builder.CreateICmpSLT(v[0], v[1], "lttmp");
    return _builder.CreateSExt(v[0], Type::getInt64Ty(getGlobalContext()));
}

Value *jit_engine::visitor_gen_val(const binop_gt *node)
{
    std::vector<Value*> v = nodes_to_vals(node);
    v[0] = _builder.CreateICmpSGT(v[0], v[1], "gttmp");
    return _builder.CreateSExt(v[0], Type::getInt64Ty(getGlobalContext()));
}

Value *jit_engine::visitor_gen_val(const binop_lte *node)
{
    std::vector<Value*> v = nodes_to_vals(node);
    v[0] = _builder.CreateICmpSLE(v[0], v[1], "ltetmp");
    return _builder.CreateSExt(v[0], Type::getInt64Ty(getGlobalContext()));
}

Value *jit_engine::visitor_gen_val(const binop_gte *node)
{
    std::vector<Value*> v = nodes_to_vals(node);
    v[0] = _builder.CreateICmpSGE(v[0], v[1], "gtetmp");
    return _builder.CreateSExt(v[0], Type::getInt64Ty(getGlobalContext()));
}

Value *jit_engine::visitor_gen_val(const binop_eq *node)
{
    std::vector<Value*> v = nodes_to_vals(node);
    v[0] = _builder.CreateICmpEQ(v[0], v[1], "eqtmp");
    return _builder.CreateSExt(v[0], Type::getInt64Ty(getGlobalContext()));
}

Function *jit_engine::visitor_gen_func(const ast_proto *node)
{
    std::vector<Type*> params(static_cast<unsigned int>(node->num_nodes()), Type::getInt64Ty(getGlobalContext()));
    FunctionType *ft = FunctionType::get(Type::getInt64Ty(getGlobalContext()), params, false);
    Function *f = Function::Create(ft, Function::ExternalLinkage, node->node_str(), _module.get());

    if(f->getName() != node->node_str()) { //if function with name already existed, it's implicitly renamed
        f->eraseFromParent();
        f = _module.get()->getFunction(node->node_str());

        if(!f->empty())
            return error("Function already defined");
        if(static_cast<int>(f->arg_size()) != node->num_nodes()) //define or decl allowed after decl, same # args
            return error("Redefinition of a function with different number of arguments");
    }

    unsigned int idx = 0;
    std::set<std::string> check_unique;
    for(Function::arg_iterator ai = f->arg_begin(); static_cast<int>(idx) != node->num_nodes(); ai++, idx++) {
        std::string arg_name = (*node)[idx]->node_str();
        if(check_unique.find(arg_name) == check_unique.end())
            check_unique.insert(arg_name);
        else
            return error("Redeclaration of function parameter(s)");

        ai->setName(arg_name);
        _named_values[(*node)[idx]->node_str()] = ai;
    }
    return f;
}

Function *jit_engine::visitor_gen_func(const ast_func *node)
{
    _named_values.clear();
    Function *func = ((*node)[0])->gen_func(this); //proto

    if(!func)
        return error("No function prototype");

    //if(func->empty())
        //return error("Def with no corresponding body");

    BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "entry", func);
    _builder.SetInsertPoint(bb);

    //std::cout << ((*node)[1])->to_str() << std::endl;
    if(Value *ret = ((*node)[1])->gen_val(this)) { //body
        _builder.CreateRet(ret);
        verifyFunction(*func);
        _fpm.run(*func);
        return func;
    }

    func->eraseFromParent();
    return error("Could not generate code for function body");
}
