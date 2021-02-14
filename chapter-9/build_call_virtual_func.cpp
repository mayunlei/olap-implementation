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
uint64_t devirtualize(uint64_t * fPtr,uint64_t * data){ 
    if((intptr_t)fPtr  & 1) {
        size_t index = fPtr - (uint64_t*)1;
        uint64_t * vTable = (uint64_t*)*data;
        return vTable[index];
    }
    return -1;
}
class BaseClass
{
public:
    virtual int vadd1(int a,int b) = 0;
    virtual int vadd2(int a,int b) = 0;
    virtual int vadd3(int a,int b) = 0;
};
class DerivedClass : public BaseClass
{
public:
    virtual int vadd1(int a,int b) 
    {
        return a+b+1;
    }
    virtual int vadd2(int a,int b) 
    {
        return a+b+2;
    }
    virtual int vadd3(int a,int b) 
    {
        return a+b+3;
    }
};
Function *createFunc(IRBuilder<> &builder, std::string name)
{
    std::vector<Type *>  argTypes;
    argTypes.push_back(builder.getInt8Ty()->getPointerTo());
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
void createCall(IRBuilder<> &builder, std::string name,Function * addFunc,BaseClass * ins)
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
    Value * pointer64 = builder.getInt64((uint64_t)ins);
    Value * objPointer = builder.CreateIntToPtr(pointer64, builder.getInt8Ty() -> getPointerTo(),"voidPtr");
    args.push_back(objPointer);
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
    Function *addFunc= createFunc(builder, "DerivedClass::vadd3"); 
    BaseClass * ins =new DerivedClass();
    createCall(builder, "calladd",addFunc, ins);
    ModuleOb->dump();
    auto H = TheJIT->addModule(std::move(ModuleOb));   
    
    int (DerivedClass::*FP0)(int,int) = &DerivedClass::vadd1;
    std::cout<<(*(reinterpret_cast<uint64_t*>(&FP0)))<<std::endl;
    FP0 = &DerivedClass::vadd2;
    std::cout<<(*(reinterpret_cast<uint64_t*>(&FP0)))<<std::endl;
    int (DerivedClass::*FP1)(int,int) = &DerivedClass::vadd3;
    std::cout<<(*(reinterpret_cast<uint64_t*>(&FP1)))<<std::endl;


    uint64_t methodAddress =  devirtualize( (uint64_t *)(*(reinterpret_cast<uint64_t*>(&FP1))), reinterpret_cast<uint64_t*>(ins));
    TheJIT -> addExternalFunction("DerivedClass::vadd3", methodAddress);
    auto ExprSymbol = TheJIT->findSymbol("calladd");
    int (*FP)() = (int (*)())(intptr_t)cantFail(ExprSymbol.getAddress());
    int res = FP();
    std::cout<<res<<std::endl;
    return 0;
}
