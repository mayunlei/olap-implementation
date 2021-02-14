#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include <iostream>
#include <memory>
#include "KaleidoscopeJIT.h"
using namespace llvm;
using namespace std;
using namespace llvm::orc;
static LLVMContext Context;
static std::unique_ptr<Module>ModuleOb = std::make_unique<Module>("calladd.cpp", Context);
static std::unique_ptr<KaleidoscopeJIT> TheJIT ;
static std::unique_ptr<legacy::FunctionPassManager> TheFPM;
static void InitializeModuleAndPassManager() {
  // Open a new module.
  ModuleOb->setDataLayout(TheJIT->getTargetMachine().createDataLayout());

  // Create a new pass manager attached to it.
  TheFPM = std::make_unique<legacy::FunctionPassManager>(ModuleOb.get());

  // Do simple "peephole" optimizations and bit-twiddling optzns.
  TheFPM->add(createInstructionCombiningPass());
  // Reassociate expressions.
  TheFPM->add(createReassociatePass());
  // Eliminate Common SubExpressions.
  TheFPM->add(createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  TheFPM->add(createCFGSimplificationPass());

  TheFPM->doInitialization();
}
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif
int add(int a,int b)
{
    return a+b;
}
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
            ModuleOb.get());
   // ModuleOb -> getOrInsertFunction("add",funcType);
    return addFunc;
}
void createCall(IRBuilder<> &builder, std::string name,Function * addFunc)
{
    std::vector<Type *>  argTypes;
    FunctionType *funcType = llvm::FunctionType::get(
            builder.getInt32Ty(),
            argTypes,
            false);
    Function * callAddFunc =  llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            name,
            ModuleOb.get());
    BasicBlock *entry = BasicBlock::Create(Context, "entry",callAddFunc);
    builder.SetInsertPoint(entry);
    
    std::vector<Value *> args;
    args.push_back(builder.getInt32(1));
    args.push_back(builder.getInt32(2));

    Value *res = builder.CreateCall(addFunc,args,"calladd");
    builder.CreateRet(res);
}

int main(int argc, char *argv[]) {
    static IRBuilder<> builder(Context);

    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmPrinter();
    TheJIT = std::make_unique<KaleidoscopeJIT>();
    InitializeModuleAndPassManager();
    Function *addFunc= createFunc(builder, "add"); 
    createCall(builder, "calladd",addFunc);
    ModuleOb->dump();
    auto H = TheJIT->addModule(std::move(ModuleOb));   
    
    int (*FP1)(int,int) = &add;
    uint64_t methodAddress= *(reinterpret_cast<uint64_t*>(&FP1));
    TheJIT -> addExternalFunction("add", methodAddress);
    auto ExprSymbol = TheJIT->findSymbol("calladd");
    int (*FP)() = (int (*)())(intptr_t)cantFail(ExprSymbol.getAddress());
    int res = FP();
    std::cout<<res<<std::endl;
    return 0;
}
