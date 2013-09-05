#include "jit_engine.h"
#include "ast.h"
#include "alib.h"

void jit_engine::init_alib()
{
    auto ftype = FunctionType::get(Type::getVoidTy(getGlobalContext()), std::vector<Type*>(1, Type::getInt64Ty(getGlobalContext())), false);
    auto func = Function::Create(ftype, Function::ExternalLinkage, "printI64", _module.get());
    _exec_engine->addGlobalMapping(func, (void*)&printI64);

    ftype = FunctionType::get(Type::getVoidTy(getGlobalContext()), std::vector<Type*>(1, Type::getDoubleTy(getGlobalContext())), false);
    func = Function::Create(ftype, Function::ExternalLinkage, "printDbl", _module.get());
    _exec_engine->addGlobalMapping(func, (void*)&printDbl);
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
    : _module(std::unique_ptr<Module>(new Module("alang module", getGlobalContext()))),
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

    _lookup_llvm_type[static_cast<int>(eval_t::ev_int64)] = Type::getInt64Ty(getGlobalContext());
    _lookup_llvm_type[static_cast<int>(eval_t::ev_float)] = Type::getDoubleTy(getGlobalContext());
    _lookup_llvm_type[static_cast<int>(eval_t::ev_void)] = Type::getVoidTy(getGlobalContext());

    init_fpm();

    init_alib();
    _module->setTargetTriple("i686-pc-linux-gnu");
}

eval_t jit_engine::lookup_var(const std::string& str)
{
    if(_lookup_var.find(str) == _lookup_var.end())
        return eval_t::ev_invalid;

    return _lookup_var[str];
}

Type* jit_engine::lookup_type(int n)
{
    if(_lookup_llvm_type.find(n) == _lookup_llvm_type.end())
        return nullptr;

    return _lookup_llvm_type[n];
}

/*std::shared_ptr<func_template> jit_engine::lookup_template(const std::string& str)
{
    if(_templates.find(str) == _templates.end())
        return nullptr;

    return _templates[str];
}*/

void jit_engine::cast_int64(Value*& v)
{
    if(v->getType()->getTypeID() == Type::TypeID::DoubleTyID)
        v = _builder.CreateFPToSI(v, Type::getInt64Ty(getGlobalContext()));
}

void jit_engine::cast_float(Value*& v)
{
    if(v->getType()->getTypeID() == Type::TypeID::IntegerTyID)
        v = _builder.CreateSIToFP(v, Type::getDoubleTy(getGlobalContext()));
}

std::vector<Value*> jit_engine::nodes_to_vals(const ast *node, bool do_cast)
{
    std::vector<Value*> ret;
    bool failed = false;
    node->for_nodes([&](const std::shared_ptr<ast>& n) {
            Value* tmp = n->gen_val(this);

            if(do_cast) {
                switch(node->eval_type()) {
                case eval_t::ev_int64:
                    cast_int64(tmp);
                    break;
                case eval_t::ev_float:
                    cast_float(tmp);
                    break;
                /*case eval_t::ev_template:
                    std::cout << "ev_template in nodes_to_vals" << std::endl;*/
                case eval_t::ev_invalid:
                case eval_t::ev_void:
                    failed = true;
                    break;
                }
            }

            ret.push_back(tmp);
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

void print_nodes(const std::shared_ptr<ast>& node)
{
    node->for_nodes([&](const std::shared_ptr<ast>& n) {
            print_nodes(n);
            std::cout << n->to_str() << std::endl;
            });
}

eval_t jit_engine::resolve_types(const std::shared_ptr<ast>& node)
{
    if(node->num_nodes() == 0) {
        if(lookup_var(node->node_str()) != eval_t::ev_invalid) {
            return _lookup_var[node->node_str()];
        }
        if(node->eval_type() != eval_t::ev_invalid/* && node->eval_type() != eval_t::ev_template*/) {
            return node->eval_type();
        } else {
            return eval_t::ev_invalid;
        }
    }

    std::vector<eval_t> nodes_types;
    node->for_nodes([&](const std::shared_ptr<ast>& n) {
            eval_t t = resolve_types(n);
            n->set_eval_type(t);
            if(t != eval_t::ev_invalid/* && t != eval_t::ev_template*/) {
                nodes_types.push_back(t);
            } else {
                error("resolve_types failed");
            }
            });

    return *std::max_element(std::begin(nodes_types), std::end(nodes_types));
}

Value *jit_engine::visitor_gen_val(const expr_call *node)
{
    Function *target_func = _module->getFunction(node->node_str());
    if(!target_func) {
        /*std::shared_ptr<func_template> ft = lookup_template(node->node_str());
        if(!ft) {
            return error("Unknown function referenced");
        }

        std::shared_ptr<ast> proto_tmp = ((*ft)[0]);
        for(int i = 0; i < proto_tmp->num_nodes(); i++) {
            _lookup_var[((*proto_tmp)[static_cast<unsigned int>(i)])->node_str()] =((*node)[static_cast<unsigned int>(i)])->eval_type();
            ((*proto_tmp)[static_cast<unsigned int>(i)])->set_eval_type(((*node)[static_cast<unsigned int>(i)])->eval_type());
        }

        eval_t expr_type = resolve_types((*ft)[1]);
        ft->for_nodes([&](const std::shared_ptr<ast>& n) {n->set_eval_type(expr_type);});

        //std::cout << "ft" << std::endl;
        //print_nodes(ft);

        BasicBlock *bb = _builder.GetInsertBlock();
        target_func = visitor_gen_func(static_cast<ast_func*>(ft.get()));
        _builder.SetInsertPoint(bb);*/
        return error("Unknown function referenced");
    }

    if(static_cast<int>(target_func->arg_size()) != node->num_nodes())
        return error("Incorrect number of function arguments");

    std::vector<Value*> vargs = nodes_to_vals(node);

    if(target_func->getReturnType() != Type::getVoidTy(getGlobalContext()))
        return _builder.CreateCall(target_func, vargs, "calltmp");

    return _builder.CreateCall(target_func, vargs);
}

Value *jit_engine::visitor_gen_val(const expr_if *node)
{
    Value *condv = ((*node)[0])->gen_val(this);
    if(!condv)
        return error("Could not generate condition in if");

    if(node->eval_type() == eval_t::ev_int64)
        condv = _builder.CreateICmpNE(condv, ConstantInt::get(getGlobalContext(),
                    APInt(condv->getType()->getIntegerBitWidth(), 0, true)), "ifcond");
    else if(node->eval_type() == eval_t::ev_float)
        condv = _builder.CreateFCmpONE(condv, ConstantFP::get(getGlobalContext(), APFloat(0.0)), "ifcond");
    else
        return error("Invalid eval type in if");

    Function *f = _builder.GetInsertBlock()->getParent(); //current block->current function

    BasicBlock *thenBB = BasicBlock::Create(getGlobalContext(), "then", f); //inserted into end of current function
    BasicBlock *elseBB = BasicBlock::Create(getGlobalContext(), "else");
    BasicBlock *mergeBB = BasicBlock::Create(getGlobalContext(), "ifcont");

    _builder.CreateCondBr(condv, thenBB, elseBB);
    _builder.SetInsertPoint(thenBB);

    Value *thenv = ((*node)[1])->gen_val(this);
    if(!thenv)
        return error("Could not generate then in if");

    _builder.CreateBr(mergeBB); //all basic blocks must be terminated (return/branch)
    thenBB = _builder.GetInsertBlock(); //in case of nested control structures in then block

    f->getBasicBlockList().push_back(elseBB);
    _builder.SetInsertPoint(elseBB);

    Value *elsev = ((*node)[2])->gen_val(this);
    if(!elsev)
        return error("Could not generate else in if");

    _builder.CreateBr(mergeBB);
    elseBB = _builder.GetInsertBlock();

    f->getBasicBlockList().push_back(mergeBB);
    _builder.SetInsertPoint(mergeBB);

    PHINode *pn;
    if(node->eval_type() == eval_t::ev_int64)
        pn = _builder.CreatePHI(Type::getInt64Ty(getGlobalContext()), 2, "iftmp");
    else if(node->eval_type() == eval_t::ev_float)
        pn = _builder.CreatePHI(Type::getDoubleTy(getGlobalContext()), 2, "iftmp");
    else
        return error("Invalid eval type in if");

    pn->addIncoming(thenv, thenBB);
    pn->addIncoming(elsev, elseBB);
    return pn;
}

Value *jit_engine::visitor_gen_val(const binop_add *node)
{
    std::vector<Value*> v = nodes_to_vals(node, true);
    if(node->eval_type() == eval_t::ev_int64)
        return _builder.CreateAdd(v[0], v[1], "addtmp");
    if(node->eval_type() == eval_t::ev_float)
        return _builder.CreateFAdd(v[0], v[1], "addtmp");

    return error("Invalid eval type in binop_add");
}

Value *jit_engine::visitor_gen_val(const binop_sub *node)
{
    std::vector<Value*> v = nodes_to_vals(node, true);
    if(node->eval_type() == eval_t::ev_int64)
        return _builder.CreateSub(v[0], v[1], "subtmp");
    if(node->eval_type() == eval_t::ev_float)
        return _builder.CreateFSub(v[0], v[1], "subtmp");

    return error("Invalid eval type in binop_sub");
}

Value *jit_engine::visitor_gen_val(const binop_mul *node)
{
    std::vector<Value*> v = nodes_to_vals(node, true);
    if(node->eval_type() == eval_t::ev_int64)
        return _builder.CreateMul(v[0], v[1], "multmp");
    if(node->eval_type() == eval_t::ev_float)
        return _builder.CreateFMul(v[0], v[1], "multmp");

    return error("Invalid eval type in binop_mul");
}

Value *jit_engine::visitor_gen_val(const binop_div *node)
{
    std::vector<Value*> v = nodes_to_vals(node, true);
    if(node->eval_type() == eval_t::ev_int64)
        return _builder.CreateSDiv(v[0], v[1], "divtmp");
    if(node->eval_type() == eval_t::ev_float)
        return _builder.CreateFDiv(v[0], v[1], "divtmp");

    return error("Invalid eval type in binop_div");
}

Value *jit_engine::visitor_gen_val(const binop_lt *node)
{
    std::vector<Value*> v = nodes_to_vals(node, true);
    if(node->eval_type() == eval_t::ev_int64) {
        v[0] = _builder.CreateICmpSLT(v[0], v[1], "lttmp");
        return _builder.CreateSExt(v[0], Type::getInt64Ty(getGlobalContext()));
    }
    if(node->eval_type() == eval_t::ev_float) {
        v[0] = _builder.CreateFCmpULT(v[0], v[1], "lttmp");
        return _builder.CreateUIToFP(v[0], Type::getDoubleTy(getGlobalContext()));
    }

    return error("Invalid eval type in binop_lt");
}

Value *jit_engine::visitor_gen_val(const binop_gt *node)
{
    std::vector<Value*> v = nodes_to_vals(node, true);
    if(node->eval_type() == eval_t::ev_int64) {
        v[0] = _builder.CreateICmpSGT(v[0], v[1], "gttmp");
        return _builder.CreateSExt(v[0], Type::getInt64Ty(getGlobalContext()));
    }
    if(node->eval_type() == eval_t::ev_float) {
        v[0] = _builder.CreateFCmpUGT(v[0], v[1], "gttmp");
        return _builder.CreateUIToFP(v[0], Type::getDoubleTy(getGlobalContext()));
    }

    return error("Invalid eval type in binop_gt");
}

Value *jit_engine::visitor_gen_val(const binop_lte *node)
{
    std::vector<Value*> v = nodes_to_vals(node, true);
    if(node->eval_type() == eval_t::ev_int64) {
        v[0] = _builder.CreateICmpSLE(v[0], v[1], "ltetmp");
        return _builder.CreateSExt(v[0], Type::getInt64Ty(getGlobalContext()));
    }
    if(node->eval_type() == eval_t::ev_float) {
        v[0] = _builder.CreateFCmpULE(v[0], v[1], "ltetmp");
        return _builder.CreateUIToFP(v[0], Type::getDoubleTy(getGlobalContext()));
    }

    return error("Invalid eval type in binop_lte");
}

Value *jit_engine::visitor_gen_val(const binop_gte *node)
{
    std::vector<Value*> v = nodes_to_vals(node, true);
    if(node->eval_type() == eval_t::ev_int64) {
        v[0] = _builder.CreateICmpSGE(v[0], v[1], "gtetmp");
        return _builder.CreateSExt(v[0], Type::getInt64Ty(getGlobalContext()));
    }
    if(node->eval_type() == eval_t::ev_float) {
        v[0] = _builder.CreateFCmpUGE(v[0], v[1], "gtetmp");
        return _builder.CreateUIToFP(v[0], Type::getDoubleTy(getGlobalContext()));
    }

    return error("Invalid eval type in binop_gte");
}

Value *jit_engine::visitor_gen_val(const binop_eq *node)
{
    std::vector<Value*> v = nodes_to_vals(node, true);
    if(node->eval_type() == eval_t::ev_int64) {
        v[0] = _builder.CreateICmpEQ(v[0], v[1], "eqtmp");
        return _builder.CreateSExt(v[0], Type::getInt64Ty(getGlobalContext()));
    }
    if(node->eval_type() == eval_t::ev_float) {
        v[0] = _builder.CreateFCmpUEQ(v[0], v[1], "eqtmp");
        return _builder.CreateUIToFP(v[0], Type::getDoubleTy(getGlobalContext()));
    }

    return error("Invalid eval type in binop_eq");
}

Function *jit_engine::build_proto(const ast_proto *node, Function *f)
{
    if(f->getName() != node->node_str()) { //if function with name already existed, it's implicitly renamed
        f->eraseFromParent();
        f = _module->getFunction(node->node_str());

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

Function *jit_engine::visitor_gen_func(const ast_proto *node)
{
    std::vector<Type*> params;//(static_cast<unsigned int>(node->num_nodes()), Type::getInt64Ty(getGlobalContext()));
    node->for_nodes([&](const std::shared_ptr<ast>& n){
            //std::cout << static_cast<int>(n->eval_type()) << std::endl;
            Type *param_type = lookup_type(static_cast<int>(n->eval_type()));
            if(!param_type)
                 error("Invaild parameter eval type in proto");

            params.push_back(lookup_type(static_cast<int>(n->eval_type())));
            });

    /*if(node->eval_type() == eval_t::ev_template)
        return error("Template eval type in prototype");*/

    Type *t = lookup_type(static_cast<int>(node->eval_type()));
    if(!t)
        return error("Invalid eval type in prototype");

    FunctionType *ftype = FunctionType::get(t, params, false);
    Function *f = Function::Create(ftype, Function::ExternalLinkage, node->node_str(), _module.get());

    return build_proto(node, f);
}

Function *jit_engine::visitor_gen_func(const proto_anon *node)
{
    FunctionType *ftype = FunctionType::get(Type::getVoidTy(getGlobalContext()), std::vector<Type*>(), false);
    Function *f = Function::Create(ftype, Function::ExternalLinkage, node->node_str(), _module.get());

    return build_proto(node, f);
}

/*Function *jit_engine::visitor_gen_func(const proto_template *node)
{
    if(lookup_template(node->node_str()) != nullptr)
        return error("Template function already declared");

    _templates[node->node_str()] = std::make_shared<proto_template>(*node);
    std::cout << "visitor_gen_func(proto_template*) called" << std::endl;

    return nullptr;
}*/

Function *jit_engine::build_func(const ast_func *node)
{
    _named_values.clear();
    Function *func = ((*node)[0])->gen_func(this); //proto

    if(!func)
        return error("Could not generate function prototype");

    BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "entry", func);
    //_func_blocks.push(bb);
    _builder.SetInsertPoint(bb);

    return func;
}

Function *jit_engine::visitor_gen_func(const ast_func *node)
{
    auto func = build_func(node);

    if(Value *ret = ((*node)[1])->gen_val(this)) { //body
        _builder.CreateRet(ret);
        verifyFunction(*func);
        _fpm.run(*func);
        return func;
    }

    func->eraseFromParent();
    return error("Could not generate code for function body");
}

Function *jit_engine::visitor_gen_func(const func_anon *node)
{
    auto func = build_func(node);

    if(((*node)[1])->gen_val(this)) { //body
        _builder.CreateRetVoid();
        verifyFunction(*func);
        _fpm.run(*func);
        return func;
    }

    func->eraseFromParent();
    return error("Could not generate code for function body");
}

/*Function *jit_engine::visitor_gen_func(const func_template *node)
{
    if(lookup_template(((*node)[0])->node_str()) != nullptr)
        return error("Template function already declared");

    _templates[((*node)[0])->node_str()] = std::make_shared<func_template>(*node);
    return nullptr;
}*/
