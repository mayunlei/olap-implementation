#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include <iostream>
using namespace llvm;
using namespace std;
static LLVMContext Context;
static Module *ModuleOb = new Module("add.cpp", Context);
GlobalVariable *createGlobalVar(IRBuilder<> &builder, std::string Name) 
{
    ModuleOb->getOrInsertGlobal(Name, builder.getInt32Ty());
    GlobalVariable *gVarPtr = ModuleOb->getNamedGlobal(Name);
    gVarPtr->setLinkage(GlobalValue::CommonLinkage);
   // gVar->setAlignment(4);
    return gVarPtr;
}
Function *createFunc(IRBuilder<> &builder, std::string name,Value * gVarPtr)
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

    Value * tmp1 = builder.CreateAdd(left,right);
    //std::cout<<tmp1->getType() -> getTypeID()<<":"<<gVar ->getType() ->  getTypeID()<<std::endl;
    Value * gVar = builder.CreateLoad(builder.getInt32Ty(),gVarPtr);
    Value * result = builder.CreateAdd(tmp1, gVar);
    builder.CreateRet(result);
    return addFunc;
}

int main(int argc, char *argv[]) {
    static IRBuilder<> builder(Context);
    Value * gVarPtr = createGlobalVar(builder,"gVar");
    Function *fooFunc = createFunc(builder, "add",gVarPtr); 
    ModuleOb->dump();
    return 0;
}
