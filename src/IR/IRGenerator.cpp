#include "IRGenerator.hpp"
#include "AST.hpp"
#include "IRVisitor.hpp"
#include "SemanticAnalyzer.hpp"
#include "SemanticResult.hpp"
#include "SymbolCollector.hpp"
#include <any>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

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

std::any AllocaInst::accept(IRVisitor *visitor) {
  return visitor->visitAllocaInst(this);
}

std::any LoadInst::accept(IRVisitor *visitor) {
  return visitor->visitLoadInst(this);
}

std::any StoreInst::accept(IRVisitor *visitor) {
  return visitor->visitStoreInst(this);
}

std::any ReturnInst::accept(IRVisitor *visitor) {
  return visitor->visitReturnInst(this);
}

void IRModules::addRxItems(const SemanticAnalyzer &analyzer) {
  auto manglingFunctionName = [](FunctionSymbol *symbol) {
    if (!symbol->owner) {
      return symbol->name;
    } else {
      return std::to_string(symbol->name.size()) + "_" + symbol->owner->name +
             "_" + symbol->name;
    }
  };
  auto emplaceFunction = [&](FunctionSymbol *symbol) {
    std::unique_ptr<TypeModule> typePtr;
    if (symbol->returnType.kind == TypeKind::Unit) {
      TypeModule unitReturnType(llvmTypeKind::Void);
      typePtr = std::make_unique<TypeModule>(std::move(unitReturnType));
    } else {
      typePtr = std::make_unique<TypeModule>(
          TypeModule::lowerType(&symbol->returnType));
    }
    std::vector<std::pair<std::unique_ptr<TypeModule>, Operand::SSA>>
        paramTypePtrs;
    paramTypePtrs.reserve(symbol->paramTypes.size());
    for (const auto &paramType : symbol->paramTypes) {
      auto type =
          std::make_unique<TypeModule>(TypeModule::lowerType(&paramType));
      paramTypePtrs.emplace_back(std::move(type), Operand::SSA{});
    }
    functions.emplace_back(std::make_unique<FunctionModule>(
        functionState::Defined, manglingFunctionName(symbol),
        std::move(typePtr), std::move(paramTypePtrs)));
    functionModuleMap.emplace(symbol->declaration, functions.back().get());
  };
  for (const auto &[structName, structItem] :
       analyzer.globalScopeVisitor->typeNameSpace) {
    auto *symbol = dynamic_cast<StructSymbol *>(structItem.get());
    structs.emplace_back(std::make_unique<StructModule>(symbol));
    for (auto &[itemName, itemSymbol] : symbol->structSymbols) {
      if (auto *funcSymbol = dynamic_cast<FunctionSymbol *>(itemSymbol.get())) {
        emplaceFunction(funcSymbol);
      }
    }
  }
  for (const auto &[funcName, funcItem] :
       analyzer.globalScopeVisitor->valueNameSpace) {
    if (auto *symbol = dynamic_cast<FunctionSymbol *>(funcItem.get())) {
      emplaceFunction(symbol);
    }
  }
}

Operand IRGenerator::emitValue(ExprNode *node) {
  auto operand = std::any_cast<Operand>(node->accept(this));
  if (node->semantic->isPlace) {
    auto valueSSA = guard.makeSSA(SSAType::Value);
    auto llvmType = std::make_unique<TypeModule>(
        TypeModule::lowerType(&node->semantic->adjustedType));
    currentBlock->insts.emplace_back(std::make_unique<LoadInst>(
        std::move(llvmType), std::get<Operand::SSA>(operand.operand),
        valueSSA));
    return Operand(valueSSA);
  }
  return operand;
}

// Crate and attribute.
std::any IRGenerator::visitCrate(CrateNode *node) {
  for (auto &item : node->items) {
    if (auto *function = dynamic_cast<FunctionDeclItemNode *>(item.get())) {
      function->accept(this);
    }
  }
  return {};
}

// Items.
std::any IRGenerator::visitFunctionDeclItem(FunctionDeclItemNode *node) {
  auto *funcModule = modules->functionModuleMap.at(node);
  guard.clear();
  // Name all incoming parameters before allocating their local storage.
  const bool hasSelf = node->selfParam.has_value();
  for (std::size_t i = 0; i < funcModule->paramTypes.size(); ++i) {
    auto &[type, ssa] = funcModule->paramTypes[i];
    const auto name =
        hasSelf && i == 0
            ? std::string("self")
            : node->functionParam[i - (hasSelf ? 1 : 0)].identifier;
    ssa = guard.makeSSA(name, SSAType::Value);
  }
  funcModule->basicBlocks.emplace_back(std::make_unique<BasicBlock>());
  currentBlock = funcModule->basicBlocks.front().get();
  currentBlock->blockName = guard.generateBasicBlockName();
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
          std::move(_), funcModule->paramTypes[index].second, ptr));
    }
    ++index;
  };
  std::size_t index = 0;
  if (node->selfParam) {
    defineParam(node->selfParam->paramSymbol,
                *funcModule->paramTypes[index].first, index);
  }
  for (auto &param : node->functionParam) {
    defineParam(param.paramSymbol, *funcModule->paramTypes[index].first, index);
  }
  node->functionBody->accept(this);
  if (!currentBlock->terminator &&
      funcModule->returnType->kind == llvmTypeKind::Void) {
    currentBlock->terminator = std::make_unique<ReturnInst>(
        std::make_unique<TypeModule>(llvmTypeKind::Void));
  }
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

std::any IRGenerator::visitLetStmt(LetStmtNode *) { return {}; }

std::any IRGenerator::visitExpressionStmt(ExpressionStmtNode *node) {
  node->expr->accept(this);
  return {};
}

// Expressions without blocks.
std::any IRGenerator::visitUnitExpr(UnitExprNode *) { return {}; }

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
        return Operand(imm);
      } else if (std::holds_alternative<bool>(constantSymbol->value.value)) {
        Operand::Immediate imm(std::get<bool>(constantSymbol->value.value));
        return Operand(imm);
      } else {
        std::unreachable();
      }
    } else {
      const auto *localSymbol =
          std::get<const LocalSymbol *>(value->bindingSymbol);
      return Operand(guard.localAddressMap.at(localSymbol));
    }
  }
  std::unreachable();
}

std::any IRGenerator::visitLiteralExpr(LiteralExprNode *node) {
  Operand::Immediate imm;
  auto *literalSemantic =
      dynamic_cast<LiteralExprSemantic *>(node->semantic.get());
  imm.constValue = literalSemantic->value;
  return Operand(imm);
}

std::any IRGenerator::visitUnaryExpr(UnaryExprNode *) { return {}; }

std::any IRGenerator::visitBinaryExpr(BinaryExprNode *) { return {}; }

std::any IRGenerator::visitComparisonExpr(ComparisonExprNode *) { return {}; }

std::any IRGenerator::visitLogicExpr(LogicExprNode *) { return {}; }

std::any IRGenerator::visitCastExpr(CastExprNode *) { return {}; }

std::any IRGenerator::visitBorrowExpr(BorrowExprNode *) { return {}; }

std::any IRGenerator::visitDereferenceExpr(DereferenceExprNode *) { return {}; }

std::any IRGenerator::visitAssignmentExpr(AssignmentExprNode *) { return {}; }

std::any IRGenerator::visitMagnitudeExpr(MagnitudeExprNode *) { return {}; }

std::any IRGenerator::visitConstExpr(ConstExprNode *node) {
  Operand::Immediate imm;
  auto *constantSemantic =
      dynamic_cast<ConstExprSemantic *>(node->semantic.get());
  imm.constValue = constantSemantic->value;
  return Operand(imm);
}

std::any IRGenerator::visitArrayExpr(ArrayExprNode *) { return {}; }

std::any IRGenerator::visitIndexExpr(IndexExprNode *) { return {}; }

std::any IRGenerator::visitStructExpr(StructExprNode *) { return {}; }

std::any IRGenerator::visitCallExpr(CallExprNode *) { return {}; }

std::any IRGenerator::visitMethodCallExpr(MethodCallExprNode *) { return {}; }

std::any IRGenerator::visitReturnExpr(ReturnExprNode *node) {
  if (node->returnValue) {
    auto returnValue = emitValue(node->returnValue.value().get());
    auto returnType = std::make_unique<TypeModule>(TypeModule::lowerType(
        &node->returnValue.value()->semantic->adjustedType));
    currentBlock->terminator =
        std::make_unique<ReturnInst>(std::move(returnType), returnValue);
  } else {
    auto returnType = std::make_unique<TypeModule>(llvmTypeKind::Void);
    currentBlock->terminator =
        std::make_unique<ReturnInst>(std::move(returnType));
  }
  return {};
}

std::any IRGenerator::visitAccessExpr(AccessExprNode *) { return {}; }

std::any IRGenerator::visitContinueExpr(ContinueExprNode *) { return {}; }

std::any IRGenerator::visitBreakExpr(BreakExprNode *) { return {}; }

// Expressions with blocks.
std::any IRGenerator::visitBlockExpr(BlockExprNode *node) {
  for (auto &statement : node->statements) {
    if (currentBlock->terminator) {
      break;
    }
    statement->accept(this);
  }
  if (!currentBlock->terminator && node->tailExpr) {
    return node->tailExpr.value()->accept(this);
  }
  return {};
}

std::any IRGenerator::visitIfExpr(IfExprNode *) { return {}; }

std::any IRGenerator::visitInfiniteLoopExpr(InfiniteLoopExprNode *) {
  return {};
}

std::any IRGenerator::visitPredicateLoopExpr(PredicateLoopExprNode *) {
  return {};
}
