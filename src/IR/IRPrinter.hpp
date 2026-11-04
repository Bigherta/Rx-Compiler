#pragma once

#include "IRGenerator.hpp"
#include "IRVisitor.hpp"
#include <cstddef>
#include <ostream>
#include <variant>

class IRPrinter : public IRVisitor {
public:
  std::ostream &out;
  IRPrinter(std::ostream &out) : out(out) {}

  std::any visitIRModules(IRModules *modules) override {
    out << "target datalayout = \"" << modules->dataLayout << "\"\n";
    out << "target triple = \"" << modules->targetTriple << "\"\n\n";

    for (const auto &structure : modules->structs) {
      structure->accept(this);
    }
    out << '\n';
    for (const auto &function : modules->functions) {
      function->accept(this);
    }
    return {};
  }

  std::any visitTypeModule(TypeModule *module) override {
    switch (module->kind) {
    case llvmTypeKind::I32: {
      out << "i32";
      break;
    }
    case llvmTypeKind::I8: {
      out << "i8";
      break;
    }
    case llvmTypeKind::I1: {
      out << "i1";
      break;
    }
    case llvmTypeKind::Ptr: {
      out << "ptr";
      break;
    }
    case llvmTypeKind::Void: {
      out << "void";
      break;
    }
    case llvmTypeKind::Label: {
      break;
    }
    case llvmTypeKind::Array: {
      out << "[" << module->length.value() << " x ";
      module->innerType->accept(this);
      out << "]";
      break;
    }
    case llvmTypeKind::Struct: {
      out << "%" << module->structName.value();
    } break;
    }
    return {};
  }

  std::any visitStructModule(StructModule *module) override {
    out << "%" << module->structName << " = type { ";
    for (std::size_t i = 0; i < module->fields.size(); ++i) {
      if (i > 0) {
        out << ", ";
      }
      module->fields[i]->accept(this);
    }
    out << "}\n";
    return {};
  }

  std::any visitFunctionModule(FunctionModule *module) override {
    const bool isDefined = module->state == functionState::Defined;
    out << (isDefined ? "define " : "declare ");
    module->returnType->accept(this);
    out << " @" << module->functionName << "(";
    for (std::size_t i = 0; i < module->paramTypes.size(); ++i) {
      if (i > 0) {
        out << ", ";
      }
      module->paramTypes[i].first->accept(this);
      if (isDefined) {
        out << " %" << module->paramTypes[i].second.SSAName;
      }
    }
    out << ")";
    if (isDefined) {
      out << " {\n";
      for (const auto &block : module->basicBlocks) {
        block->accept(this);
      }
      out << "}\n";
    } else {
      out << '\n';
    }
    return {};
  }

  std::any visitBasicBlock(BasicBlock *block) override {
    out << block->blockName << ":\n";
    for (const auto &inst : block->insts) {
      out << "  ";
      inst->accept(this);
    }
    if (block->terminator) {
      out << "  ";
      block->terminator->accept(this);
    }
    return {};
  }

  std::any visitOperand(Operand *operand) override {
    if (std::holds_alternative<Operand::SSA>(operand->operand)) {
      out << '%' << std::get<Operand::SSA>(operand->operand).SSAName;
    } else {
      const auto &value =
          std::get<Operand::Immediate>(operand->operand).constValue;
      if (std::holds_alternative<long long>(value)) {
        out << std::get<long long>(value);
      } else {
        out << (std::get<bool>(value) ? 1 : 0);
      }
    }
    return {};
  }

  std::any visitAllocaInst(AllocaInst *inst) override {
    out << "%" << inst->ptrSSA.SSAName << " = alloca ";
    inst->allocaType->accept(this);
    out << '\n';
    return {};
  }

  std::any visitLoadInst(LoadInst *) override { return {}; }

  std::any visitStoreInst(StoreInst *) override { return {}; }

  std::any visitBinaryInst(BinaryInst *) override { return {}; }

  std::any visitICmpInst(ICmpInst *) override { return {}; }

  std::any visitCastInst(CastInst *) override { return {}; }

  std::any visitCallInst(CallInst *) override { return {}; }

  std::any visitGEPInst(GEPInst *) override { return {}; }

  std::any visitExtractvalueInst(ExtractvalueInst *) override { return {}; }

  std::any visitInsertvalueInst(InsertvalueInst *) override { return {}; }

  std::any visitBrInst(BrInst *) override { return {}; }

  std::any visitCondInst(CondInst *) override { return {}; }

  std::any visitReturnInst(ReturnInst *inst) override {
    out << "ret ";
    inst->returnType->accept(this);
    if (inst->returnValue) {
      out << ' ';
      inst->returnValue->accept(this);
    }
    out << '\n';
    return {};
  }
};
