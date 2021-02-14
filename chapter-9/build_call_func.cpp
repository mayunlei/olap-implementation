#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include <iostream>
using namespace llvm;
using namespace std;
static LLVMContext Context;
static Module *ModuleOb = new Module("add.cpp", Context);
Function *createFunc(IRBuilder<> &builder, std::string name)
{
    std::vector<Type *>  argTypes;
    argTypes.push_back(builder.getInt32Ty());
    argTypes.push_back(builder.getInt32Ty());
    //create the function type by ret type and arg type
    FunctionType *funcType = llvm::FunctionType::get(
            /*ret types*/builder.getInt32Ty(),
            /*arg types*/argTypes,
            /*is var arg*/false);
    //create function by function type and name
    Function *addFunc= llvm::Function::Create(
            funcType, 
            llvm::Function::ExternalLinkage, 
            name, 
            ModuleOb);
    auto itr = addFunc -> arg_begin();
    Value * left = &(*itr);
    itr++;
    Value * right= &(*itr);
    //create a new basic block, with label 'entry'
    BasicBlock *entry = BasicBlock::Create(Context, "entry", addFunc);
    builder.SetInsertPoint(entry);

    Value * result = builder.CreateAdd(left,right);
    builder.CreateRet(result);
    return addFunc;
}
void createCall(IRBuilder<> &builder, std::string name,Function * addFunc)
{
    std::vector<Type *>  argTypes;
    FunctionType *funcType = llvm::FunctionType::get(
            builder.getVoidTy(),
            argTypes,
            false);
    Function * callAddFunc =  llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            name,
            ModuleOb);
    BasicBlock *entry = BasicBlock::Create(Context, "entry",callAddFunc);
    builder.SetInsertPoint(entry);
    
    std::vector<Value *> args;
    args.push_back(builder.getInt32(1));
    args.push_back(builder.getInt32(2));

    builder.CreateCall(addFunc,args,"calladd");
}

int main(int argc, char *argv[]) {
    static IRBuilder<> builder(Context);
    Function *addFunc= createFunc(builder, "add"); 
    createCall(builder, "calladd",addFunc);
    ModuleOb->dump();
    return 0;
}
