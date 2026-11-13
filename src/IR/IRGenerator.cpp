#include "IRGenerator.hpp"
#include "AST.hpp"
#include "IRVisitor.hpp"
#include "SemanticAnalyzer.hpp"
#include "SemanticResult.hpp"
#include "SymbolCollector.hpp"
#include <algorithm>
#include <any>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace helper {
bool isSignedType(const Type &type) {
  return type.kind == TypeKind::Isize || type.kind == TypeKind::I32;
}
bool isBooleanType(const Type &type) { return type.kind == TypeKind::Bool; }
}; // namespace helper

std::any IRModules::accept(IRVisitor *visitor) {
  return visitor->visitIRModules(this);
}

std::any TypeModule::accept(IRVisitor *visitor) {
  return visitor->visitTypeModule(this);
}

std::any StructModule::accept(IRVisitor *visitor) {
  return visitor->visitStructModule(this);
}

std::any FunctionModule::accept(IRVisitor *visitor) {
  return visitor->visitFunctionModule(this);
}

std::any BasicBlock::accept(IRVisitor *visitor) {
  return visitor->visitBasicBlock(this);
}

std::any Operand::accept(IRVisitor *visitor) {
  return visitor->visitOperand(this);
}

std::any PhiInst::accept(IRVisitor *visitor) {
  return visitor->visitPhiInst(this);
}

std::any AllocaInst::accept(IRVisitor *visitor) {
  return visitor->visitAllocaInst(this);
}

std::any LoadInst::accept(IRVisitor *visitor) {
  return visitor->visitLoadInst(this);
}

std::any StoreInst::accept(IRVisitor *visitor) {
  return visitor->visitStoreInst(this);
}

std::any BinaryInst::accept(IRVisitor *visitor) {
  return visitor->visitBinaryInst(this);
}

std::any ICmpInst::accept(IRVisitor *visitor) {
  return visitor->visitICmpInst(this);
}

std::any CastInst::accept(IRVisitor *visitor) {
  return visitor->visitCastInst(this);
}

std::any ReturnInst::accept(IRVisitor *visitor) {
  return visitor->visitReturnInst(this);
}

std::any CallInst::accept(IRVisitor *visitor) {
  return visitor->visitCallInst(this);
}

std::any GEPInst::accept(IRVisitor *visitor) {
  return visitor->visitGEPInst(this);
}

std::any InsertvalueInst::accept(IRVisitor *visitor) {
  return visitor->visitInsertvalueInst(this);
}

std::any ExtractvalueInst::accept(IRVisitor *visitor) {
  return visitor->visitExtractvalueInst(this);
}

std::any BrInst::accept(IRVisitor *visitor) {
  return visitor->visitBrInst(this);
}

std::any CondInst::accept(IRVisitor *visitor) {
  return visitor->visitCondInst(this);
}

void IRModules::addRxItems(const SemanticAnalyzer *analyzer) {
  auto emplaceFunction = [&](FunctionSymbol *symbol) {
    functions.emplace_back(std::make_unique<FunctionModule>(symbol));
    functionModuleMap.emplace(symbol, functions.back().get());
  };
  for (const auto &[structName, structItem] :
       analyzer->globalScopeVisitor->typeNameSpace) {
    auto *symbol = dynamic_cast<StructSymbol *>(structItem.get());
    structs.emplace_back(std::make_unique<StructModule>(symbol));
    structModuleMap.emplace(structs.back()->structName, structs.back().get());
    for (auto &[itemName, itemSymbol] : symbol->structSymbols) {
      if (auto *funcSymbol = dynamic_cast<FunctionSymbol *>(itemSymbol.get())) {
        emplaceFunction(funcSymbol);
      }
    }
  }
  for (const auto &[funcName, funcItem] :
       analyzer->globalScopeVisitor->valueNameSpace) {
    if (auto *symbol = dynamic_cast<FunctionSymbol *>(funcItem.get())) {
      emplaceFunction(symbol);
    }
  }
}

FunctionModule *IRGenerator::getOrCreateFunction(FunctionSymbol *symbol) {
  auto iterator = modules->functionModuleMap.find(symbol);
  if (iterator != modules->functionModuleMap.end()) {
    return iterator->second;
  }
  auto function = std::make_unique<FunctionModule>(symbol);
  auto *module = function.get();
  modules->functions.emplace_back(std::move(function));
  modules->functionModuleMap.emplace(symbol, module);
  pendingBuiltinFunctions.push_back(module);
  return module;
}

void IRGenerator::beginFunction(
    FunctionModule *module, const std::vector<std::string> &parameterNames) {
  currentFunction = module;
  guard.clear();
  for (std::size_t i = 0; i < module->paramTypes.size(); ++i) {
    module->paramTypes[i].second =
        guard.makeSSA(parameterNames.at(i), SSAType::Value);
  }
  auto blockName = guard.generateBasicBlockName();
  module->basicBlocks.emplace(blockName,
                              std::make_unique<BasicBlock>(blockName));
  currentBlock = module->basicBlocks.at(blockName).get();
  module->entry = currentBlock;
}

std::optional<Operand> IRGenerator::emitValue(ExprNode *node) {
  auto result = std::any_cast<std::optional<Operand>>(node->accept(this));
  if (!result) {
    return std::nullopt;
  }
  auto operand = result.value();
  std::optional<Operand> placeAddress = std::nullopt;
  if (node->semantic->isPlace) {
    auto valueSSA = guard.makeSSA(SSAType::Value);
    auto llvmType = std::make_unique<TypeModule>(
        TypeModule::lowerType(&node->semantic->originalType, false));
    currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
        std::move(llvmType), Operand::SSA(operand), valueSSA));
    placeAddress = operand;
    operand = valueSSA;
  }
  TypeModule currentType =
      TypeModule::lowerType(&node->semantic->originalType, false);
  for (const auto &adjustment : node->semantic->adjustments) {
    switch (adjustment.kind) {
    case AdjustmentKind::Deref: {
      auto toType = std::make_unique<TypeModule>(
          TypeModule::lowerType(&adjustment.targetType, false));
      auto valueSSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
          std::move(toType), Operand::SSA(operand), valueSSA));
      placeAddress = operand;
      operand = valueSSA;
      currentType = TypeModule::lowerType(&adjustment.targetType, false);
      break;
    }
    case AdjustmentKind::Borrow: {
      if (placeAddress) {
        operand = placeAddress.value();
      } else {
        auto addrSSA = guard.makeSSA(SSAType::Addr);
        {
          auto _ = std::make_unique<TypeModule>(currentType);

          currentBlock->insts.emplace_back(
              std::make_unique<AllocaInst>(addrSSA, std::move(_)));
        }
        {
          auto _ = std::make_unique<TypeModule>(currentType);
          currentBlock->insts.emplace_back(
              std::make_unique<StoreInst>(std::move(_), operand, addrSSA));
        }
        operand = addrSSA;
      }
      break;
    }
    case AdjustmentKind::MutToShared:
      break;
    case AdjustmentKind::NeverToAny:
      break;
    }
  }
  return operand;
}

std::optional<Operand> IRGenerator::emitPlace(ExprNode *node) {
  auto result = std::any_cast<std::optional<Operand>>(node->accept(this));
  if (!result) {
    return std::nullopt;
  }
  auto operand = result.value();
  if (!node->semantic->isPlace) {
    auto temporaryPlace = guard.makeSSA(SSAType::Addr);
    {
      auto _ = std::make_unique<TypeModule>(
          TypeModule::lowerType(&node->semantic->originalType, false));

      currentBlock->insts.emplace_back(
          std::make_unique<AllocaInst>(temporaryPlace, std::move(_)));
    }
    {
      auto _ = std::make_unique<TypeModule>(
          TypeModule::lowerType(&node->semantic->originalType, false));
      currentBlock->insts.emplace_back(
          std::make_unique<StoreInst>(std::move(_), operand, temporaryPlace));
      operand = temporaryPlace;
    }
  }
  for (const auto &adjustment : node->semantic->adjustments) {
    switch (adjustment.kind) {
    case AdjustmentKind::Deref: {
      auto nextAddress = guard.makeSSA(SSAType::Addr);
      currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
          std::make_unique<TypeModule>(llvmTypeKind::Ptr), operand,
          nextAddress));
      operand = nextAddress;
      break;
    }
    case AdjustmentKind::Borrow: {
      auto borrowSSA = guard.makeSSA(SSAType::Addr);
      {
        auto _ = std::make_unique<TypeModule>(
            TypeModule::lowerType(&adjustment.targetType, false));
        currentBlock->insts.emplace_back(
            std::make_unique<AllocaInst>(borrowSSA, std::move(_)));
      }
      {
        auto _ = std::make_unique<TypeModule>(
            TypeModule::lowerType(&adjustment.targetType, false));
        currentBlock->insts.emplace_back(
            std::make_unique<StoreInst>(std::move(_), operand, borrowSSA));
      }
      operand = borrowSSA;
      break;
    }
    case AdjustmentKind::MutToShared:
      break;
    case AdjustmentKind::NeverToAny:
      break;
    }
  }
  return operand;
}

// Crate and attribute.
std::any IRGenerator::visitCrate(CrateNode *node) {
  for (auto &item : node->items) {
    item->accept(this);
  }
  return {};
}

// Items.
std::any IRGenerator::visitFunctionDeclItem(FunctionDeclItemNode *node) {
  auto *module = modules->functionModuleMap.at(node->functionSymbol);
  std::vector<std::string> parameterNames;
  parameterNames.reserve(module->paramTypes.size());
  const bool hasSelf = node->selfParam.has_value();
  for (std::size_t i = 0; i < module->paramTypes.size(); ++i) {
    parameterNames.emplace_back(
        hasSelf && i == 0
            ? std::string("self")
            : node->functionParam[i - (hasSelf ? 1 : 0)].identifier);
  }
  beginFunction(module, parameterNames);
  auto defineParam = [&](const LocalSymbol *symbol, const TypeModule &module,
                         std::size_t &index) {
    auto ptr = guard.makeSSA(symbol->varName, SSAType::Addr);
    {
      auto _ = std::make_unique<TypeModule>(module);
      currentBlock->insts.emplace_back(
          std::make_unique<AllocaInst>(ptr, std::move(_)));
      guard.localAddressMap.emplace(symbol, ptr);
    }
    {
      auto _ = std::make_unique<TypeModule>(module);
      currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
          std::move(_), currentFunction->paramTypes[index].second, ptr));
    }
    ++index;
  };
  std::size_t index = 0;
  if (node->selfParam) {
    defineParam(node->selfParam->paramSymbol,
                *currentFunction->paramTypes[index].first, index);
  }
  for (auto &param : node->functionParam) {
    defineParam(param.paramSymbol, *currentFunction->paramTypes[index].first,
                index);
  }
  auto visitor = node->functionBody->accept(this);
  if (!currentBlock->terminator) {
    if (currentFunction->returnType->kind == llvmTypeKind::Void) {
      currentBlock->terminator = std::make_unique<ReturnInst>(
          std::make_unique<TypeModule>(llvmTypeKind::Void));
    } else {
      auto _ = std::make_unique<TypeModule>(TypeModule::lowerType(
          &node->functionBody->semantic->adjustedType, true));
      currentBlock->terminator = std::make_unique<ReturnInst>(
          std::move(_), std::any_cast<std::optional<Operand>>(visitor).value());
    }
  }
  currentFunction->state = functionState::Defined;
  return {};
}

std::any IRGenerator::visitImplDeclItem(ImplDeclItemNode *node) {
  for (auto &associateItem : node->associatedItems) {
    if (std::holds_alternative<std::unique_ptr<FunctionDeclItemNode>>(
            associateItem)) {
      auto *associateFunc =
          std::get<std::unique_ptr<FunctionDeclItemNode>>(associateItem).get();
      associateFunc->accept(this);
    }
  }
  return {};
}

// Statements.
std::any IRGenerator::visitEmptyStmt(EmptyStmtNode *) { return {}; }

std::any IRGenerator::visitLetStmt(LetStmtNode *node) {
  auto valueOperand = emitValue(node->value.get());
  if (!valueOperand) {
    return {};
  }
  auto allocatePtr = guard.makeSSA(node->identifier, SSAType::Addr);
  guard.localAddressMap.emplace(node->semantic->bindingSymbol, allocatePtr);
  {
    auto _ = std::make_unique<TypeModule>(
        TypeModule::lowerType(&node->semantic->statementType, false));
    currentBlock->insts.emplace_back(
        std::make_unique<AllocaInst>(allocatePtr, std::move(_)));
  }
  {
    auto _ = std::make_unique<TypeModule>(
        TypeModule::lowerType(&node->semantic->statementType, false));
    currentBlock->insts.emplace_back(
        std::make_unique<StoreInst>(std::move(_), *valueOperand, allocatePtr));
  }
  return {};
}

std::any IRGenerator::visitExpressionStmt(ExpressionStmtNode *node) {
  node->expr->accept(this);
  return {};
}

// Expressions without blocks.
std::any IRGenerator::visitUnitExpr(UnitExprNode *) {
  return std::optional<Operand>{Operand(Operand::Unit{})};
}

// specialised for value lower down
std::any IRGenerator::visitPathInExpr(PathInExprNode *node) {
  if (auto *value =
          dynamic_cast<ValuePathInExprSemantic *>(node->semantic.get())) {
    if (std::holds_alternative<const ConstantSymbol *>(value->bindingSymbol)) {
      const auto *constantSymbol =
          std::get<const ConstantSymbol *>(value->bindingSymbol);
      if (std::holds_alternative<long long>(constantSymbol->value.value)) {
        Operand::Immediate imm(
            std::get<long long>(constantSymbol->value.value));
        return std::optional<Operand>{Operand(imm)};
      } else if (std::holds_alternative<bool>(constantSymbol->value.value)) {
        Operand::Immediate imm(std::get<bool>(constantSymbol->value.value));
        return std::optional<Operand>{Operand(imm)};
      } else {
        std::unreachable();
      }
    } else {
      const auto *localSymbol =
          std::get<const LocalSymbol *>(value->bindingSymbol);
      return std::optional<Operand>{
          Operand(guard.localAddressMap.at(localSymbol))};
    }
  }
  std::unreachable();
}

std::any IRGenerator::visitLiteralExpr(LiteralExprNode *node) {

  auto *literalSemantic =
      dynamic_cast<LiteralExprSemantic *>(node->semantic.get());
  Operand::Immediate imm(std::move(literalSemantic->value));
  return std::optional<Operand>{Operand(imm)};
}

std::any IRGenerator::visitUnaryExpr(UnaryExprNode *node) {
  auto operand = emitValue(node->childExpr.get());
  if (!operand) {
    return std::optional<Operand>{};
  }
  auto valueSSA = guard.makeSSA(SSAType::Value);
  if (node->op == UnaryOperator::NEG) {
    auto type = std::make_unique<TypeModule>(llvmTypeKind::I32);
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        BinaryIntergerOperator::Sub, std::move(type),
        Operand(Operand::Immediate(0ll)), *operand, valueSSA));
  } else {
    auto type = std::make_unique<TypeModule>(
        node->semantic->adjustedType.kind == TypeKind::Bool
            ? llvmTypeKind::I8
            : llvmTypeKind::I32);
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        BinaryIntergerOperator::Xor, std::move(type),
        Operand(Operand::Immediate(
            node->semantic->adjustedType.kind == TypeKind::Bool ? 1ll : -1ll)),
        *operand, valueSSA));
  }
  return std::optional<Operand>{Operand(valueSSA)};
}

std::any IRGenerator::visitBinaryExpr(BinaryExprNode *node) {
  auto leftOperand = emitValue(node->leftChildExpr.get());
  if (!leftOperand) {
    return std::optional<Operand>{};
  }
  auto rightOperand = emitValue(node->rightChildExpr.get());
  if (!rightOperand) {
    return std::optional<Operand>{};
  }
  if (node->op == BinaryOperator::ShiftLeft ||
      node->op == BinaryOperator::ShiftRight) {
    auto countSSA = guard.makeSSA(SSAType::Value);
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        BinaryIntergerOperator::And,
        std::make_unique<TypeModule>(llvmTypeKind::I32), *rightOperand,
        Operand(Operand::Immediate(31ll)), countSSA));
    rightOperand = Operand(countSSA);
  }
  auto valueSSA = guard.makeSSA(SSAType::Value);
  switch (node->op) {
  case BinaryOperator::Add: {
    auto type = std::make_unique<TypeModule>(llvmTypeKind::I32);
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        BinaryIntergerOperator::Add, std::move(type), *leftOperand,
        *rightOperand, valueSSA));
    return std::optional<Operand>{Operand(valueSSA)};
  }
  case BinaryOperator::Sub: {
    auto type = std::make_unique<TypeModule>(llvmTypeKind::I32);
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        BinaryIntergerOperator::Sub, std::move(type), *leftOperand,
        *rightOperand, valueSSA));
    return std::optional<Operand>{Operand(valueSSA)};
  }
  case BinaryOperator::Mul: {
    auto type = std::make_unique<TypeModule>(llvmTypeKind::I32);
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        BinaryIntergerOperator::Mul, std::move(type), *leftOperand,
        *rightOperand, valueSSA));
    return std::optional<Operand>{Operand(valueSSA)};
  }
  case BinaryOperator::Div: {
    auto type = std::make_unique<TypeModule>(
        TypeModule::lowerType(&node->semantic->adjustedType, false));
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        type->isSigned ? BinaryIntergerOperator::SDiv
                       : BinaryIntergerOperator::UDiv,
        std::move(type), *leftOperand, *rightOperand, valueSSA));
    return std::optional<Operand>{Operand(valueSSA)};
  }
  case BinaryOperator::Mod: {
    auto type = std::make_unique<TypeModule>(
        TypeModule::lowerType(&node->semantic->adjustedType, false));
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        type->isSigned ? BinaryIntergerOperator::SRem
                       : BinaryIntergerOperator::URem,
        std::move(type), *leftOperand, *rightOperand, valueSSA));
    return std::optional<Operand>{Operand(valueSSA)};
  }
  case BinaryOperator::BitAnd: {
    auto type = std::make_unique<TypeModule>(
        TypeModule::lowerType(&node->semantic->adjustedType, false));
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        BinaryIntergerOperator::And, std::move(type), *leftOperand,
        *rightOperand, valueSSA));
    return std::optional<Operand>{Operand(valueSSA)};
  }
  case BinaryOperator::BitOr: {
    auto type = std::make_unique<TypeModule>(
        TypeModule::lowerType(&node->semantic->adjustedType, false));
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        BinaryIntergerOperator::Or, std::move(type), *leftOperand,
        *rightOperand, valueSSA));
    return std::optional<Operand>{Operand(valueSSA)};
  }
  case BinaryOperator::BitXor: {
    auto type = std::make_unique<TypeModule>(
        TypeModule::lowerType(&node->semantic->adjustedType, false));
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        BinaryIntergerOperator::Xor, std::move(type), *leftOperand,
        *rightOperand, valueSSA));
    return std::optional<Operand>{Operand(valueSSA)};
  }
  case BinaryOperator::ShiftLeft: {
    auto type = std::make_unique<TypeModule>(
        TypeModule::lowerType(&node->semantic->adjustedType, false));
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        BinaryIntergerOperator::Shl, std::move(type), *leftOperand,
        *rightOperand, valueSSA));
    return std::optional<Operand>{Operand(valueSSA)};
  }
  case BinaryOperator::ShiftRight: {
    auto type = std::make_unique<TypeModule>(
        TypeModule::lowerType(&node->semantic->adjustedType, false));
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        node->leftChildExpr->semantic->adjustedType.kind == TypeKind::I32 ||
                node->leftChildExpr->semantic->adjustedType.kind ==
                    TypeKind::Isize
            ? BinaryIntergerOperator::AShr
            : BinaryIntergerOperator::LShr,
        std::move(type), *leftOperand, *rightOperand, valueSSA));
    return std::optional<Operand>{Operand(valueSSA)};
  }
  }
  std::unreachable();
}

void IRGenerator::emitEqualFunction(const std::string &typeManglingName,
                                    const Type &compareType) {
  guard.clear();
  auto returnType = std::make_unique<TypeModule>(llvmTypeKind::I1);
  std::vector<std::pair<std::unique_ptr<TypeModule>, Operand::SSA>> paramTypes;
  Operand left = guard.makeSSA(SSAType::Value);
  Operand right = guard.makeSSA(SSAType::Value);
  paramTypes.emplace_back(std::pair{
      std::make_unique<TypeModule>(TypeModule::lowerType(&compareType, false)),
      left});
  paramTypes.emplace_back(std::pair{
      std::make_unique<TypeModule>(TypeModule::lowerType(&compareType, false)),
      right});
  auto equalFunction = std::make_unique<FunctionModule>(
      functionState::Defined, typeManglingName + "_equal",
      std::move(returnType), std::move(paramTypes));
  currentFunction = equalFunction.get();
  auto blockName = guard.generateBasicBlockName();
  equalFunction->basicBlocks.emplace(blockName,
                                     std::make_unique<BasicBlock>(blockName));
  currentBlock = equalFunction->basicBlocks.at(blockName).get();
  equalFunction->entry = currentBlock;
  auto temporaryMaterialize = [&](const Operand &operand) {
    auto temporaryPlace = guard.makeSSA(SSAType::Addr);
    {
      auto _ = std::make_unique<TypeModule>(
          TypeModule::lowerType(&compareType, false));

      currentBlock->insts.emplace_back(
          std::make_unique<AllocaInst>(temporaryPlace, std::move(_)));
    }
    {
      auto _ = std::make_unique<TypeModule>(
          TypeModule::lowerType(&compareType, false));
      currentBlock->insts.emplace_back(
          std::make_unique<StoreInst>(std::move(_), operand, temporaryPlace));
    }
    return temporaryPlace;
  };
  switch (compareType.kind) {
  case TypeKind::I32:
  case TypeKind::U32:
  case TypeKind::Isize:
  case TypeKind::Usize: {
    auto compareSSA = guard.makeSSA(SSAType::Value);
    auto type = std::make_unique<TypeModule>(llvmTypeKind::I32);
    currentBlock->insts.emplace_back(std::make_unique<ICmpInst>(
        ICmpOperator::Eq, std::move(type), left, right, compareSSA));
    currentBlock->terminator = std::make_unique<ReturnInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I1), compareSSA);
    break;
  }
  case TypeKind::Bool: {
    auto compareSSA = guard.makeSSA(SSAType::Value);
    auto type = std::make_unique<TypeModule>(llvmTypeKind::I8);
    currentBlock->insts.emplace_back(std::make_unique<ICmpInst>(
        ICmpOperator::Eq, std::move(type), left, right, compareSSA));
    currentBlock->terminator = std::make_unique<ReturnInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I1), compareSSA);
    break;
  }
  case TypeKind::Vec: {
    auto leftPlace = temporaryMaterialize(left);
    auto rightPlace = temporaryMaterialize(right);
    auto leftLengthSSA = emitVecLength(leftPlace);
    auto rightLengthSSA = emitVecLength(rightPlace);
    auto sameLengthSSA = guard.makeSSA(SSAType::Value);
    currentBlock->insts.emplace_back(std::make_unique<ICmpInst>(
        ICmpOperator::Eq, std::make_unique<TypeModule>(llvmTypeKind::I32),
        leftLengthSSA, rightLengthSSA, sameLengthSSA));
    auto *lengthCheckBlock = currentBlock;
    auto setupBlockName = guard.generateBasicBlockName();
    currentFunction->basicBlocks.emplace(
        setupBlockName, std::make_unique<BasicBlock>(setupBlockName));
    auto *setupBlock = currentFunction->basicBlocks.at(setupBlockName).get();
    auto mergeBlockName = guard.generateBasicBlockName();
    currentFunction->basicBlocks.emplace(
        mergeBlockName, std::make_unique<BasicBlock>(mergeBlockName));
    auto *mergeBlock = currentFunction->basicBlocks.at(mergeBlockName).get();
    currentBlock->terminator =
        std::make_unique<CondInst>(sameLengthSSA, setupBlock, mergeBlock);
    currentBlock = setupBlock;
    auto leftPtrSSA = emitVecPtr(leftPlace);
    auto rightPtrSSA = emitVecPtr(rightPlace);
    auto indexSSA = guard.makeSSA(SSAType::Addr);
    currentBlock->insts.emplace_back(std::make_unique<AllocaInst>(
        indexSSA, std::make_unique<TypeModule>(llvmTypeKind::I32)));
    currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I32), Operand(0), indexSSA));
    auto lengthBlockName = guard.generateBasicBlockName();
    currentFunction->basicBlocks.emplace(
        lengthBlockName, std::make_unique<BasicBlock>(lengthBlockName));
    auto *lengthBlock = currentFunction->basicBlocks.at(lengthBlockName).get();
    currentBlock->terminator = std::make_unique<BrInst>(lengthBlock);
    currentBlock = lengthBlock;
    auto indexValueSSA = guard.makeSSA(SSAType::Value);
    currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I32), indexSSA,
        indexValueSSA));
    auto lengthLimitSSA = guard.makeSSA(SSAType::Value);
    currentBlock->insts.emplace_back(std::make_unique<ICmpInst>(
        ICmpOperator::Ult, std::make_unique<TypeModule>(llvmTypeKind::I32),
        indexValueSSA, leftLengthSSA, lengthLimitSSA));
    auto equalBlockName = guard.generateBasicBlockName();
    currentFunction->basicBlocks.emplace(
        equalBlockName, std::make_unique<BasicBlock>(equalBlockName));
    auto *equalBlock = currentFunction->basicBlocks.at(equalBlockName).get();
    currentBlock->terminator =
        std::make_unique<CondInst>(lengthLimitSSA, equalBlock, mergeBlock);
    currentBlock = equalBlock;
    auto leftChildOperand = guard.makeSSA(SSAType::Value);
    auto rightChildOperand = guard.makeSSA(SSAType::Value);
    {
      auto leftChildAddr = guard.makeSSA(SSAType::Addr);
      auto innerType = std::make_unique<TypeModule>(
          TypeModule::lowerType(compareType.elementType.get(), false));
      currentBlock->insts.emplace_back(std::make_unique<GEPInst>(
          leftChildAddr, std::make_unique<TypeModule>(*innerType), leftPtrSSA,
          std::vector<Operand>{indexValueSSA}));
      currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
          std::move(innerType), leftChildAddr, leftChildOperand));
    }
    {
      auto rightChildAddr = guard.makeSSA(SSAType::Addr);
      auto innerType = std::make_unique<TypeModule>(
          TypeModule::lowerType(compareType.elementType.get(), false));
      currentBlock->insts.emplace_back(std::make_unique<GEPInst>(
          rightChildAddr, std::make_unique<TypeModule>(*innerType), rightPtrSSA,
          std::vector<Operand>{indexValueSSA}));
      currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
          std::move(innerType), rightChildAddr, rightChildOperand));
    }
    auto equalLimitSSA = guard.makeSSA(SSAType::Value);
    std::vector<std::pair<std::unique_ptr<TypeModule>, Operand>> callParams;
    callParams.emplace_back(
        std::pair{std::make_unique<TypeModule>(TypeModule::lowerType(
                      compareType.elementType.get(), false)),
                  leftChildOperand});
    callParams.emplace_back(
        std::pair{std::make_unique<TypeModule>(TypeModule::lowerType(
                      compareType.elementType.get(), false)),
                  rightChildOperand});
    currentBlock->insts.emplace_back(std::make_unique<CallInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I1), equalLimitSSA,
        SemanticAnalyzer::encodeType(*compareType.elementType) + "_equal",
        std::move(callParams)));
    auto *equalFinalBlock = currentBlock;
    auto bodyBlockName = guard.generateBasicBlockName();
    currentFunction->basicBlocks.emplace(
        bodyBlockName, std::make_unique<BasicBlock>(bodyBlockName));
    auto *bodyBlock = currentFunction->basicBlocks.at(bodyBlockName).get();
    currentBlock->terminator =
        std::make_unique<CondInst>(equalLimitSSA, bodyBlock, mergeBlock);
    currentBlock = bodyBlock;
    auto nextIndexSSA = guard.makeSSA(SSAType::Value);
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        BinaryIntergerOperator::Add,
        std::make_unique<TypeModule>(llvmTypeKind::I32), indexValueSSA,
        Operand(1), nextIndexSSA));
    currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I32), nextIndexSSA,
        indexSSA));
    currentBlock->terminator = std::make_unique<BrInst>(lengthBlock);
    currentBlock = mergeBlock;
    auto comparisonSSA = guard.makeSSA(SSAType::Value);
    std::vector<std::pair<Operand, BasicBlock *>> previousBlocks;
    previousBlocks.emplace_back(std::pair{Operand(0), lengthCheckBlock});
    previousBlocks.emplace_back(std::pair{Operand(0), equalFinalBlock});
    previousBlocks.emplace_back(std::pair{Operand(1), lengthBlock});
    currentBlock->phis.emplace_back(std::make_unique<PhiInst>(
        comparisonSSA, std::make_unique<TypeModule>(llvmTypeKind::I1),
        std::move(previousBlocks)));
    currentBlock->terminator = std::make_unique<ReturnInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I1), comparisonSSA);
    break;
  }
  case TypeKind::Array: {
    auto leftPlace = temporaryMaterialize(left);
    auto rightPlace = temporaryMaterialize(right);
    auto length = std::get<long long>(compareType.arrayLen.value().value);
    auto indexSSA = guard.makeSSA(SSAType::Addr);
    currentBlock->insts.emplace_back(std::make_unique<AllocaInst>(
        indexSSA, std::make_unique<TypeModule>(llvmTypeKind::I32)));
    currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I32), Operand(0), indexSSA));
    auto lengthBlockName = guard.generateBasicBlockName();
    currentFunction->basicBlocks.emplace(
        lengthBlockName, std::make_unique<BasicBlock>(lengthBlockName));
    auto *lengthBlock = currentFunction->basicBlocks.at(lengthBlockName).get();
    currentBlock->terminator = std::make_unique<BrInst>(lengthBlock);
    currentBlock = lengthBlock;
    auto lengthLimitSSA = guard.makeSSA(SSAType::Value);
    auto indexValueSSA = guard.makeSSA(SSAType::Value);
    currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I32), indexSSA,
        indexValueSSA));
    currentBlock->insts.emplace_back(std::make_unique<ICmpInst>(
        ICmpOperator::Ult, std::make_unique<TypeModule>(llvmTypeKind::I32),
        indexValueSSA, Operand(length), lengthLimitSSA));
    auto equalBlockName = guard.generateBasicBlockName();
    currentFunction->basicBlocks.emplace(
        equalBlockName, std::make_unique<BasicBlock>(equalBlockName));
    auto *equalBlock = currentFunction->basicBlocks.at(equalBlockName).get();
    auto mergeBlockName = guard.generateBasicBlockName();
    currentFunction->basicBlocks.emplace(
        mergeBlockName, std::make_unique<BasicBlock>(mergeBlockName));
    auto *mergeBlock = currentFunction->basicBlocks.at(mergeBlockName).get();
    currentBlock->terminator =
        std::make_unique<CondInst>(lengthLimitSSA, equalBlock, mergeBlock);
    currentBlock = equalBlock;
    auto leftChildOperand = guard.makeSSA(SSAType::Value);
    auto rightChildOperand = guard.makeSSA(SSAType::Value);
    {
      auto leftChildAddr = guard.makeSSA(SSAType::Addr);
      auto arrayType = std::make_unique<TypeModule>(
          TypeModule::lowerType(&compareType, false));
      auto innerType = std::make_unique<TypeModule>(
          TypeModule::lowerType(compareType.elementType.get(), false));
      currentBlock->insts.emplace_back(std::make_unique<GEPInst>(
          leftChildAddr, std::move(arrayType), leftPlace,
          std::vector<Operand>{0, indexValueSSA}));
      currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
          std::move(innerType), leftChildAddr, leftChildOperand));
    }
    {
      auto rightChildAddr = guard.makeSSA(SSAType::Addr);
      auto arrayType = std::make_unique<TypeModule>(
          TypeModule::lowerType(&compareType, false));
      auto innerType = std::make_unique<TypeModule>(
          TypeModule::lowerType(compareType.elementType.get(), false));
      currentBlock->insts.emplace_back(std::make_unique<GEPInst>(
          rightChildAddr, std::move(arrayType), rightPlace,
          std::vector<Operand>{0, indexValueSSA}));
      currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
          std::move(innerType), rightChildAddr, rightChildOperand));
    }
    auto equalLimitSSA = guard.makeSSA(SSAType::Value);
    std::vector<std::pair<std::unique_ptr<TypeModule>, Operand>> callParams;
    callParams.emplace_back(
        std::pair{std::make_unique<TypeModule>(TypeModule::lowerType(
                      compareType.elementType.get(), false)),
                  leftChildOperand});
    callParams.emplace_back(
        std::pair{std::make_unique<TypeModule>(TypeModule::lowerType(
                      compareType.elementType.get(), false)),
                  rightChildOperand});
    currentBlock->insts.emplace_back(std::make_unique<CallInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I1), equalLimitSSA,
        SemanticAnalyzer::encodeType(*compareType.elementType) + "_equal",
        std::move(callParams)));
    auto bodyBlockName = guard.generateBasicBlockName();
    currentFunction->basicBlocks.emplace(
        bodyBlockName, std::make_unique<BasicBlock>(bodyBlockName));
    auto *bodyBlock = currentFunction->basicBlocks.at(bodyBlockName).get();
    auto equalFinalBlock = currentBlock;
    currentBlock->terminator =
        std::make_unique<CondInst>(equalLimitSSA, bodyBlock, mergeBlock);
    currentBlock = bodyBlock;
    auto nextIndexSSA = guard.makeSSA(SSAType::Value);
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        BinaryIntergerOperator::Add,
        std::make_unique<TypeModule>(llvmTypeKind::I32), indexValueSSA,
        Operand(1), nextIndexSSA));
    currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I32), nextIndexSSA,
        indexSSA));
    currentBlock->terminator = std::make_unique<BrInst>(lengthBlock);
    currentBlock = mergeBlock;
    auto comparisonSSA = guard.makeSSA(SSAType::Value);
    std::vector<std::pair<Operand, BasicBlock *>> previousBlocks;
    previousBlocks.emplace_back(std::pair{Operand(0), equalFinalBlock});
    previousBlocks.emplace_back(std::pair{Operand(1), lengthBlock});
    currentBlock->phis.emplace_back(std::make_unique<PhiInst>(
        comparisonSSA, std::make_unique<TypeModule>(llvmTypeKind::I1),
        std::move(previousBlocks)));
    currentBlock->terminator = std::make_unique<ReturnInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I1), comparisonSSA);
    break;
  }
  case TypeKind::Struct: {
    auto *symbol = modules->analyzer->globalScopeVisitor->lookupStruct(
        *compareType.structName);
    if (symbol->layoutFields.empty()) {
      auto comparisonSSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<ICmpInst>(
          ICmpOperator::Eq, std::make_unique<TypeModule>(llvmTypeKind::I1),
          Operand(0), Operand(0), comparisonSSA));
      currentBlock->terminator = std::make_unique<ReturnInst>(
          std::make_unique<TypeModule>(llvmTypeKind::I1), comparisonSSA);
      break;
    }
    auto mergeBlockName = guard.generateBasicBlockName();
    currentFunction->basicBlocks.emplace(
        mergeBlockName, std::make_unique<BasicBlock>(mergeBlockName));
    auto *mergeBlock = currentFunction->basicBlocks.at(mergeBlockName).get();
    std::vector<std::pair<Operand, BasicBlock *>> previousBlocks;
    for (std::size_t i = 0; i < symbol->layoutFields.size(); ++i) {
      auto leftChildOperand = guard.makeSSA(SSAType::Value);
      auto rightChildOperand = guard.makeSSA(SSAType::Value);
      auto aggregateType = TypeModule::lowerType(&compareType, false);
      currentBlock->insts.emplace_back(std::make_unique<ExtractvalueInst>(
          leftChildOperand, std::make_unique<TypeModule>(aggregateType), left,
          std::vector<std::size_t>{i}));
      currentBlock->insts.emplace_back(std::make_unique<ExtractvalueInst>(
          rightChildOperand, std::make_unique<TypeModule>(aggregateType), right,
          std::vector<std::size_t>{i}));
      const auto &fieldType = symbol->structFields.at(symbol->layoutFields[i]);
      auto equalLimitSSA = guard.makeSSA(SSAType::Value);
      std::vector<std::pair<std::unique_ptr<TypeModule>, Operand>> callParams;
      callParams.emplace_back(
          std::pair{std::make_unique<TypeModule>(TypeModule::lowerType(
                        &fieldType, false)),
                    leftChildOperand});
      callParams.emplace_back(
          std::pair{std::make_unique<TypeModule>(TypeModule::lowerType(
                        &fieldType, false)),
                    rightChildOperand});
      currentBlock->insts.emplace_back(std::make_unique<CallInst>(
          std::make_unique<TypeModule>(llvmTypeKind::I1), equalLimitSSA,
          SemanticAnalyzer::encodeType(fieldType) + "_equal",
          std::move(callParams)));
      if (i + 1 == symbol->layoutFields.size()) {
        previousBlocks.emplace_back(equalLimitSSA, currentBlock);
        currentBlock->terminator = std::make_unique<BrInst>(mergeBlock);
      } else {
        auto nextBlockName = guard.generateBasicBlockName();
        currentFunction->basicBlocks.emplace(
            nextBlockName, std::make_unique<BasicBlock>(nextBlockName));
        auto *nextBlock = currentFunction->basicBlocks.at(nextBlockName).get();
        previousBlocks.emplace_back(Operand(0), currentBlock);
        currentBlock->terminator =
            std::make_unique<CondInst>(equalLimitSSA, nextBlock, mergeBlock);
        currentBlock = nextBlock;
      }
    }
    currentBlock = mergeBlock;
    auto comparisonSSA = guard.makeSSA(SSAType::Value);
    currentBlock->phis.emplace_back(std::make_unique<PhiInst>(
        comparisonSSA, std::make_unique<TypeModule>(llvmTypeKind::I1),
        std::move(previousBlocks)));
    currentBlock->terminator = std::make_unique<ReturnInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I1), comparisonSSA);
    break;
  }
  case TypeKind::Box:
  case TypeKind::MutReference:
  case TypeKind::ImmutReference: {
    auto leftValue = guard.makeSSA(SSAType::Value);
    {
      auto _ = std::make_unique<TypeModule>(
          TypeModule::lowerType(compareType.elementType.get(), false));
      currentBlock->insts.emplace_back(
          std::make_unique<LoadInst>(std::move(_), left, leftValue));
    }
    auto rightValue = guard.makeSSA(SSAType::Value);
    {
      auto _ = std::make_unique<TypeModule>(
          TypeModule::lowerType(compareType.elementType.get(), false));
      currentBlock->insts.emplace_back(
          std::make_unique<LoadInst>(std::move(_), right, rightValue));
    }
    auto equalLimitSSA = guard.makeSSA(SSAType::Value);
    std::vector<std::pair<std::unique_ptr<TypeModule>, Operand>> callParams;
    callParams.emplace_back(
        std::pair{std::make_unique<TypeModule>(TypeModule::lowerType(
                      compareType.elementType.get(), false)),
                  leftValue});
    callParams.emplace_back(
        std::pair{std::make_unique<TypeModule>(TypeModule::lowerType(
                      compareType.elementType.get(), false)),
                  rightValue});
    currentBlock->insts.emplace_back(std::make_unique<CallInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I1), equalLimitSSA,
        SemanticAnalyzer::encodeType(*compareType.elementType) + "_equal",
        std::move(callParams)));
    currentBlock->terminator = std::make_unique<ReturnInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I1), equalLimitSSA);
    break;
  }
  default:
    std::unreachable();
  }
  modules->functions.emplace_back(std::move(equalFunction));
}

void IRGenerator::registerEqualFunction(const std::string &manglingName,
                                        const Type &type) {
  if (pendingEqualFunctions.contains(manglingName)) {
    return;
  }
  // Register before following dependencies so recursive types terminate here.
  pendingEqualFunctions.emplace(manglingName, type);
  if (type.kind == TypeKind::Struct) {
    auto *symbol = modules->analyzer->globalScopeVisitor->lookupStruct(
        type.structName.value());
    for (const auto &[name, entryType] : symbol->structFields) {
      auto mangling = SemanticAnalyzer::encodeType(entryType);
      registerEqualFunction(mangling, entryType);
    }
  } else if (type.kind == TypeKind::Array || type.kind == TypeKind::Vec ||
             type.kind == TypeKind::Box ||
             type.kind == TypeKind::ImmutReference ||
             type.kind == TypeKind::MutReference) {
    auto mangling = SemanticAnalyzer::encodeType(*type.elementType);
    registerEqualFunction(mangling, *type.elementType);
  }
}

std::any IRGenerator::visitComparisonExpr(ComparisonExprNode *node) {
  auto leftOperand = emitValue(node->leftChildExpr.get());
  if (!leftOperand) {
    return std::optional<Operand>{};
  }
  auto rightOperand = emitValue(node->rightChildExpr.get());
  if (!rightOperand) {
    return std::optional<Operand>{};
  }
  auto valueSSA = guard.makeSSA(SSAType::Value);
  auto emitComparison = [&](ICmpOperator signedOp, ICmpOperator unsignedOp) {
    const auto &operandType = node->leftChildExpr->semantic->adjustedType;
    auto type = std::make_unique<TypeModule>(helper::isBooleanType(operandType)
                                                 ? llvmTypeKind::I8
                                                 : llvmTypeKind::I32);
    currentBlock->insts.emplace_back(std::make_unique<ICmpInst>(
        helper::isSignedType(operandType) ? signedOp : unsignedOp,
        std::move(type), *leftOperand, *rightOperand, valueSSA));
  };
  switch (node->op) {
  case ComparisonOperator::equal: {
    const Type &compareType = node->leftChildExpr->semantic->adjustedType;
    auto mangling = SemanticAnalyzer::encodeType(compareType);
    registerEqualFunction(mangling, compareType);
    std::vector<std::pair<std::unique_ptr<TypeModule>, Operand>> callParams;
    callParams.emplace_back(
        std::pair{std::make_unique<TypeModule>(TypeModule::lowerType(
                      &compareType, false)),
                  *leftOperand});
    callParams.emplace_back(
        std::pair{std::make_unique<TypeModule>(TypeModule::lowerType(
                      &compareType, false)),
                  *rightOperand});
    currentBlock->insts.emplace_back(std::make_unique<CallInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I1), valueSSA,
        mangling + "_equal",
        std::move(callParams)));
    break;
  }
  case ComparisonOperator::notEqual: {
    const Type &compareType = node->leftChildExpr->semantic->adjustedType;
    auto mangling = SemanticAnalyzer::encodeType(compareType);
    registerEqualFunction(mangling, compareType);
    auto equalSSA = guard.makeSSA(SSAType::Value);
    std::vector<std::pair<std::unique_ptr<TypeModule>, Operand>> callParams;
    callParams.emplace_back(
        std::pair{std::make_unique<TypeModule>(TypeModule::lowerType(
                      &compareType, false)),
                  *leftOperand});
    callParams.emplace_back(
        std::pair{std::make_unique<TypeModule>(TypeModule::lowerType(
                      &compareType, false)),
                  *rightOperand});
    currentBlock->insts.emplace_back(std::make_unique<CallInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I1), equalSSA,
        mangling + "_equal",
        std::move(callParams)));
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        BinaryIntergerOperator::Xor,
        std::make_unique<TypeModule>(llvmTypeKind::I1), equalSSA, Operand(1),
        valueSSA));
    break;
  }
  case ComparisonOperator::greaterThan:
    emitComparison(ICmpOperator::Sgt, ICmpOperator::Ugt);
    break;
  case ComparisonOperator::lessThan:
    emitComparison(ICmpOperator::Slt, ICmpOperator::Ult);
    break;
  case ComparisonOperator::greaterThanOrEqual:
    emitComparison(ICmpOperator::Sge, ICmpOperator::Uge);
    break;
  case ComparisonOperator::lessThanOrEqual:
    emitComparison(ICmpOperator::Sle, ICmpOperator::Ule);
    break;
  }
  auto fromType = std::make_unique<TypeModule>(llvmTypeKind::I1);
  auto toType = std::make_unique<TypeModule>(llvmTypeKind::I8);
  auto storeSSA = guard.makeSSA(SSAType::Value);
  currentBlock->insts.emplace_back(std::make_unique<CastInst>(
      CastOperator::ZExt, std::move(fromType), std::move(toType),
      Operand(valueSSA), storeSSA));
  return std::optional<Operand>{Operand(storeSSA)};
}

std::any IRGenerator::visitLogicExpr(LogicExprNode *node) {
  auto leftOperand = emitValue(node->leftChildExpr.get());
  if (!leftOperand) {
    return std::optional<Operand>{};
  }
  auto originTrueType = std::make_unique<TypeModule>(llvmTypeKind::I8);
  auto targetTrueType = std::make_unique<TypeModule>(llvmTypeKind::I1);
  auto originCond = guard.makeSSA(SSAType::Value);
  currentBlock->insts.emplace_back(std::make_unique<CastInst>(
      CastOperator::Trunc, std::move(originTrueType), std::move(targetTrueType),
      *leftOperand, originCond));

  auto mergeBlockName = guard.generateBasicBlockName();
  currentFunction->basicBlocks.emplace(
      mergeBlockName, std::make_unique<BasicBlock>(mergeBlockName));
  auto *mergeBlock = currentFunction->basicBlocks.at(mergeBlockName).get();
  auto *shortCircuitBlock = currentBlock;
  auto rightBlockName = guard.generateBasicBlockName();
  currentFunction->basicBlocks.emplace(
      rightBlockName, std::make_unique<BasicBlock>(rightBlockName));
  auto *rightBlock = currentFunction->basicBlocks.at(rightBlockName).get();
  if (node->op == LogicOperator::LogicAnd) {
    currentBlock->terminator =
        std::make_unique<CondInst>(originCond, rightBlock, mergeBlock);
  } else {
    currentBlock->terminator =
        std::make_unique<CondInst>(originCond, mergeBlock, rightBlock);
  }

  currentBlock = rightBlock;
  auto rightOperand = emitValue(node->rightChildExpr.get());
  if (rightOperand) {
    currentBlock->terminator = std::make_unique<BrInst>(mergeBlock);
  }
  auto rightMergeBlock = currentBlock;
  currentBlock = mergeBlock;
  auto mergeSSA = guard.makeSSA(SSAType::Value);
  auto _ = std::make_unique<TypeModule>(llvmTypeKind::I8);
  if (rightOperand) {
    currentBlock->phis.emplace_back(
        std::make_unique<PhiInst>(mergeSSA, std::move(_),
                                  std::vector<std::pair<Operand, BasicBlock *>>{
                                      {*leftOperand, shortCircuitBlock},
                                      {*rightOperand, rightMergeBlock},
                                  }));
  } else {
    currentBlock->phis.emplace_back(
        std::make_unique<PhiInst>(mergeSSA, std::move(_),
                                  std::vector<std::pair<Operand, BasicBlock *>>{
                                      {*leftOperand, shortCircuitBlock}}));
  }
  return std::optional<Operand>{Operand(mergeSSA)};
}

std::any IRGenerator::visitCastExpr(CastExprNode *node) {
  auto operand = emitValue(node->castValue.get());
  if (!operand) {
    return std::optional<Operand>{};
  }
  if (SemanticAnalyzer::isIntegerType(
          node->castValue->semantic->adjustedType)) {
    return operand;
  }
  auto fromType = std::make_unique<TypeModule>(llvmTypeKind::I8);
  auto toType = std::make_unique<TypeModule>(llvmTypeKind::I32);
  auto valueSSA = guard.makeSSA(SSAType::Value);
  currentBlock->insts.emplace_back(
      std::make_unique<CastInst>(CastOperator::ZExt, std::move(fromType),
                                 std::move(toType), *operand, valueSSA));
  return std::optional<Operand>{Operand(valueSSA)};
}

std::any IRGenerator::visitBorrowExpr(BorrowExprNode *node) {
  return emitPlace(node->borrowExpr.get());
} // value, return the pointer which point to the inner variable

std::any IRGenerator::visitDereferenceExpr(DereferenceExprNode *node) {
  return emitValue(node->dereferenceExpr.get());
} // place, return the pointer value

std::any IRGenerator::visitAssignmentExpr(AssignmentExprNode *node) {
  auto rightOperand = emitValue(node->rightChildExpr.get());
  if (!rightOperand) {
    return std::optional<Operand>{};
  }
  auto leftOperand = emitPlace(node->leftChildExpr.get());
  if (!leftOperand) {
    return std::optional<Operand>{};
  }
  auto *leftType = &node->leftChildExpr->semantic->adjustedType;
  auto emitCompoundAssignment = [&](BinaryIntergerOperator op,
                                    std::unique_ptr<TypeModule> &&binaryType,
                                    bool maskShiftCount) {
    auto intermediateSSA = guard.makeSSA(SSAType::Value);
    auto loadType =
        std::make_unique<TypeModule>(TypeModule::lowerType(leftType, false));
    currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
        std::move(loadType), Operand::SSA(*leftOperand), intermediateSSA));

    if (maskShiftCount) {
      auto countSSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
          BinaryIntergerOperator::And,
          std::make_unique<TypeModule>(llvmTypeKind::I32), *rightOperand,
          Operand(Operand::Immediate(31ll)), countSSA));
      rightOperand = Operand(countSSA);
    }

    auto resultSSA = guard.makeSSA(SSAType::Value);
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        op, std::move(binaryType), Operand(intermediateSSA), *rightOperand,
        resultSSA));

    auto storeType =
        std::make_unique<TypeModule>(TypeModule::lowerType(leftType, false));
    currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
        std::move(storeType), Operand(resultSSA), Operand::SSA(*leftOperand)));
  };

  switch (node->assignmentOp) {
  case AssignmentOperator::Assignment: {
    auto storeType =
        std::make_unique<TypeModule>(TypeModule::lowerType(leftType, false));
    currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
        std::move(storeType), *rightOperand, Operand::SSA(*leftOperand)));
    break;
  }
  case AssignmentOperator::additionAssignment: {
    emitCompoundAssignment(BinaryIntergerOperator::Add,
                           std::make_unique<TypeModule>(llvmTypeKind::I32),
                           false);
    break;
  }
  case AssignmentOperator::subtractionAssignment: {
    emitCompoundAssignment(BinaryIntergerOperator::Sub,
                           std::make_unique<TypeModule>(llvmTypeKind::I32),
                           false);
    break;
  }
  case AssignmentOperator::multiplicationAssignment: {
    emitCompoundAssignment(BinaryIntergerOperator::Mul,
                           std::make_unique<TypeModule>(llvmTypeKind::I32),
                           false);
    break;
  }
  case AssignmentOperator::divisionAssignment: {
    auto type =
        std::make_unique<TypeModule>(TypeModule::lowerType(leftType, false));
    auto op = type->isSigned ? BinaryIntergerOperator::SDiv
                             : BinaryIntergerOperator::UDiv;
    emitCompoundAssignment(op, std::move(type), false);
    break;
  }
  case AssignmentOperator::moduloAssignment: {
    auto type =
        std::make_unique<TypeModule>(TypeModule::lowerType(leftType, false));
    auto op = type->isSigned ? BinaryIntergerOperator::SRem
                             : BinaryIntergerOperator::URem;
    emitCompoundAssignment(op, std::move(type), false);
    break;
  }
  case AssignmentOperator::ANDAssignment: {
    emitCompoundAssignment(
        BinaryIntergerOperator::And,
        std::make_unique<TypeModule>(TypeModule::lowerType(leftType, false)),
        false);
    break;
  }
  case AssignmentOperator::ORAssignment: {
    emitCompoundAssignment(
        BinaryIntergerOperator::Or,
        std::make_unique<TypeModule>(TypeModule::lowerType(leftType, false)),
        false);
    break;
  }
  case AssignmentOperator::XORAssignment: {
    emitCompoundAssignment(
        BinaryIntergerOperator::Xor,
        std::make_unique<TypeModule>(TypeModule::lowerType(leftType, false)),
        false);
    break;
  }
  case AssignmentOperator::leftShiftAssignment: {
    emitCompoundAssignment(
        BinaryIntergerOperator::Shl,
        std::make_unique<TypeModule>(TypeModule::lowerType(leftType, false)),
        true);
    break;
  }
  case AssignmentOperator::rightShiftAssignment: {
    auto op = helper::isSignedType(*leftType) ? BinaryIntergerOperator::AShr
                                              : BinaryIntergerOperator::LShr;
    emitCompoundAssignment(
        op,
        std::make_unique<TypeModule>(TypeModule::lowerType(leftType, false)),
        true);
    break;
  }
  }
  return std::optional<Operand>{Operand(Operand::Unit{})};
}

std::any IRGenerator::visitConstExpr(ConstExprNode *node) {

  auto *constantSemantic =
      dynamic_cast<ConstExprSemantic *>(node->semantic.get());
  Operand::Immediate imm(constantSemantic->value);
  return std::optional<Operand>{Operand(imm)};
}

std::any IRGenerator::visitArrayExpr(ArrayExprNode *node) {
  if (std::holds_alternative<std::vector<std::unique_ptr<ExprNode>>>(
          node->arrayEntries)) {
    const auto &arrayEntries =
        std::get<std::vector<std::unique_ptr<ExprNode>>>(node->arrayEntries);
    std::size_t index = 0;
    Operand::SSA lastArraySSA("poison");
    for (const auto &entry : arrayEntries) {
      auto entryOperand = emitValue(entry.get());
      if (!entryOperand) {
        return std::optional<Operand>{};
      }
      auto resultSSA = guard.makeSSA(SSAType::Value);
      auto resultType = std::make_unique<TypeModule>(
          TypeModule::lowerType(&node->semantic->originalType, false));
      auto elementType = std::make_unique<TypeModule>(
          TypeModule::lowerType(&entry->semantic->adjustedType, false));
      currentBlock->insts.emplace_back(std::make_unique<InsertvalueInst>(
          resultSSA, std::move(resultType), lastArraySSA,
          std::move(elementType), *entryOperand,
          std::vector<std::size_t>{index}));
      lastArraySSA = resultSSA;
      ++index;
    }
    return std::optional<Operand>{Operand(lastArraySSA)};
  } else {
    const auto &arrayEntries = std::get<
        std::pair<std::unique_ptr<ExprNode>, std::unique_ptr<ConstExprNode>>>(
        node->arrayEntries);
    auto entryOperand = emitValue(arrayEntries.first.get());
    if (!entryOperand) {
      return std::optional<Operand>{};
    }
    auto constSemantic =
        dynamic_cast<ConstExprSemantic *>(arrayEntries.second->semantic.get());
    if (constSemantic->value == 0) {
      return std::optional<Operand>{Operand(Operand::SSA("poison"))};
    }
    auto arrayType =
        TypeModule::lowerType(&node->semantic->originalType, false);
    auto arraySSA = guard.makeSSA(SSAType::Addr);
    currentBlock->insts.emplace_back(std::make_unique<AllocaInst>(
        arraySSA, std::make_unique<TypeModule>(arrayType)));
    auto indexSSA = guard.makeSSA(SSAType::Addr);
    currentBlock->insts.emplace_back(std::make_unique<AllocaInst>(
        indexSSA, std::make_unique<TypeModule>(llvmTypeKind::I32)));
    currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I32), Operand(0), indexSSA));

    auto condBlockName = guard.generateBasicBlockName();
    currentFunction->basicBlocks.emplace(
        condBlockName, std::make_unique<BasicBlock>(condBlockName));
    auto *condBlock = currentFunction->basicBlocks.at(condBlockName).get();
    auto bodyBlockName = guard.generateBasicBlockName();
    currentFunction->basicBlocks.emplace(
        bodyBlockName, std::make_unique<BasicBlock>(bodyBlockName));
    auto *bodyBlock = currentFunction->basicBlocks.at(bodyBlockName).get();
    auto exitBlockName = guard.generateBasicBlockName();
    currentFunction->basicBlocks.emplace(
        exitBlockName, std::make_unique<BasicBlock>(exitBlockName));
    auto *exitBlock = currentFunction->basicBlocks.at(exitBlockName).get();
    currentBlock->terminator = std::make_unique<BrInst>(condBlock);
    currentBlock = condBlock;
    auto indexValueSSA = guard.makeSSA(SSAType::Value);
    currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I32), indexSSA,
        indexValueSSA));
    auto comparisonSSA = guard.makeSSA(SSAType::Value);
    currentBlock->insts.emplace_back(std::make_unique<ICmpInst>(
        ICmpOperator::Ult, std::make_unique<TypeModule>(llvmTypeKind::I32),
        indexValueSSA, Operand(constSemantic->value), comparisonSSA));
    currentBlock->terminator =
        std::make_unique<CondInst>(comparisonSSA, bodyBlock, exitBlock);

    currentBlock = bodyBlock;
    auto elementSSA = guard.makeSSA(SSAType::Addr);
    currentBlock->insts.emplace_back(std::make_unique<GEPInst>(
        elementSSA, std::make_unique<TypeModule>(arrayType), arraySSA,
        std::vector<Operand>{Operand(0), indexValueSSA}));
    currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
        std::make_unique<TypeModule>(*arrayType.innerType), *entryOperand,
        elementSSA));
    auto nextIndexSSA = guard.makeSSA(SSAType::Value);
    currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
        BinaryIntergerOperator::Add,
        std::make_unique<TypeModule>(llvmTypeKind::I32), indexValueSSA,
        Operand(1), nextIndexSSA));
    currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
        std::make_unique<TypeModule>(llvmTypeKind::I32), nextIndexSSA,
        indexSSA));
    currentBlock->terminator = std::make_unique<BrInst>(condBlock);

    currentBlock = exitBlock;
    auto resultSSA = guard.makeSSA(SSAType::Value);
    currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
        std::make_unique<TypeModule>(arrayType), arraySSA, resultSSA));
    return std::optional<Operand>{Operand(resultSSA)};
  }
}

std::any IRGenerator::visitIndexExpr(IndexExprNode *node) {
  auto baseSSA = emitPlace(node->arrayExpr.get());
  if (!baseSSA) {
    return std::optional<Operand>{};
  }

  auto indexSSA = emitValue(node->indexExpr.get());
  if (!indexSSA) {
    return std::optional<Operand>{};
  }
  if (node->arrayExpr->semantic->adjustedType.kind == TypeKind::Array) {
    auto resultSSA = guard.makeSSA(SSAType::Addr);
    auto arrayType = std::make_unique<TypeModule>(
        TypeModule::lowerType(&node->arrayExpr->semantic->adjustedType, false));
    currentBlock->insts.emplace_back(
        std::make_unique<GEPInst>(resultSSA, std::move(arrayType), *baseSSA,
                                  std::vector<Operand>{Operand(0), *indexSSA}));
    return std::optional<Operand>{resultSSA};
  } else {
    auto ptrSSA = guard.makeSSA(SSAType::Addr);
    auto vecModule = std::make_unique<TypeModule>(llvmTypeKind::Struct);
    vecModule->structName = "builtin_Vec";
    currentBlock->insts.emplace_back(std::make_unique<GEPInst>(
        ptrSSA, std::move(vecModule), *baseSSA,
        std::vector<Operand>{Operand(0), Operand(0)}));
    auto dataPtrSSA = guard.makeSSA(SSAType::Addr);
    currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
        std::make_unique<TypeModule>(llvmTypeKind::Ptr), ptrSSA, dataPtrSSA));
    auto resultSSA = guard.makeSSA(SSAType::Addr);
    auto innerType = std::make_unique<TypeModule>(TypeModule::lowerType(
        node->arrayExpr->semantic->adjustedType.elementType.get(), false));
    currentBlock->insts.emplace_back(
        std::make_unique<GEPInst>(resultSSA, std::move(innerType), dataPtrSSA,
                                  std::vector<Operand>{*indexSSA}));
    return std::optional<Operand>{resultSSA};
  }
}

std::any IRGenerator::visitStructExpr(StructExprNode *node) {
  auto *structSemantic =
      dynamic_cast<StructPathInExprSemantic *>(node->path->semantic.get());
  auto *symbol = structSemantic->structSymbol;
  Operand::SSA lastStructSSA("poison");
  for (const auto &field : node->structExprFields) {
    auto fieldOperand = emitValue(field.fieldExpr.get());
    if (!fieldOperand) {
      return std::optional<Operand>{};
    }
    auto resultSSA = guard.makeSSA(SSAType::Value);
    auto resultType = std::make_unique<TypeModule>(
        TypeModule::lowerType(&node->semantic->originalType, false));
    auto elementType = std::make_unique<TypeModule>(
        TypeModule::lowerType(&field.fieldExpr->semantic->adjustedType, false));
    currentBlock->insts.emplace_back(std::make_unique<InsertvalueInst>(
        resultSSA, std::move(resultType), lastStructSSA, std::move(elementType),
        *fieldOperand,
        std::vector<std::size_t>{symbol->fieldIndices.at(field.identifier)}));
    lastStructSSA = resultSSA;
  }
  return std::optional<Operand>{Operand(lastStructSSA)};
}

std::any IRGenerator::visitCallExpr(CallExprNode *node) {
  auto *semantic = static_cast<CallExprSemantic *>(node->semantic.get());
  auto *function = getOrCreateFunction(semantic->calleeSymbol);
  const auto &funcName = function->functionName;
  auto resultType = std::make_unique<TypeModule>(*function->returnType);
  std::vector<std::pair<std::unique_ptr<TypeModule>, Operand>> params;
  params.reserve(node->callParams.size());
  for (const auto &paramNode : node->callParams) {
    auto paramOperand = emitValue(paramNode.get());
    if (!paramOperand) {
      return std::optional<Operand>{};
    }
    params.emplace_back(std::make_unique<TypeModule>(TypeModule::lowerType(
                            &paramNode->semantic->adjustedType, false)),
                        *paramOperand);
  }
  if (resultType->kind != llvmTypeKind::Void) {
    auto resultSSA = guard.makeSSA(SSAType::Value);
    currentBlock->insts.emplace_back(std::make_unique<CallInst>(
        std::move(resultType), resultSSA, funcName, std::move(params)));
    return std::optional<Operand>{Operand(resultSSA)};
  } else {
    currentBlock->insts.emplace_back(std::make_unique<CallInst>(
        std::move(resultType), funcName, std::move(params)));
  }
  return std::optional<Operand>{Operand(Operand::Unit{})};
}

std::any IRGenerator::visitMethodCallExpr(MethodCallExprNode *node) {
  auto *semantic = static_cast<CallExprSemantic *>(node->semantic.get());
  auto *function = getOrCreateFunction(semantic->calleeSymbol);
  const auto &funcName = function->functionName;
  auto resultType = std::make_unique<TypeModule>(*function->returnType);
  std::vector<std::pair<std::unique_ptr<TypeModule>, Operand>> params;
  params.reserve(node->callParams.size() + 1);
  auto selfOperand = emitValue(node->callee.get());
  if (!selfOperand) {
    return std::optional<Operand>{};
  }
  params.emplace_back(std::make_unique<TypeModule>(TypeModule::lowerType(
                          &node->callee->semantic->adjustedType, false)),
                      *selfOperand);
  for (const auto &paramNode : node->callParams) {
    auto paramOperand = emitValue(paramNode.get());
    if (!paramOperand) {
      return std::optional<Operand>{};
    }
    params.emplace_back(std::make_unique<TypeModule>(TypeModule::lowerType(
                            &paramNode->semantic->adjustedType, false)),
                        *paramOperand);
  }
  if (resultType->kind != llvmTypeKind::Void) {
    auto resultSSA = guard.makeSSA(SSAType::Value);
    currentBlock->insts.emplace_back(std::make_unique<CallInst>(
        std::move(resultType), resultSSA, funcName, std::move(params)));
    return std::optional<Operand>{Operand(resultSSA)};
  } else {
    currentBlock->insts.emplace_back(std::make_unique<CallInst>(
        std::move(resultType), funcName, std::move(params)));
  }
  return std::optional<Operand>{Operand(Operand::Unit{})};
}

std::any IRGenerator::visitReturnExpr(ReturnExprNode *node) {
  if (node->returnValue) {
    auto returnValue = emitValue(node->returnValue.value().get());
    if (!returnValue) {
      return std::optional<Operand>{};
    }
    auto returnType = std::make_unique<TypeModule>(TypeModule::lowerType(
        &node->returnValue.value()->semantic->adjustedType, true));
    if (returnType->kind == llvmTypeKind::Void) {
      currentBlock->terminator =
          std::make_unique<ReturnInst>(std::move(returnType));
    } else {
      currentBlock->terminator =
          std::make_unique<ReturnInst>(std::move(returnType), *returnValue);
    }
  } else {
    auto returnType = std::make_unique<TypeModule>(llvmTypeKind::Void);
    currentBlock->terminator =
        std::make_unique<ReturnInst>(std::move(returnType));
  }
  return std::optional<Operand>{};
}

std::any IRGenerator::visitAccessExpr(AccessExprNode *node) {
  auto baseSSA = emitPlace(node->accessObject.get());
  if (!baseSSA) {
    return std::optional<Operand>{};
  }
  const Type &type = node->accessObject->semantic->adjustedType;
  const StructSymbol *structSymbol =
      modules->analyzer->globalScopeVisitor->lookupStruct(*type.structName);
  auto index = structSymbol->fieldIndices.at(node->identifier);
  auto resultSSA = guard.makeSSA(SSAType::Addr);
  auto structType = std::make_unique<TypeModule>(TypeModule::lowerType(
      &node->accessObject->semantic->adjustedType, false));
  currentBlock->insts.emplace_back(std::make_unique<GEPInst>(
      resultSSA, std::move(structType), *baseSSA,
      std::vector<Operand>{Operand(0), Operand(index)}));
  return std::optional<Operand>{resultSSA};
}

std::any IRGenerator::visitContinueExpr(ContinueExprNode *node) {
  currentBlock->terminator =
      std::make_unique<BrInst>(loopContext.back().continueTarget);
  return std::optional<Operand>{};
}

std::any IRGenerator::visitBreakExpr(BreakExprNode *node) {
  auto breakValue = emitValue(node->loopValue.get());
  if (!breakValue) {
    return std::optional<Operand>{};
  }
  if (auto &mergePhi = loopContext.back().phiBlocks) {
    mergePhi->emplace_back(std::pair{*breakValue, currentBlock});
  }
  loopContext.back().willExit = true;
  currentBlock->terminator =
      std::make_unique<BrInst>(loopContext.back().breakTarget);
  return std::optional<Operand>{};
}

// Expressions with blocks.
std::any IRGenerator::visitBlockExpr(BlockExprNode *node) {
  for (auto &statement : node->statements) {
    if (currentBlock->terminator) {
      break;
    }
    statement->accept(this);
  }
  if (currentBlock->terminator) {
    return std::optional<Operand>{};
  }
  if (node->tailExpr) {
    return emitValue(node->tailExpr->get());
  }
  return std::optional<Operand>{Operand(Operand::Unit{})};
}

bool IRGenerator::generateIfExpr(
    IfExprNode *node, BasicBlock *mergeBlock, const Operand &condOperand,
    std::vector<std::pair<Operand, BasicBlock *>> *phiBlocks = nullptr) {
  auto ifBlockName = guard.generateBasicBlockName();
  currentFunction->basicBlocks.emplace(
      ifBlockName, std::make_unique<BasicBlock>(ifBlockName));
  auto condValue = guard.makeSSA(SSAType::Value);
  auto booleanType = std::make_unique<TypeModule>(llvmTypeKind::I8);
  currentBlock->insts.emplace_back(std::make_unique<ICmpInst>(
      ICmpOperator::Ne, std::move(booleanType), condOperand,
      Operand::Immediate(0ll), condValue));
  if (node->elseExpr) {
    auto elseBlockName = guard.generateBasicBlockName();
    currentFunction->basicBlocks.emplace(
        elseBlockName, std::make_unique<BasicBlock>(elseBlockName));
    currentBlock->terminator = std::make_unique<CondInst>(
        condValue, currentFunction->basicBlocks.at(ifBlockName).get(),
        currentFunction->basicBlocks.at(elseBlockName).get());
    currentBlock = currentFunction->basicBlocks.at(ifBlockName).get();
    auto trueResultOperand = emitValue(node->body.get());
    if (trueResultOperand) {
      auto _ = std::make_unique<TypeModule>(
          TypeModule::lowerType(&node->semantic->originalType, false));
      if (phiBlocks) {
        phiBlocks->emplace_back(std::pair{*trueResultOperand, currentBlock});
      }
      currentBlock->terminator = std::make_unique<BrInst>(mergeBlock);
    }

    currentBlock = currentFunction->basicBlocks.at(elseBlockName).get();
    auto falseResultOperand = emitValue(node->elseExpr.value().get());
    if (falseResultOperand) {
      auto _ = std::make_unique<TypeModule>(
          TypeModule::lowerType(&node->semantic->originalType, false));
      if (phiBlocks) {
        phiBlocks->emplace_back(std::pair{*falseResultOperand, currentBlock});
      }
      currentBlock->terminator = std::make_unique<BrInst>(mergeBlock);
    }
    return trueResultOperand || falseResultOperand;
  } else if (node->elseIfExpr) {
    auto intermediateBlockName = guard.generateBasicBlockName();
    currentFunction->basicBlocks.emplace(
        intermediateBlockName,
        std::make_unique<BasicBlock>(intermediateBlockName));
    currentBlock->terminator = std::make_unique<CondInst>(
        condValue, currentFunction->basicBlocks.at(ifBlockName).get(),
        currentFunction->basicBlocks.at(intermediateBlockName).get());
    currentBlock = currentFunction->basicBlocks.at(ifBlockName).get();
    auto resultOperand = emitValue(node->body.get());
    if (resultOperand) {
      auto resultType = std::make_unique<TypeModule>(
          TypeModule::lowerType(&node->semantic->originalType, false));
      if (phiBlocks) {
        phiBlocks->emplace_back(std::pair{*resultOperand, currentBlock});
      }
      currentBlock->terminator = std::make_unique<BrInst>(mergeBlock);
    }
    currentBlock = currentFunction->basicBlocks.at(intermediateBlockName).get();
    auto condOperand = emitValue(node->elseIfExpr.value()->conditions.get());
    if (!resultOperand && !condOperand) {
      return false;
    } else {
      if (condOperand) {
        return generateIfExpr(node->elseIfExpr->get(), mergeBlock, *condOperand,
                              phiBlocks) ||
               resultOperand;
      } else {
        return resultOperand.has_value();
      }
    }
  } else {
    currentBlock->terminator = std::make_unique<CondInst>(
        condValue, currentFunction->basicBlocks.at(ifBlockName).get(),
        mergeBlock);
    currentBlock = currentFunction->basicBlocks.at(ifBlockName).get();
    auto resultOperand = emitValue(node->body.get());
    if (!resultOperand) {
      return true;
    }
    auto resultType = std::make_unique<TypeModule>(
        TypeModule::lowerType(&node->semantic->originalType, false));
    if (phiBlocks) {
      phiBlocks->emplace_back(std::pair{*resultOperand, currentBlock});
    }
    currentBlock->terminator = std::make_unique<BrInst>(mergeBlock);
    currentBlock = mergeBlock;
    return true;
  }
}

std::any IRGenerator::visitIfExpr(IfExprNode *node) {
  auto condOperand = emitValue(node->conditions.get());
  if (!condOperand) {
    return std::optional<Operand>{};
  }
  auto mergeBlockName = guard.generateBasicBlockName();
  currentFunction->basicBlocks.emplace(
      mergeBlockName, std::make_unique<BasicBlock>(mergeBlockName));
  auto *mergeBlock = currentFunction->basicBlocks.at(mergeBlockName).get();
  if (node->semantic->originalType.kind != TypeKind::Unit &&
      node->semantic->originalType.kind != TypeKind::Never) {
    std::vector<std::pair<Operand, BasicBlock *>> phiBlocks;
    if (generateIfExpr(node, mergeBlock, *condOperand, &phiBlocks)) {
      currentBlock = mergeBlock;
      auto resultSSA = guard.makeSSA(SSAType::Value);
      auto _ = std::make_unique<TypeModule>(
          TypeModule::lowerType(&node->semantic->originalType, false));
      currentBlock->phis.emplace_back(std::make_unique<PhiInst>(
          resultSSA, std::move(_), std::move(phiBlocks)));
      return std::optional<Operand>{Operand(resultSSA)};
    } else {
      currentFunction->basicBlocks.erase(mergeBlockName);
      return std::optional<Operand>{};
    }
  } else {
    if (generateIfExpr(node, mergeBlock, *condOperand)) {
      currentBlock = mergeBlock;
      return std::optional<Operand>{Operand(Operand::Unit{})};
    } else {
      currentFunction->basicBlocks.erase(mergeBlockName);
      return std::optional<Operand>{};
    }
  }
}

std::any IRGenerator::visitInfiniteLoopExpr(InfiniteLoopExprNode *node) {
  auto bodyBlockName = guard.generateBasicBlockName();
  currentFunction->basicBlocks.emplace(
      bodyBlockName, std::make_unique<BasicBlock>(bodyBlockName));
  auto *bodyBlock = currentFunction->basicBlocks.at(bodyBlockName).get();
  auto exitBlockName = guard.generateBasicBlockName();
  currentFunction->basicBlocks.emplace(
      exitBlockName, std::make_unique<BasicBlock>(exitBlockName));
  auto *exitBlock = currentFunction->basicBlocks.at(exitBlockName).get();
  if (node->semantic->originalType.kind != TypeKind::Unit &&
      node->semantic->originalType.kind != TypeKind::Never) {
    loopContext.emplace_back(LoopContextRecord{
        bodyBlock, exitBlock, std::vector<std::pair<Operand, BasicBlock *>>{}});
  } else {
    loopContext.emplace_back(LoopContextRecord{bodyBlock, exitBlock});
  }
  currentBlock->terminator = std::make_unique<BrInst>(bodyBlock);
  currentBlock = bodyBlock;
  auto blockValue = emitValue(node->body.get());
  if (blockValue) {
    currentBlock->terminator = std::make_unique<BrInst>(bodyBlock);
  }
  if (loopContext.back().willExit) {
    currentBlock = exitBlock;
    if (loopContext.back().phiBlocks) {
      auto resultValue = guard.makeSSA(SSAType::Value);
      auto _ = std::make_unique<TypeModule>(
          TypeModule::lowerType(&node->semantic->originalType, false));
      currentBlock->phis.emplace_back(std::make_unique<PhiInst>(
          resultValue, std::move(_), std::move(*loopContext.back().phiBlocks)));
      loopContext.pop_back();
      return std::optional<Operand>(resultValue);
    } else if (node->semantic->originalType.kind == TypeKind::Unit) {
      loopContext.pop_back();
      return std::optional<Operand>(Operand::Unit{});
    }
  } else {
    currentFunction->basicBlocks.erase(exitBlockName);
  }
  loopContext.pop_back();
  return std::optional<Operand>{};
}

TypeLayout IRGenerator::layout(const TypeModule &type) {
  switch (type.kind) {
  case llvmTypeKind::I32:
    return TypeLayout{4, 4};
  case llvmTypeKind::I8:
    return TypeLayout{1, 1};
  case llvmTypeKind::Ptr:
    return TypeLayout{4, 4};
  case llvmTypeKind::Array: {
    auto innerLayout = layout(*type.innerType);
    std::size_t size = innerLayout.size * type.length.value();
    std::size_t alignment = innerLayout.alignment;
    return TypeLayout{size, alignment};
  }
  case llvmTypeKind::Struct: {
    auto *module = modules->structModuleMap.at(type.structName.value());
    std::size_t size = 0;
    std::size_t alignment = 1;
    for (const auto &field : module->fields) {
      const auto fieldLayout = layout(*field);
      alignment = std::max(alignment, fieldLayout.alignment);
      size = (size + fieldLayout.alignment - 1) / fieldLayout.alignment *
             fieldLayout.alignment;
      size += fieldLayout.size;
    }
    size = (size + alignment - 1) / alignment * alignment;
    return TypeLayout{size, alignment};
  }
  default:
    std::unreachable();
  }
}

Operand::SSA IRGenerator::emitAlloc(const Operand &size,
                                    const Operand &alignment) {
  auto addrSSA = guard.makeSSA(SSAType::Addr);
  std::vector<std::pair<std::unique_ptr<TypeModule>, Operand>> params;
  params.emplace_back(std::make_unique<TypeModule>(llvmTypeKind::I32), size);
  params.emplace_back(std::make_unique<TypeModule>(llvmTypeKind::I32),
                      alignment);
  currentBlock->insts.emplace_back(std::make_unique<CallInst>(
      std::make_unique<TypeModule>(llvmTypeKind::Ptr), addrSSA, "__rx_alloc",
      std::move(params)));
  return addrSSA;
}

Operand::SSA IRGenerator::emitVecPtr(const Operand::SSA &self) {
  auto vecType = std::make_unique<TypeModule>(llvmTypeKind::Struct);
  vecType->structName = "builtin_Vec";
  auto addrSSA = guard.makeSSA(SSAType::Addr);
  currentBlock->insts.emplace_back(
      std::make_unique<GEPInst>(addrSSA, std::move(vecType), self,
                                std::vector<Operand>{Operand(0), Operand(0)}));
  auto valueSSA = guard.makeSSA(SSAType::Value);
  currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
      std::make_unique<TypeModule>(llvmTypeKind::Ptr), addrSSA, valueSSA));
  return valueSSA;
}

Operand::SSA IRGenerator::emitVecLength(const Operand::SSA &self) {
  auto vecType = std::make_unique<TypeModule>(llvmTypeKind::Struct);
  vecType->structName = "builtin_Vec";
  auto addrSSA = guard.makeSSA(SSAType::Addr);
  currentBlock->insts.emplace_back(
      std::make_unique<GEPInst>(addrSSA, std::move(vecType), self,
                                std::vector<Operand>{Operand(0), Operand(1)}));
  auto valueSSA = guard.makeSSA(SSAType::Value);
  currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
      std::make_unique<TypeModule>(llvmTypeKind::I32), addrSSA, valueSSA));
  return valueSSA;
}

Operand::SSA IRGenerator::emitVecCapacity(const Operand::SSA &self) {
  auto vecType = std::make_unique<TypeModule>(llvmTypeKind::Struct);
  vecType->structName = "builtin_Vec";
  auto addrSSA = guard.makeSSA(SSAType::Addr);
  currentBlock->insts.emplace_back(
      std::make_unique<GEPInst>(addrSSA, std::move(vecType), self,
                                std::vector<Operand>{Operand(0), Operand(2)}));
  auto valueSSA = guard.makeSSA(SSAType::Value);
  currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
      std::make_unique<TypeModule>(llvmTypeKind::I32), addrSSA, valueSSA));
  return valueSSA;
}

std::any IRGenerator::visitPredicateLoopExpr(PredicateLoopExprNode *node) {
  auto condBlockName = guard.generateBasicBlockName();
  currentFunction->basicBlocks.emplace(
      condBlockName, std::make_unique<BasicBlock>(condBlockName));
  auto *condBlock = currentFunction->basicBlocks.at(condBlockName).get();
  currentBlock->terminator = std::make_unique<BrInst>(condBlock);
  currentBlock = condBlock;
  auto condOperand = emitValue(node->conditions.get());
  if (!condOperand) {
    return std::optional<Operand>{};
  }
  auto whileBodyBlockName = guard.generateBasicBlockName();
  currentFunction->basicBlocks.emplace(
      whileBodyBlockName, std::make_unique<BasicBlock>(whileBodyBlockName));
  auto *whileBodyBlock =
      currentFunction->basicBlocks.at(whileBodyBlockName).get();
  auto exitBlockName = guard.generateBasicBlockName();
  currentFunction->basicBlocks.emplace(
      exitBlockName, std::make_unique<BasicBlock>(exitBlockName));
  auto *exitBlock = currentFunction->basicBlocks.at(exitBlockName).get();
  auto fromType = std::make_unique<TypeModule>(llvmTypeKind::I8);
  auto toType = std::make_unique<TypeModule>(llvmTypeKind::I1);
  auto condValue = guard.makeSSA(SSAType::Value);
  currentBlock->insts.emplace_back(
      std::make_unique<CastInst>(CastOperator::Trunc, std::move(fromType),
                                 std::move(toType), *condOperand, condValue));
  currentBlock->terminator =
      std::make_unique<CondInst>(condValue, whileBodyBlock, exitBlock);
  loopContext.emplace_back(LoopContextRecord{condBlock, exitBlock});
  currentBlock = whileBodyBlock;
  auto bodyValue = emitValue(node->body.get());
  if (bodyValue) {
    currentBlock->terminator = std::make_unique<BrInst>(condBlock);
  }
  currentBlock = exitBlock;
  loopContext.pop_back();
  return std::optional<Operand>{Operand(Operand::Unit{})};
}

void IRGenerator::generateBuiltinFunctions() {
  while (!pendingBuiltinFunctions.empty()) {
    auto *module = pendingBuiltinFunctions.front();
    pendingBuiltinFunctions.pop_front();
    std::vector<std::string> parameterNames;
    parameterNames.reserve(module->paramTypes.size());
    for (std::size_t i = 0; i < module->paramTypes.size(); ++i) {
      parameterNames.emplace_back(i == 0 && module->symbol->receiver !=
                                                ReceiverKind::None
                                      ? std::string("self")
                                      : "arg" + std::to_string(i));
    }
    beginFunction(module, parameterNames);
    switch (module->symbol->kind) {
    case FunctionKind::Normal:
      std::unreachable();
    case FunctionKind::Clone: {
      const auto &cloneType = module->symbol->returnType;
      auto emitClone = [&](const Type &type, const Operand::SSA &address) {
        const auto calleeName = SemanticAnalyzer::encodeType(type) + "_clone";
        auto *callee = getOrCreateFunction(
            modules->analyzer->builtinFunctions.at(calleeName).get());
        auto valueSSA = guard.makeSSA(SSAType::Value);
        std::vector<std::pair<std::unique_ptr<TypeModule>, Operand>> params;
        params.emplace_back(std::make_unique<TypeModule>(llvmTypeKind::Ptr),
                            address);
        currentBlock->insts.emplace_back(std::make_unique<CallInst>(
            std::make_unique<TypeModule>(*callee->returnType), valueSSA,
            callee->functionName, std::move(params)));
        return valueSSA;
      };
      switch (cloneType.kind) {
      case TypeKind::I32:
      case TypeKind::U32:
      case TypeKind::Isize:
      case TypeKind::Usize: {
        auto resultSSA = guard.makeSSA(SSAType::Value);
        const auto &ptrSSA = module->paramTypes.front().second;
        currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
            std::make_unique<TypeModule>(llvmTypeKind::I32), ptrSSA,
            resultSSA));
        currentBlock->terminator = std::make_unique<ReturnInst>(
            std::make_unique<TypeModule>(llvmTypeKind::I32), resultSSA);
        break;
      }
      case TypeKind::Bool: {
        auto resultSSA = guard.makeSSA(SSAType::Value);
        const auto &ptrSSA = module->paramTypes.front().second;
        currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
            std::make_unique<TypeModule>(llvmTypeKind::I8), ptrSSA, resultSSA));
        currentBlock->terminator = std::make_unique<ReturnInst>(
            std::make_unique<TypeModule>(llvmTypeKind::I8), resultSSA);
        break;
      }
      case TypeKind::ImmutReference: {
        auto resultSSA = guard.makeSSA(SSAType::Value);
        const auto &ptrSSA = module->paramTypes.front().second;
        currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
            std::make_unique<TypeModule>(llvmTypeKind::Ptr), ptrSSA,
            resultSSA));
        currentBlock->terminator = std::make_unique<ReturnInst>(
            std::make_unique<TypeModule>(llvmTypeKind::Ptr), resultSSA);
        break;
      }
      case TypeKind::Array: {
        Operand::SSA lastArraySSA("poison");
        const auto &selfSSA = module->paramTypes.front().second;
        for (std::size_t i = 0;
             i < std::get<long long>(cloneType.arrayLen.value().value); ++i) {
          auto addrSSA = guard.makeSSA(SSAType::Addr);
          {
            auto _ = std::make_unique<TypeModule>(
                TypeModule::lowerType(&cloneType, false));
            currentBlock->insts.emplace_back(std::make_unique<GEPInst>(
                addrSSA, std::move(_), selfSSA,
                std::vector<Operand>{Operand(0), Operand(i)}));
          }
          auto valueSSA = emitClone(*cloneType.elementType, addrSSA);
          auto newArraySSA = guard.makeSSA(SSAType::Value);
          {
            auto arrayType = std::make_unique<TypeModule>(
                TypeModule::lowerType(&cloneType, false));
            auto elementType = std::make_unique<TypeModule>(
                TypeModule::lowerType(cloneType.elementType.get(), false));
            currentBlock->insts.emplace_back(std::make_unique<InsertvalueInst>(
                newArraySSA, std::move(arrayType), lastArraySSA,
                std::move(elementType), valueSSA, std::vector<std::size_t>{i}));
            lastArraySSA = newArraySSA;
          }
        }
        currentBlock->terminator = std::make_unique<ReturnInst>(
            std::make_unique<TypeModule>(*module->returnType), lastArraySSA);
        break;
      }
      case TypeKind::Struct: {
        auto *symbol = modules->analyzer->globalScopeVisitor->lookupStruct(
            cloneType.structName.value());
        Operand::SSA lastStructSSA("poison");
        const auto &selfSSA = module->paramTypes.front().second;
        for (std::size_t i = 0; i < symbol->layoutFields.size(); ++i) {
          const auto &fieldType =
              symbol->structFields.at(symbol->layoutFields[i]);
          auto addrSSA = guard.makeSSA(SSAType::Addr);
          currentBlock->insts.emplace_back(std::make_unique<GEPInst>(
              addrSSA, std::make_unique<TypeModule>(*module->returnType),
              selfSSA, std::vector<Operand>{Operand(0), Operand(i)}));
          auto valueSSA = emitClone(fieldType, addrSSA);
          auto newStructSSA = guard.makeSSA(SSAType::Value);
          currentBlock->insts.emplace_back(std::make_unique<InsertvalueInst>(
              newStructSSA, std::make_unique<TypeModule>(*module->returnType),
              lastStructSSA,
              std::make_unique<TypeModule>(
                  TypeModule::lowerType(&fieldType, false)),
              valueSSA, std::vector<std::size_t>{i}));
          lastStructSSA = newStructSSA;
        }
        currentBlock->terminator = std::make_unique<ReturnInst>(
            std::make_unique<TypeModule>(*module->returnType), lastStructSSA);
        break;
      }
      case TypeKind::Box: {
        const auto &elementType = *cloneType.elementType;
        auto sourceSSA = guard.makeSSA(SSAType::Addr);
        currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
            std::make_unique<TypeModule>(llvmTypeKind::Ptr),
            module->paramTypes.front().second, sourceSSA));
        auto typeLayout = layout(TypeModule::lowerType(&elementType, false));
        auto resultSSA =
            emitAlloc(Operand(typeLayout.size), Operand(typeLayout.alignment));
        auto valueSSA = emitClone(elementType, sourceSSA);
        currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
            std::make_unique<TypeModule>(
                TypeModule::lowerType(&elementType, false)),
            valueSSA, resultSSA));
        currentBlock->terminator = std::make_unique<ReturnInst>(
            std::make_unique<TypeModule>(*module->returnType), resultSSA);
        break;
      }
      case TypeKind::Vec: {
        const auto &elementType = *cloneType.elementType;
        auto typeLayout = layout(TypeModule::lowerType(&elementType, false));
        auto originalPtrSSA = emitVecPtr(module->paramTypes.front().second);
        auto lengthSSA = emitVecLength(module->paramTypes.front().second);
        auto capacitySSA = emitVecCapacity(module->paramTypes.front().second);
        auto totSizeSSA = guard.makeSSA(SSAType::Value);
        currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
            BinaryIntergerOperator::Mul,
            std::make_unique<TypeModule>(llvmTypeKind::I32), capacitySSA,
            Operand(typeLayout.size), totSizeSSA));
        auto ptrSSA = emitAlloc(totSizeSSA, Operand(typeLayout.alignment));
        auto indexSSA = guard.makeSSA(SSAType::Value);
        currentBlock->insts.emplace_back(std::make_unique<AllocaInst>(
            indexSSA, std::make_unique<TypeModule>(llvmTypeKind::I32)));
        currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
            std::make_unique<TypeModule>(llvmTypeKind::I32), Operand(0),
            indexSSA));
        auto condBlockName = guard.generateBasicBlockName();
        currentFunction->basicBlocks.emplace(
            condBlockName, std::make_unique<BasicBlock>(condBlockName));
        auto *condBlock = currentFunction->basicBlocks.at(condBlockName).get();
        currentBlock->terminator = std::make_unique<BrInst>(condBlock);
        currentBlock = condBlock;
        auto comparisonSSA = guard.makeSSA(SSAType::Value);
        auto indexValueSSA = guard.makeSSA(SSAType::Value);
        currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
            std::make_unique<TypeModule>(llvmTypeKind::I32), indexSSA,
            indexValueSSA));
        currentBlock->insts.emplace_back(std::make_unique<ICmpInst>(
            ICmpOperator::Ult, std::make_unique<TypeModule>(llvmTypeKind::I32),
            indexValueSSA, lengthSSA, comparisonSSA));
        auto bodyBlockName = guard.generateBasicBlockName();
        currentFunction->basicBlocks.emplace(
            bodyBlockName, std::make_unique<BasicBlock>(bodyBlockName));
        auto *bodyBlock = currentFunction->basicBlocks.at(bodyBlockName).get();
        auto exitBlockName = guard.generateBasicBlockName();
        currentFunction->basicBlocks.emplace(
            exitBlockName, std::make_unique<BasicBlock>(exitBlockName));
        auto *exitBlock = currentFunction->basicBlocks.at(exitBlockName).get();
        currentBlock->terminator =
            std::make_unique<CondInst>(comparisonSSA, bodyBlock, exitBlock);
        currentBlock = bodyBlock;
        auto originalAddr = guard.makeSSA(SSAType::Addr);
        {
          auto _ = std::make_unique<TypeModule>(
              TypeModule::lowerType(&elementType, false));
          currentBlock->insts.emplace_back(std::make_unique<GEPInst>(
              originalAddr, std::move(_), originalPtrSSA,
              std::vector<Operand>{indexValueSSA}));
        }
        auto valueSSA = emitClone(*cloneType.elementType, originalAddr);
        auto newAddr = guard.makeSSA(SSAType::Addr);
        {
          auto _ = std::make_unique<TypeModule>(
              TypeModule::lowerType(&elementType, false));
          currentBlock->insts.emplace_back(
              std::make_unique<GEPInst>(newAddr, std::move(_), ptrSSA,
                                        std::vector<Operand>{indexValueSSA}));
        }
        auto _ = std::make_unique<TypeModule>(
            TypeModule::lowerType(cloneType.elementType.get(), false));
        currentBlock->insts.emplace_back(
            std::make_unique<StoreInst>(std::move(_), valueSSA, newAddr));
        auto nextIndexSSA = guard.makeSSA(SSAType::Value);
        currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
            BinaryIntergerOperator::Add,
            std::make_unique<TypeModule>(llvmTypeKind::I32), indexValueSSA,
            Operand(1), nextIndexSSA));
        currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
            std::make_unique<TypeModule>(llvmTypeKind::I32), nextIndexSSA,
            indexSSA));
        currentBlock->terminator = std::make_unique<BrInst>(condBlock);
        currentBlock = exitBlock;
        Operand::SSA lastVecSSA("poison");
        std::vector<std::pair<llvmTypeKind, Operand>> fields{
            {llvmTypeKind::Ptr, ptrSSA},
            {llvmTypeKind::I32, lengthSSA},
            {llvmTypeKind::I32, capacitySSA}};
        for (std::size_t i = 0; i < fields.size(); ++i) {
          auto newVecSSA = guard.makeSSA(SSAType::Value);
          currentBlock->insts.emplace_back(std::make_unique<InsertvalueInst>(
              newVecSSA, std::make_unique<TypeModule>(*module->returnType),
              lastVecSSA, std::make_unique<TypeModule>(fields[i].first),
              fields[i].second, std::vector<std::size_t>{i}));
          lastVecSSA = newVecSSA;
        }
        currentBlock->terminator = std::make_unique<ReturnInst>(
            std::make_unique<TypeModule>(*module->returnType), lastVecSSA);
        break;
      }
      default:
        std::unreachable();
      }
      break;
    }
    case FunctionKind::Len: {
      const auto &selfType = *module->symbol->paramTypes.front().elementType;
      if (selfType.kind == TypeKind::Array) {
        currentBlock->terminator = std::make_unique<ReturnInst>(
            std::make_unique<TypeModule>(*module->returnType),
            Operand(std::get<long long>(selfType.arrayLen.value().value)));
      } else {
        auto lengthSSA = emitVecLength(module->paramTypes.front().second);
        currentBlock->terminator = std::make_unique<ReturnInst>(
            std::make_unique<TypeModule>(*module->returnType), lengthSSA);
      }
      break;
    }
    case FunctionKind::New: {
      const auto &newType = module->symbol->returnType;
      if (newType.kind == TypeKind::Box) {
        const auto &elementType = *newType.elementType;
        auto typeLayout = layout(TypeModule::lowerType(&elementType, false));
        auto addrSSA =
            emitAlloc(Operand(typeLayout.size), Operand(typeLayout.alignment));
        currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
            std::make_unique<TypeModule>(
                TypeModule::lowerType(&elementType, false)),
            module->paramTypes.front().second, addrSSA));
        currentBlock->terminator = std::make_unique<ReturnInst>(
            std::make_unique<TypeModule>(llvmTypeKind::Ptr), addrSSA);
      } else {
        auto vecModule = TypeModule(llvmTypeKind::Struct);
        vecModule.structName = "builtin_Vec";
        currentBlock->terminator = std::make_unique<ReturnInst>(
            std::make_unique<TypeModule>(vecModule), Operand::Unit());
      }
      break;
    }
    case FunctionKind::IsEmpty: {
      auto lengthSSA = emitVecLength(module->paramTypes.front().second);
      auto comparisonSSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<ICmpInst>(
          ICmpOperator::Eq, std::make_unique<TypeModule>(llvmTypeKind::I32),
          lengthSSA, Operand(0), comparisonSSA));
      auto resultSSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<CastInst>(
          CastOperator::ZExt, std::make_unique<TypeModule>(llvmTypeKind::I1),
          std::make_unique<TypeModule>(llvmTypeKind::I8), comparisonSSA,
          resultSSA));
      currentBlock->terminator = std::make_unique<ReturnInst>(
          std::make_unique<TypeModule>(*module->returnType), resultSSA);
      break;
    }
    case FunctionKind::Push: {
      const auto &vecType = *module->symbol->paramTypes.front().elementType;
      const auto &elementType = *vecType.elementType;
      auto typeLayout = layout(TypeModule::lowerType(&elementType, false));
      const auto &selfSSA = module->paramTypes.front().second;
      auto originalPtrSSA = emitVecPtr(selfSSA);
      auto lengthSSA = emitVecLength(selfSSA);
      auto capacitySSA = emitVecCapacity(selfSSA);
      auto comparisonSSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<ICmpInst>(
          ICmpOperator::Eq, std::make_unique<TypeModule>(llvmTypeKind::I32),
          lengthSSA, capacitySSA, comparisonSSA));
      auto growBlockName = guard.generateBasicBlockName();
      currentFunction->basicBlocks.emplace(
          growBlockName, std::make_unique<BasicBlock>(growBlockName));
      auto *growBlock = currentFunction->basicBlocks.at(growBlockName).get();
      auto appendBlockName = guard.generateBasicBlockName();
      currentFunction->basicBlocks.emplace(
          appendBlockName, std::make_unique<BasicBlock>(appendBlockName));
      auto *appendBlock =
          currentFunction->basicBlocks.at(appendBlockName).get();
      currentBlock->terminator =
          std::make_unique<CondInst>(comparisonSSA, growBlock, appendBlock);
      currentBlock = growBlock;
      auto doubledCapacitySSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
          BinaryIntergerOperator::Mul,
          std::make_unique<TypeModule>(llvmTypeKind::I32), capacitySSA,
          Operand(2), doubledCapacitySSA));
      auto newCapacitySSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
          BinaryIntergerOperator::Add,
          std::make_unique<TypeModule>(llvmTypeKind::I32), doubledCapacitySSA,
          Operand(1), newCapacitySSA));
      auto totSizeSSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
          BinaryIntergerOperator::Mul,
          std::make_unique<TypeModule>(llvmTypeKind::I32), newCapacitySSA,
          Operand(typeLayout.size), totSizeSSA));
      auto newPtrSSA = emitAlloc(totSizeSSA, Operand(typeLayout.alignment));
      auto indexSSA = guard.makeSSA(SSAType::Addr);
      currentBlock->insts.emplace_back(std::make_unique<AllocaInst>(
          indexSSA, std::make_unique<TypeModule>(llvmTypeKind::I32)));
      currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
          std::make_unique<TypeModule>(llvmTypeKind::I32), Operand(0),
          indexSSA));
      auto condBlockName = guard.generateBasicBlockName();
      currentFunction->basicBlocks.emplace(
          condBlockName, std::make_unique<BasicBlock>(condBlockName));
      auto *condBlock = currentFunction->basicBlocks.at(condBlockName).get();
      currentBlock->terminator = std::make_unique<BrInst>(condBlock);
      currentBlock = condBlock;
      auto indexValueSSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
          std::make_unique<TypeModule>(llvmTypeKind::I32), indexSSA,
          indexValueSSA));
      auto loopComparisonSSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<ICmpInst>(
          ICmpOperator::Ult, std::make_unique<TypeModule>(llvmTypeKind::I32),
          indexValueSSA, lengthSSA, loopComparisonSSA));
      auto bodyBlockName = guard.generateBasicBlockName();
      currentFunction->basicBlocks.emplace(
          bodyBlockName, std::make_unique<BasicBlock>(bodyBlockName));
      auto *bodyBlock = currentFunction->basicBlocks.at(bodyBlockName).get();
      auto exitBlockName = guard.generateBasicBlockName();
      currentFunction->basicBlocks.emplace(
          exitBlockName, std::make_unique<BasicBlock>(exitBlockName));
      auto *exitBlock = currentFunction->basicBlocks.at(exitBlockName).get();
      currentBlock->terminator =
          std::make_unique<CondInst>(loopComparisonSSA, bodyBlock, exitBlock);
      currentBlock = bodyBlock;
      auto originalAddr = guard.makeSSA(SSAType::Addr);
      {
        auto _ = std::make_unique<TypeModule>(
            TypeModule::lowerType(&elementType, false));
        currentBlock->insts.emplace_back(std::make_unique<GEPInst>(
            originalAddr, std::move(_), originalPtrSSA,
            std::vector<Operand>{indexValueSSA}));
      }
      auto valueSSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
          std::make_unique<TypeModule>(
              TypeModule::lowerType(&elementType, false)),
          originalAddr, valueSSA));
      auto newAddr = guard.makeSSA(SSAType::Addr);
      {
        auto _ = std::make_unique<TypeModule>(
            TypeModule::lowerType(&elementType, false));
        currentBlock->insts.emplace_back(
            std::make_unique<GEPInst>(newAddr, std::move(_), newPtrSSA,
                                      std::vector<Operand>{indexValueSSA}));
      }
      currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
          std::make_unique<TypeModule>(
              TypeModule::lowerType(&elementType, false)),
          valueSSA, newAddr));
      auto nextIndexSSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
          BinaryIntergerOperator::Add,
          std::make_unique<TypeModule>(llvmTypeKind::I32), indexValueSSA,
          Operand(1), nextIndexSSA));
      currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
          std::make_unique<TypeModule>(llvmTypeKind::I32), nextIndexSSA,
          indexSSA));
      currentBlock->terminator = std::make_unique<BrInst>(condBlock);
      currentBlock = exitBlock;
      auto ptrAddr = guard.makeSSA(SSAType::Addr);
      {
        auto _ = std::make_unique<TypeModule>(
            TypeModule::lowerType(&vecType, false));
        currentBlock->insts.emplace_back(std::make_unique<GEPInst>(
            ptrAddr, std::move(_), selfSSA,
            std::vector<Operand>{Operand(0), Operand(0)}));
      }
      currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
          std::make_unique<TypeModule>(llvmTypeKind::Ptr), newPtrSSA, ptrAddr));
      auto capacityAddr = guard.makeSSA(SSAType::Addr);
      {
        auto _ = std::make_unique<TypeModule>(
            TypeModule::lowerType(&vecType, false));
        currentBlock->insts.emplace_back(std::make_unique<GEPInst>(
            capacityAddr, std::move(_), selfSSA,
            std::vector<Operand>{Operand(0), Operand(2)}));
      }
      currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
          std::make_unique<TypeModule>(llvmTypeKind::I32), newCapacitySSA,
          capacityAddr));
      currentBlock->terminator = std::make_unique<BrInst>(appendBlock);
      currentBlock = appendBlock;
      auto ptrSSA = emitVecPtr(selfSSA);
      auto elementAddr = guard.makeSSA(SSAType::Addr);
      {
        auto _ = std::make_unique<TypeModule>(
            TypeModule::lowerType(&elementType, false));
        currentBlock->insts.emplace_back(
            std::make_unique<GEPInst>(elementAddr, std::move(_), ptrSSA,
                                      std::vector<Operand>{lengthSSA}));
      }
      currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
          std::make_unique<TypeModule>(
              TypeModule::lowerType(&elementType, false)),
          module->paramTypes[1].second, elementAddr));
      auto newLengthSSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
          BinaryIntergerOperator::Add,
          std::make_unique<TypeModule>(llvmTypeKind::I32), lengthSSA,
          Operand(1), newLengthSSA));
      auto lengthAddr = guard.makeSSA(SSAType::Addr);
      {
        auto _ = std::make_unique<TypeModule>(
            TypeModule::lowerType(&vecType, false));
        currentBlock->insts.emplace_back(std::make_unique<GEPInst>(
            lengthAddr, std::move(_), selfSSA,
            std::vector<Operand>{Operand(0), Operand(1)}));
      }
      currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
          std::make_unique<TypeModule>(llvmTypeKind::I32), newLengthSSA,
          lengthAddr));
      currentBlock->terminator = std::make_unique<ReturnInst>(
          std::make_unique<TypeModule>(llvmTypeKind::Void));
      break;
    }
    case FunctionKind::Remove: {
      const auto &vecType = *module->symbol->paramTypes.front().elementType;
      const auto &elementType = *vecType.elementType;
      const auto &selfSSA = module->paramTypes.front().second;
      const auto &removeIndexSSA = module->paramTypes[1].second;
      auto ptrSSA = emitVecPtr(selfSSA);
      auto lengthSSA = emitVecLength(selfSSA);
      auto removedAddr = guard.makeSSA(SSAType::Addr);
      {
        auto _ = std::make_unique<TypeModule>(
            TypeModule::lowerType(&elementType, false));
        currentBlock->insts.emplace_back(
            std::make_unique<GEPInst>(removedAddr, std::move(_), ptrSSA,
                                      std::vector<Operand>{removeIndexSSA}));
      }
      auto removedValueSSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
          std::make_unique<TypeModule>(*module->returnType), removedAddr,
          removedValueSSA));
      auto newLengthSSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
          BinaryIntergerOperator::Sub,
          std::make_unique<TypeModule>(llvmTypeKind::I32), lengthSSA,
          Operand(1), newLengthSSA));
      auto indexSSA = guard.makeSSA(SSAType::Addr);
      currentBlock->insts.emplace_back(std::make_unique<AllocaInst>(
          indexSSA, std::make_unique<TypeModule>(llvmTypeKind::I32)));
      currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
          std::make_unique<TypeModule>(llvmTypeKind::I32), removeIndexSSA,
          indexSSA));
      auto condBlockName = guard.generateBasicBlockName();
      currentFunction->basicBlocks.emplace(
          condBlockName, std::make_unique<BasicBlock>(condBlockName));
      auto *condBlock = currentFunction->basicBlocks.at(condBlockName).get();
      currentBlock->terminator = std::make_unique<BrInst>(condBlock);
      currentBlock = condBlock;
      auto comparisonSSA = guard.makeSSA(SSAType::Value);
      auto indexValueSSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
          std::make_unique<TypeModule>(llvmTypeKind::I32), indexSSA,
          indexValueSSA));
      currentBlock->insts.emplace_back(std::make_unique<ICmpInst>(
          ICmpOperator::Ult, std::make_unique<TypeModule>(llvmTypeKind::I32),
          indexValueSSA, newLengthSSA, comparisonSSA));
      auto bodyBlockName = guard.generateBasicBlockName();
      currentFunction->basicBlocks.emplace(
          bodyBlockName, std::make_unique<BasicBlock>(bodyBlockName));
      auto *bodyBlock = currentFunction->basicBlocks.at(bodyBlockName).get();
      auto exitBlockName = guard.generateBasicBlockName();
      currentFunction->basicBlocks.emplace(
          exitBlockName, std::make_unique<BasicBlock>(exitBlockName));
      auto *exitBlock = currentFunction->basicBlocks.at(exitBlockName).get();
      currentBlock->terminator =
          std::make_unique<CondInst>(comparisonSSA, bodyBlock, exitBlock);
      currentBlock = bodyBlock;
      auto nextIndexSSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<BinaryInst>(
          BinaryIntergerOperator::Add,
          std::make_unique<TypeModule>(llvmTypeKind::I32), indexValueSSA,
          Operand(1), nextIndexSSA));
      auto originalAddr = guard.makeSSA(SSAType::Addr);
      {
        auto _ = std::make_unique<TypeModule>(
            TypeModule::lowerType(&elementType, false));
        currentBlock->insts.emplace_back(
            std::make_unique<GEPInst>(originalAddr, std::move(_), ptrSSA,
                                      std::vector<Operand>{nextIndexSSA}));
      }
      auto valueSSA = guard.makeSSA(SSAType::Value);
      currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
          std::make_unique<TypeModule>(
              TypeModule::lowerType(&elementType, false)),
          originalAddr, valueSSA));
      auto newAddr = guard.makeSSA(SSAType::Addr);
      {
        auto _ = std::make_unique<TypeModule>(
            TypeModule::lowerType(&elementType, false));
        currentBlock->insts.emplace_back(
            std::make_unique<GEPInst>(newAddr, std::move(_), ptrSSA,
                                      std::vector<Operand>{indexValueSSA}));
      }
      currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
          std::make_unique<TypeModule>(
              TypeModule::lowerType(&elementType, false)),
          valueSSA, newAddr));
      currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
          std::make_unique<TypeModule>(llvmTypeKind::I32), nextIndexSSA,
          indexSSA));
      currentBlock->terminator = std::make_unique<BrInst>(condBlock);
      currentBlock = exitBlock;
      auto lengthAddr = guard.makeSSA(SSAType::Addr);
      {
        auto _ = std::make_unique<TypeModule>(
            TypeModule::lowerType(&vecType, false));
        currentBlock->insts.emplace_back(std::make_unique<GEPInst>(
            lengthAddr, std::move(_), selfSSA,
            std::vector<Operand>{Operand(0), Operand(1)}));
      }
      currentBlock->insts.emplace_back(std::make_unique<StoreInst>(
          std::make_unique<TypeModule>(llvmTypeKind::I32), newLengthSSA,
          lengthAddr));
      currentBlock->terminator = std::make_unique<ReturnInst>(
          std::make_unique<TypeModule>(*module->returnType), removedValueSSA);
      break;
    }
    }
    module->state = functionState::Defined;
  }
}

void IRGenerator::generateEqualFunctions() {
  for (const auto &[mangling, type] : pendingEqualFunctions) {
    emitEqualFunction(mangling, type);
  }
}
