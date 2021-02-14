#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
using namespace llvm;
static LLVMContext Context;
static Module *ModuleOb = new Module("add.cpp", Context);

Function *createFunc(IRBuilder<> &Builder, std::string name)
{
    std::vector<Type *>  argTypes;
    argTypes.push_back(Builder.getInt32Ty());
    argTypes.push_back(Builder.getInt32Ty());
    //create the function type by ret type and arg type
    FunctionType *funcType = llvm::FunctionType::get(
            /*ret types*/Builder.getInt32Ty(),
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
    Builder.SetInsertPoint(entry);

    Value * result = Builder.CreateAdd(left,right);
    Builder.CreateRet(result);
    addFunc -> dump();
    return addFunc;
}

int main(int argc, char *argv[]) {
    static IRBuilder<> Builder(Context);
    Function *fooFunc = createFunc(Builder, "add"); 
    ModuleOb->dump();
    return 0;
}
