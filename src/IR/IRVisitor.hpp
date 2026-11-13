#pragma once

#include "IRGenerator.hpp"
#include <any>

class IRVisitor {
public:
  virtual ~IRVisitor() = default;

  virtual std::any visitIRModules(IRModules *node) = 0;

  virtual std::any visitTypeModule(TypeModule *node) = 0;

  virtual std::any visitStructModule(StructModule *node) = 0;

  virtual std::any visitFunctionModule(FunctionModule *node) = 0;

  virtual std::any visitBasicBlock(BasicBlock *node) = 0;

  virtual std::any visitOperand(Operand *node) = 0;

  virtual std::any visitPhiInst(PhiInst *node) = 0;

  virtual std::any visitAllocaInst(AllocaInst *node) = 0;

  virtual std::any visitLoadInst(LoadInst *node) = 0;

  virtual std::any visitStoreInst(StoreInst *node) = 0;

  virtual std::any visitBinaryInst(BinaryInst *node) = 0;

  virtual std::any visitICmpInst(ICmpInst *node) = 0;

  virtual std::any visitCastInst(CastInst *node) = 0;

  virtual std::any visitCallInst(CallInst *node) = 0;

  virtual std::any visitGEPInst(GEPInst *node) = 0;

  virtual std::any visitInsertvalueInst(InsertvalueInst *node) = 0;

  virtual std::any visitExtractvalueInst(ExtractvalueInst *node) = 0;

  virtual std::any visitBrInst(BrInst *node) = 0;

  virtual std::any visitCondInst(CondInst *node) = 0;

  virtual std::any visitReturnInst(ReturnInst *node) = 0;
};
