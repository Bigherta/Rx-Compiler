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
      module->entry->accept(this);
      for (const auto &[name, block] : module->basicBlocks) {
        if (block.get() != module->entry) {
          block->accept(this);
        }
      }
      out << "}\n";
    } else {
      out << '\n';
    }
    return {};
  }

  std::any visitBasicBlock(BasicBlock *block) override {
    out << block->blockName << ":\n";
    for (const auto &phi : block->phis) {
      out << "  ";
      phi->accept(this);
    }
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
      const auto &name = std::get<Operand::SSA>(operand->operand).SSAName;
      if (name != "poison") {
        out << '%';
      }
      out << name;
    } else if (std::holds_alternative<Operand::Immediate>(operand->operand)) {
      const auto &value =
          std::get<Operand::Immediate>(operand->operand).constValue;
      if (std::holds_alternative<long long>(value)) {
        out << std::get<long long>(value);
      } else {
        out << (std::get<bool>(value) ? 1 : 0);
      }
    } else {
      out << "zeroinitializer";
    }
    return {};
  }

  std::any visitPhiInst(PhiInst *inst) override {
    out << "%" << inst->resultSSA.SSAName << " = phi ";
    inst->type->accept(this);
    out << ' ';
    for (std::size_t i = 0; i < inst->incoming.size(); ++i) {
      if (i > 0) {
        out << ", ";
      }
      auto &[value, block] = inst->incoming[i];
      out << "[ ";
      value.accept(this);
      out << ", %" << block->blockName << " ]";
    }
    out << '\n';
    return {};
  }

  std::any visitAllocaInst(AllocaInst *inst) override {
    out << "%" << inst->ptrSSA.SSAName << " = alloca ";
    inst->allocaType->accept(this);
    out << '\n';
    return {};
  }

  std::any visitLoadInst(LoadInst *inst) override {
    out << "%" << inst->valueSSA.SSAName << " = load ";
    inst->loadType->accept(this);
    out << ", ptr %" << inst->ptrSSA.SSAName << '\n';
    return {};
  }

  std::any visitStoreInst(StoreInst *inst) override {
    out << "store ";
    inst->storeType->accept(this);
    out << ' ';
    inst->storeValue.accept(this);
    out << ", ptr %" << inst->ptrSSA.SSAName << '\n';
    return {};
  }

  std::any visitBinaryInst(BinaryInst *inst) override {
    out << "%" << inst->resultPtr.SSAName << " = ";
    switch (inst->opKind) {
    case BinaryIntergerOperator::Add:
      out << "add";
      break;
    case BinaryIntergerOperator::Sub:
      out << "sub";
      break;
    case BinaryIntergerOperator::Mul:
      out << "mul";
      break;
    case BinaryIntergerOperator::UDiv:
      out << "udiv";
      break;
    case BinaryIntergerOperator::SDiv:
      out << "sdiv";
      break;
    case BinaryIntergerOperator::URem:
      out << "urem";
      break;
    case BinaryIntergerOperator::SRem:
      out << "srem";
      break;
    case BinaryIntergerOperator::Shl:
      out << "shl";
      break;
    case BinaryIntergerOperator::LShr:
      out << "lshr";
      break;
    case BinaryIntergerOperator::AShr:
      out << "ashr";
      break;
    case BinaryIntergerOperator::And:
      out << "and";
      break;
    case BinaryIntergerOperator::Or:
      out << "or";
      break;
    case BinaryIntergerOperator::Xor:
      out << "xor";
      break;
    }
    out << " ";
    inst->type->accept(this);
    out << " ";
    inst->leftChild.accept(this);
    out << ", ";
    inst->rightChild.accept(this);
    out << '\n';
    return {};
  }

  std::any visitICmpInst(ICmpInst *inst) override {
    out << "%" << inst->resultPtr.SSAName << " = icmp ";
    switch (inst->opKind) {
    case ICmpOperator::Eq:
      out << "eq";
      break;
    case ICmpOperator::Ne:
      out << "ne";
      break;
    case ICmpOperator::Ugt:
      out << "ugt";
      break;
    case ICmpOperator::Uge:
      out << "uge";
      break;
    case ICmpOperator::Ult:
      out << "ult";
      break;
    case ICmpOperator::Ule:
      out << "ule";
      break;
    case ICmpOperator::Sgt:
      out << "sgt";
      break;
    case ICmpOperator::Sge:
      out << "sge";
      break;
    case ICmpOperator::Slt:
      out << "slt";
      break;
    case ICmpOperator::Sle:
      out << "sle";
      break;
    }
    out << " ";
    inst->cmpType->accept(this);
    out << " ";
    inst->leftChild.accept(this);
    out << ", ";
    inst->rightChild.accept(this);
    out << '\n';
    return {};
  }

  std::any visitCastInst(CastInst *inst) override {
    out << "%" << inst->resultPtr.SSAName << " = ";
    switch (inst->opKind) {
    case CastOperator::Trunc:
      out << "trunc";
      break;
    case CastOperator::ZExt:
      out << "zext";
      break;
    case CastOperator::SExt:
      out << "sext";
      break;
    }
    out << " ";
    inst->fromType->accept(this);
    out << " ";
    inst->castObj.accept(this);
    out << " to ";
    inst->toType->accept(this);
    out << '\n';
    return {};
  }

  std::any visitCallInst(CallInst *inst) override {
    if (inst->resultPtr) {
      out << "%" << inst->resultPtr->SSAName << " = ";
    }
    out << "call ";
    inst->resultType->accept(this);
    out << " @" << inst->calleeName << "(";
    for (std::size_t i = 0; i < inst->params.size(); ++i) {
      if (i != 0) {
        out << ", ";
      }
      inst->params[i].first->accept(this);
      out << " ";
      inst->params[i].second.accept(this);
    }
    out << ")\n";
    return {};
  }

  std::any visitGEPInst(GEPInst *inst) override {
    out << "%" << inst->resultPtr.SSAName << " = getelementptr ";
    inst->getObjType->accept(this);
    out << ", ptr %" << inst->initialPtr.SSAName;
    for (auto &index : inst->indexOfElements) {
      out << ", i32 ";
      index.accept(this);
    }
    out << '\n';
    return {};
  }

  std::any visitInsertvalueInst(InsertvalueInst *inst) override {
    out << "%" << inst->resultPtr.SSAName << " = insertvalue ";
    inst->aggregateType->accept(this);
    out << ' ';
    Operand aggregateValue(inst->aggregatePtr);
    aggregateValue.accept(this);
    out << ", ";
    inst->newElementType->accept(this);
    out << ' ';
    inst->newElementValue.accept(this);
    for (auto &index : inst->indexOfElements) {
      out << ", ";
      out << index;
    }
    out << '\n';
    return {};
  }

  std::any visitExtractvalueInst(ExtractvalueInst *inst) override {
    out << "%" << inst->resultPtr.SSAName << " = extractvalue ";
    inst->aggregateType->accept(this);
    out << ' ';
    inst->aggregateValue.accept(this);
    for (auto &index : inst->indexOfElements) {
      out << ", " << index;
    }
    out << '\n';
    return {};
  }

  std::any visitBrInst(BrInst *inst) override {
    out << "br label %" << inst->jumpTarget->blockName << '\n';
    return {};
  }

  std::any visitCondInst(CondInst *inst) override {
    out << "br i1 ";
    inst->cond.accept(this);
    out << ", label %" << inst->trueTarget->blockName << ", label %"
        << inst->falseTarget->blockName << '\n';
    return {};
  }

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
