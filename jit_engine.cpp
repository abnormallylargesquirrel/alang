#include "jit_engine.h"
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

Value *jit_engine::gen_val(const std::shared_ptr<ast>& node)
{
    switch(node->node_type()) {
    case tok::literal_float:
        std::cout << "Generated value for float" << std::endl;
    case tok::literal_int:
        {
        std::int64_t val;
        if(node->node_literal_signed())
            val = 0 - _str_to_i64(node->node_str());
        else
            val = _str_to_i64(node->node_str());

        return ConstantInt::getSigned(Type::getInt64Ty(getGlobalContext()), val);
            //static_cast<std::uint64_t>(val),
            //true);
        }
    case tok::var:
        {
        Value *v = _named_values[node->node_str()];
        return v ? v : error("Unknown variable referenced");
        }
    case tok::call:
        {
        Function *target_func = _module.get()->getFunction(node->node_str());
        if(!target_func)
            return error("Unknown function referenced");

        if(static_cast<int>(target_func->arg_size()) != node->num_nodes())
            return error("Incorrect number of function arguments");

        std::vector<Value*> vargs;
        bool do_ret = false;
        node->for_nodes([&](std::shared_ptr<ast> n) {
                vargs.push_back(gen_val(n));
                if(!vargs.back())
                    do_ret = true;
                });

        if(do_ret)
            return nullptr;

        return _builder.CreateCall(target_func, vargs, "calltmp");
        }
    case tok::t_if:
        {
        Value *condv = gen_val((*node)[0]);
        if(!condv)
            return nullptr;

        condv = _builder.CreateICmpNE(condv, ConstantInt::get(getGlobalContext(), APInt(64, 0, true)), "ifcond");

        Function *f = _builder.GetInsertBlock()->getParent(); //current block->current function

        BasicBlock *thenBB = BasicBlock::Create(getGlobalContext(), "then", f); //inserted into end of current function
        BasicBlock *elseBB = BasicBlock::Create(getGlobalContext(), "else");
        BasicBlock *mergeBB = BasicBlock::Create(getGlobalContext(), "ifcont");

        _builder.CreateCondBr(condv, thenBB, elseBB);
        _builder.SetInsertPoint(thenBB);

        Value *thenv = gen_val((*node)[1]);
        if(!thenv)
            return nullptr;

        _builder.CreateBr(mergeBB); //all basic blocks must be terminated (return/branch)
        thenBB = _builder.GetInsertBlock(); //in case of nested control

        f->getBasicBlockList().push_back(elseBB);
        _builder.SetInsertPoint(elseBB);

        Value *elsev = gen_val((*node)[2]);
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
    //default:
    case tok::binop:
        {
        //if(is_binop(node->node_type())) {
            std::vector<Value*> side;
            bool do_ret2 = false;
            node->for_nodes([&](std::shared_ptr<ast> n) {
                    side.push_back(gen_val(n));
                    if(!side.back())
                        do_ret2 = true;
                    });

            if(do_ret2)
                return nullptr;

            switch(node->node_str()[0]) {
            case '+': return _builder.CreateAdd(side[0], side[1], "addtmp");
            case '-': return _builder.CreateSub(side[0], side[1], "subtmp");
            case '*': return _builder.CreateMul(side[0], side[1], "multmp");
            case '/': return _builder.CreateSDiv(side[0], side[1], "divtmp");
            //case '<': return _builder.CreateICmpSLT(side[0], side[1], "lttmp");
            case '<':
                {
                if(node->node_str().size() > 1) {
                    if(node->node_str()[1] == '=') {
                        side[0] = _builder.CreateICmpSLE(side[0], side[1], "ltetmp");
                    }
                } else {
                    side[0] = _builder.CreateICmpSLT(side[0], side[1], "lttmp");
                }

                return _builder.CreateSExt(side[0], Type::getInt64Ty(getGlobalContext()));
                }
            //case '>': return _builder.CreateICmpSGT(side[0], side[1], "gttmp");
            case '>':
                {
                if(node->node_str().size() > 1) {
                    if(node->node_str()[1] == '=') {
                        side[0] = _builder.CreateICmpSGE(side[0], side[1], "gtetmp");
                    }
                } else {
                    side[0] = _builder.CreateICmpSGT(side[0], side[1], "gttmp");
                }

                return _builder.CreateSExt(side[0], Type::getInt64Ty(getGlobalContext()));
                }
            case '=':
                {
                if(node->node_str()[1] != '=')
                    return error("Assignment '=' in gen_val");

                side[0] = _builder.CreateICmpEQ(side[0], side[1], "eqtmp");
                return _builder.CreateSExt(side[0], Type::getInt64Ty(getGlobalContext()), "eqtmp");
                }
            default:
                return error("Incorrectly identified binop in jit_engine");
            }
        //}
        }
    default:
        return error("Invalid node type in jit_engine");
    }
}

Function *jit_engine::gen_func(const std::shared_ptr<ast>& node)
{
    switch(node->node_type()) {
    case tok::decl:
    case tok::proto:
        {
        std::vector<Type*> params(static_cast<unsigned int>(node->num_nodes()), Type::getInt64Ty(getGlobalContext()));
        FunctionType *ft = FunctionType::get(Type::getInt64Ty(getGlobalContext()), params, false);
        Function *f = Function::Create(ft, Function::ExternalLinkage, node->node_str(), _module.get());

        if(f->getName() != node->node_str()) { //if function with name already existed, it's implicitly renamed
            f->eraseFromParent();
            f = _module.get()->getFunction(node->node_str());

            if(!f->empty())
                return error("Redefinition of a function");
            if(static_cast<int>(f->arg_size()) != node->num_nodes()) //define or decl after decl
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
    case tok::def:
        {
        _named_values.clear();
        Function *func = gen_func((*node)[0]); //proto

        if(!func)
            return nullptr;

        //if(func->empty())
            //return error("Def with no corresponding body");

        BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "entry", func);
        _builder.SetInsertPoint(bb);

        if(Value *ret = gen_val((*node)[1])) { //body
            _builder.CreateRet(ret);
            verifyFunction(*func);
            _fpm.run(*func);
            return func;
        }

        func->eraseFromParent();
        return nullptr;
        }
    }

    return nullptr;
}

