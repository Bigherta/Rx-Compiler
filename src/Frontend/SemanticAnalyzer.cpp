#include "SemanticAnalyzer.hpp"
#include "AST.hpp"
#include "Scope.hpp"
#include "SymbolCollector.hpp"
#include <algorithm>
#include <any>
#include <memory>
#include <utility>
#include <variant>
implementationScope *SemanticAnalyzer::findImplementationScope(Scope *scope) {
  while (scope) {
    if (auto *implScope = dynamic_cast<implementationScope *>(scope)) {
      return implScope;
    }
    scope = scope->parentScope;
  }
  return nullptr;
}

Type SemanticAnalyzer::parseIntegerLiteral(const position &position,
                                           std::string literal,
                                           DiagnosticError *err,
                                           bool isNegative,
                                           const Type *expected) {
  literal.erase(std::remove(literal.begin(), literal.end(), '_'),
                literal.end());
  int base;
  if (literal.starts_with("0x")) {
    base = 16;
  } else if (literal.starts_with("0o")) {
    base = 8;
  } else if (literal.starts_with("0b")) {
    base = 2;
  } else {
    base = 10;
  }
  if (base != 10) {
    literal.erase(0, 2);
  }
  bool isUnsigned = false;
  bool isSizeType = false;
  bool hasSuffix = true;

  if (literal.ends_with("i32")) {
    literal.erase(literal.size() - 3);
  } else if (literal.ends_with("isize")) {
    isSizeType = true;
    literal.erase(literal.size() - 5);
  } else if (literal.ends_with("u32")) {
    isUnsigned = true;
    literal.erase(literal.size() - 3);
  } else if (literal.ends_with("usize")) {
    isUnsigned = true;
    isSizeType = true;
    literal.erase(literal.size() - 5);
  } else {
    hasSuffix = false;
  }

  if (!hasSuffix && expected) {
    switch (expected->kind) {
    case TypeKind::I32:
      break;
    case TypeKind::U32:
      isUnsigned = true;
      break;
    case TypeKind::Isize:
      isSizeType = true;
      break;
    case TypeKind::Usize:
      isUnsigned = true;
      isSizeType = true;
      break;
    default:
      break;
    }
  }
  auto returnType = [&]() {
    if (isSizeType && isUnsigned)
      return Type(TypeKind::Usize);

    if (isSizeType && !isUnsigned)
      return Type(TypeKind::Isize);

    if (!isSizeType && isUnsigned)
      return Type(TypeKind::U32);

    return Type(TypeKind::I32);
  };
  auto calculateLiteral = [&](bool isNeg, bool isUnsigned, int base) {
    long long value = 0;
    for (char c : literal) {
      value *= base;
      long long dexValue;
      if (c >= '0' && c <= '9') {
        dexValue = c - '0';
      } else if (c >= 'a' && c <= 'f') {
        dexValue = 10 + c - 'a';
      } else {
        dexValue = 10 + c - 'A';
      }
      value += dexValue;
      if (isNeg) {
        if (isUnsigned) {
          if (value > 4294967295) {
            return false;
          }
        } else {
          if (value > 2147483648) {
            return false;
          }
        }
      } else {
        if (isUnsigned) {
          if (value > 4294967295) {
            return false;
          }
        } else {
          if (value > 2147483647) {
            return false;
          }
        }
      }
    }
    return true;
  };
  auto pos = literal.find_first_not_of('0');
  if (pos == std::string::npos) {
    return returnType();
  }
  literal = literal.substr(pos);
  if (calculateLiteral(isNegative, isUnsigned, base)) {
    return returnType();
  } else {
    error.occur(position, "Invalid Number: exceed the range");
    return Type(TypeKind::Error);
  }
}

long long SemanticAnalyzer::parseIntegerLiteral(std::string literal) {
  literal.erase(std::remove(literal.begin(), literal.end(), '_'),
                literal.end());
  int base;
  if (literal.starts_with("0x")) {
    base = 16;
  } else if (literal.starts_with("0o")) {
    base = 8;
  } else if (literal.starts_with("0b")) {
    base = 2;
  } else {
    base = 10;
  }
  if (base != 10) {
    literal.erase(0, 2);
  }
  if (literal.ends_with("i32")) {
    literal.erase(literal.size() - 3);
  } else if (literal.ends_with("isize")) {
    literal.erase(literal.size() - 5);
  } else if (literal.ends_with("u32")) {
    literal.erase(literal.size() - 3);
  } else if (literal.ends_with("usize")) {
    literal.erase(literal.size() - 5);
  }
  return std::stoll(literal, 0, base);
}

std::any SemanticAnalyzer::visitUnitType(UnitTypeNode *node) {
  return Type(TypeKind::Unit);
}

std::any SemanticAnalyzer::visitPathInType(PathInTypeNode *node) {
  if (node->path.segments.size() > 1) {
    error.occur(node->pos, "Invalid Type: too many path arguments");
    return Type(TypeKind::Error);
  }
  const auto &seg = node->path.segments.front();
  if (seg.pathIdentSegment == "i32" || seg.pathIdentSegment == "u32" ||
      seg.pathIdentSegment == "isize" || seg.pathIdentSegment == "usize" ||
      seg.pathIdentSegment == "bool") {
    if (!seg.args.genericArgs.empty() || seg.args.hasLifeTime) {
      error.occur(node->pos, "Invalid Type: cannot receive generic args");
      return Type(TypeKind::Error);
    }
    if (seg.pathIdentSegment == "i32") {
      return Type(TypeKind::I32);
    } else if (seg.pathIdentSegment == "isize") {
      return Type(TypeKind::Isize);
    } else if (seg.pathIdentSegment == "u32") {
      return Type(TypeKind::U32);
    } else if (seg.pathIdentSegment == "usize") {
      return Type(TypeKind::Usize);
    } else {
      return Type(TypeKind::Bool);
    }
  }
  if (seg.pathIdentSegment == "Vec" || seg.pathIdentSegment == "Box") {
    if (seg.args.genericArgs.size() != 1u || seg.args.hasLifeTime) {
      error.occur(node->pos, "Invalid Type: invalid generic args");
      return Type(TypeKind::Error);
    }
    Type genericType;
    genericType.elementType = std::make_unique<Type>(std::any_cast<Type>(
        seg.args.genericArgs.front().genericArgType->accept(this)));
    if (genericType.elementType->kind != TypeKind::Error) {
      genericType.kind =
          seg.pathIdentSegment == "Vec" ? TypeKind::Vec : TypeKind::Box;
    } else {
      genericType.kind = TypeKind::Error;
    }
    return genericType;
  }
  if (!seg.args.genericArgs.empty()) {
    error.occur(node->pos,
                "Invalid Type: struct cannot receive type arguments");
    return Type(TypeKind::Error);
  }
  Type structType(TypeKind::Struct);
  structType.structName = node->path.segments.front().pathIdentSegment;
  return structType;
}

std::any SemanticAnalyzer::visitReferenceType(ReferenceTypeNode *node) {
  Type refType;
  refType.elementType =
      std::make_unique<Type>(std::any_cast<Type>(node->refType->accept(this)));
  if (refType.elementType->kind != TypeKind::Error) {
    refType.kind =
        node->isMut ? TypeKind::MutReference : TypeKind::ImmutReference;
  } else {
    refType.kind = TypeKind::Error;
  }
  return refType;
}

std::any SemanticAnalyzer::visitArrayType(ArrayTypeNode *node) {
  Type type(TypeKind::Array);
  type.elementType = std::make_unique<Type>(
      std::any_cast<Type>(node->typeParam->accept(this)));

  if (type.elementType->kind == TypeKind::Error) {
    return Type(TypeKind::Error);
  }

  type.arrayLen.emplace();
  type.arrayLen->ctx = node->length.get();
  resolveArrayLengths(type);
  return type;
}

std::any SemanticAnalyzer::visitCrate(CrateNode *node) {
  for (std::size_t i = 0; i < node->items.size(); ++i) {
    node->items[i]->accept(this);
  }
  return {};
}

std::any SemanticAnalyzer::visitFunctionDeclItem(FunctionDeclItemNode *node) {
  auto *structScope = dynamic_cast<implementationScope *>(currentScope.get());
  auto modifySelfName = [&](Type &type) {
    auto *visitorPtr = &type;
    while (visitorPtr->elementType) {
      visitorPtr = visitorPtr->elementType.get();
    }
    if (visitorPtr->structName && visitorPtr->structName == "Self") {
      if (structScope == nullptr) {
        error.occur(node->pos,
                    "Invalid Function: Self type cannot exist in top function");
      } else {
        visitorPtr->structName = structScope->structName;
      }
    }
  };
  Type returnType(TypeKind::Unit);
  if (node->returnType != std::nullopt) {
    returnType = std::any_cast<Type>(node->returnType->get()->accept(this));
    modifySelfName(returnType);
  }
  auto *functionSymbol =
      structScope ? structScope->lookupFunction(node->identifier)
                  : globalScopeVisitor->lookupFunction(node->identifier);
  if (functionSymbol) {
    functionSymbol->returnType = returnType;
  }
  auto parent = std::move(currentScope);
  currentScope = std::make_unique<Scope>(parent.get());
  for (auto &param : node->functionParam) {
    if (currentScope->lookup(param.identifier)) {
      error.occur(node->pos, "Invalid Function: duplicated parameter name");
    } else {
      Type paramType = std::any_cast<Type>(param.paramType->accept(this));
      modifySelfName(paramType);
      currentScope->define(
          param.identifier,
          LocalSymbol(node->pos, param.identifier, param.isMut, paramType));
    }
  }
  node->functionBody->accept(this);
  currentScope = std::move(parent);
  return {};
}

void SemanticAnalyzer::analyzeConstantDecl(ConstantSymbol *constant) {
  auto *node = constant->node;
  auto *globalScope = this->globalScopeVisitor;
  auto *constantSymbol = globalScopeVisitor->lookupConstant(node->identifier);
  if (!constantSymbol || constantSymbol->node != node) {
    return;
  }
  Type declaredType = std::any_cast<Type>(node->constType->accept(this));
  auto initialzer = analyzeConstExpr(node->constValue.get(), &declaredType);
  Type initializerType = initialzer.first;
  if (declaredType.kind == TypeKind::Error ||
      initializerType.kind == TypeKind::Error) {
    return;
  }
  // A constant is either bool or one of the four integers, so both sides only
  // ever carry a kind: struct name, element type and array length stay empty.
  if (declaredType.kind != initializerType.kind) {
    error.occur(node->pos, "Invalid Constant: type mismatch");
    constantSymbol->constantType = Type(TypeKind::Error);
  } else {
    constant->dependency = initialzer.second;
  }
}

void SemanticAnalyzer::analyzeConstantDecl(ConstantSymbol *constant,
                                           StructSymbol *owner) {
  auto *node = constant->node;
  auto iterator = owner->structSymbols.find(node->identifier);
  auto *constantSymbol = dynamic_cast<ConstantSymbol *>(iterator->second.get());
  if (!constantSymbol || constantSymbol->node != node) {
    return;
  }
  Type declaredType = std::any_cast<Type>(node->constType->accept(this));
  auto initialzer = analyzeConstExpr(node->constValue.get(), &declaredType);
  Type initializerType = initialzer.first;
  if (declaredType.kind == TypeKind::Error ||
      initializerType.kind == TypeKind::Error) {
    return;
  }
  // A constant is either bool or one of the four integers, so both sides only
  // ever carry a kind: struct name, element type and array length stay empty.
  if (declaredType.kind != initializerType.kind) {
    error.occur(node->pos, "Invalid Constant: type mismatch");
    constantSymbol->constantType = Type(TypeKind::Error);
  } else {
    constant->dependency = initialzer.second;
  }
}

auto SemanticAnalyzer::analyzeMagnitudeExpr(MagnitudeExprNode *node)
    -> std::pair<Type, ConstantSymbol *> {
  if (std::holds_alternative<std::string>(node->magnitude)) {
    return {parseIntegerLiteral(node->pos,
                                std::get<std::string>(node->magnitude), &error,
                                true),
            nullptr};
  } else {
    auto *pathInExpr =
        std::get<std::unique_ptr<PathInExprNode>>(node->magnitude).get();
    if (pathInExpr->path.segments.size() != 1 &&
        pathInExpr->path.segments.size() != 2) {
      error.occur(pathInExpr->pos,
                  "Invalid Constant Value: invalid constant path");
      return {Type(TypeKind::Error), nullptr};
    }
    for (const auto &segment : pathInExpr->path.segments) {
      if (!segment.args.genericArgs.empty() || segment.args.hasLifeTime) {
        error.occur(pathInExpr->pos,
                    "Invalid Constant Value: invalid constant path");
        return {Type(TypeKind::Error), nullptr};
      }
    }
    if (pathInExpr->path.segments.size() == 1) {
      const auto &seg = pathInExpr->path.segments.front();
      if (seg.pathIdentSegment == "Self" || seg.pathIdentSegment == "self") {
        error.occur(pathInExpr->pos,
                    "Invalid Constant Value: invalid constant item");
        return {Type(TypeKind::Error), nullptr};
      } else {
        auto *implScope = findImplementationScope(currentScope.get());
        if (auto *constantMember =
                implScope ? implScope->lookupConstant(seg.pathIdentSegment)
                          : nullptr;
            constantMember) {
          if (auto &kind = constantMember->constantType.kind;
              kind != TypeKind::I32 && kind != TypeKind::U32 &&
              kind != TypeKind::Isize && kind != TypeKind::Usize) {
            error.occur(pathInExpr->pos,
                        "Invalid Magnitude Value: magnitude must be number");
            return {Type(TypeKind::Error), nullptr};
          } else {
            return {constantMember->constantType, constantMember};
          }
        } else if (auto *constantItem =
                       globalScopeVisitor->lookupConstant(seg.pathIdentSegment);
                   constantItem) {
          if (auto &kind = constantItem->constantType.kind;
              kind != TypeKind::I32 && kind != TypeKind::U32 &&
              kind != TypeKind::Isize && kind != TypeKind::Usize) {
            error.occur(pathInExpr->pos,
                        "Invalid Magnitude Value: magnitude must be number");
            return std::pair{Type(TypeKind::Error), nullptr};
          } else {
            return {constantItem->constantType, constantItem};
          }
        } else {
          error.occur(pathInExpr->pos,
                      "Invalid Constant Value: constant path not found");
          return {Type(TypeKind::Error), nullptr};
        }
      }
    } else {
      auto *implScope = findImplementationScope(currentScope.get());
      const auto &structPath = pathInExpr->path.segments.front();
      const auto &constantPath = pathInExpr->path.segments.back();
      if (structPath.pathIdentSegment == "self") {
        error.occur(pathInExpr->pos,
                    "Invalid Constant Value: invalid constant path");
        return {Type(TypeKind::Error), nullptr};
      } else if (structPath.pathIdentSegment == "Self") {
        if (!implScope) {
          error.occur(pathInExpr->pos,
                      "Invalid Constant Value: invalid constant path");
          return {Type(TypeKind::Error), nullptr};
        }
        if (auto *constantMember =
                implScope->lookupConstant(constantPath.pathIdentSegment);
            constantMember) {
          if (auto &kind = constantMember->constantType.kind;
              kind != TypeKind::I32 && kind != TypeKind::U32 &&
              kind != TypeKind::Isize && kind != TypeKind::Usize) {
            error.occur(pathInExpr->pos,
                        "Invalid Magnitude Value: magnitude must be number");
            return {Type(TypeKind::Error), nullptr};
          } else {
            return {constantMember->constantType, constantMember};
          }
        } else {
          error.occur(pathInExpr->pos,
                      "Invalid Constant Value: constant path not found");
          return {Type(TypeKind::Error), nullptr};
        }
      } else {
        if (auto *structItem =
                globalScopeVisitor->lookupStruct(structPath.pathIdentSegment);
            structItem) {
          if (structItem->structSymbols.contains(
                  constantPath.pathIdentSegment)) {
            if (auto *constantAssociated = dynamic_cast<ConstantSymbol *>(
                    structItem->structSymbols[constantPath.pathIdentSegment]
                        .get());
                constantAssociated) {
              if (auto &kind = constantAssociated->constantType.kind;
                  kind != TypeKind::I32 && kind != TypeKind::U32 &&
                  kind != TypeKind::Isize && kind != TypeKind::Usize) {
                error.occur(
                    pathInExpr->pos,
                    "Invalid Magnitude Value: magnitude must be number");
                return {Type(TypeKind::Error), nullptr};
              } else {
                return {constantAssociated->constantType, constantAssociated};
              }
            } else {
              error.occur(pathInExpr->pos,
                          "Invalid Constant Value: constant path not found");
              return {Type(TypeKind::Error), nullptr};
            }
          } else {
            error.occur(pathInExpr->pos,
                        "Invalid Constant Value: constant path not found");
            return {Type(TypeKind::Error), nullptr};
          }
        } else {
          error.occur(pathInExpr->pos,
                      "Invalid Constant Value: constant path not found");
          return {Type(TypeKind::Error), nullptr};
        }
      }
    }
  }
}

auto SemanticAnalyzer::analyzeConstExpr(ConstExprNode *node,
                                        const Type *expected)
    -> std::pair<Type, ConstantSymbol *> {
  if (node->isNegative) {
    auto magPair = analyzeMagnitudeExpr(
        std::get<std::unique_ptr<MagnitudeExprNode>>(node->constValue).get());
    auto &magType = magPair.first;
    if (magType.kind == TypeKind::U32 || magType.kind == TypeKind::Usize) {
      error.occur(node->pos,
                  "Invalid Constant Value: unsigned number cannot be negative");
      return {Type(TypeKind::Error), nullptr};
    } else {
      return magPair;
    }
  } else if (std::holds_alternative<std::unique_ptr<LiteralExprNode>>(
                 node->constValue)) {
    auto *literalPtr =
        std::get<std::unique_ptr<LiteralExprNode>>(node->constValue).get();
    if (literalPtr->literalValue == "true" ||
        literalPtr->literalValue == "false") {
      return {Type(TypeKind::Bool), nullptr};
    } else {
      return {parseIntegerLiteral(node->pos, literalPtr->literalValue, &error,
                                  false, expected),
              nullptr};
    }
  } else {
    auto *pathInExpr =
        std::get<std::unique_ptr<PathInExprNode>>(node->constValue).get();
    if (pathInExpr->path.segments.size() != 1 &&
        pathInExpr->path.segments.size() != 2) {
      error.occur(pathInExpr->pos,
                  "Invalid Constant Value: invalid constant path");
      return {Type(TypeKind::Error), nullptr};
    }
    for (const auto &segment : pathInExpr->path.segments) {
      if (!segment.args.genericArgs.empty() || segment.args.hasLifeTime) {
        error.occur(pathInExpr->pos,
                    "Invalid Constant Value: invalid constant path");
        return {Type(TypeKind::Error), nullptr};
      }
    }
    if (pathInExpr->path.segments.size() == 1) {
      const auto &seg = pathInExpr->path.segments.front();
      if (seg.pathIdentSegment == "Self" || seg.pathIdentSegment == "self") {
        error.occur(pathInExpr->pos,
                    "Invalid Constant Value: invalid constant item");
        return {Type(TypeKind::Error), nullptr};
      } else {
        auto *implScope = findImplementationScope(currentScope.get());
        if (auto *constantMember =
                implScope ? implScope->lookupConstant(seg.pathIdentSegment)
                          : nullptr;
            constantMember) {
          return {constantMember->constantType, constantMember};
        } else if (auto *constantItem =
                       globalScopeVisitor->lookupConstant(seg.pathIdentSegment);
                   constantItem) {
          return {constantItem->constantType, constantItem};
        } else {
          error.occur(pathInExpr->pos,
                      "Invalid Constant Value: constant path not found");
          return {Type(TypeKind::Error), nullptr};
        }
      }
    } else {
      auto *implScope = findImplementationScope(currentScope.get());
      const auto &structPath = pathInExpr->path.segments.front();
      const auto &constantPath = pathInExpr->path.segments.back();
      if (structPath.pathIdentSegment == "self") {
        error.occur(pathInExpr->pos,
                    "Invalid Constant Value: invalid constant path");
        return {Type(TypeKind::Error), nullptr};
      } else if (structPath.pathIdentSegment == "Self") {
        if (!implScope) {
          error.occur(pathInExpr->pos,
                      "Invalid Constant Value: invalid constant path");
          return {Type(TypeKind::Error), nullptr};
        }
        if (auto *constantMember =
                implScope->lookupConstant(constantPath.pathIdentSegment);
            constantMember) {
          return {constantMember->constantType, constantMember};
        } else {
          error.occur(pathInExpr->pos,
                      "Invalid Constant Value: constant path not found");
          return {Type(TypeKind::Error), nullptr};
        }
      } else {
        if (auto *structItem =
                globalScopeVisitor->lookupStruct(structPath.pathIdentSegment);
            structItem) {
          if (structItem->structSymbols.contains(
                  constantPath.pathIdentSegment)) {
            if (auto *constantAssociated = dynamic_cast<ConstantSymbol *>(
                    structItem->structSymbols[constantPath.pathIdentSegment]
                        .get());
                constantAssociated) {
              return {constantAssociated->constantType, constantAssociated};
            } else {
              error.occur(pathInExpr->pos,
                          "Invalid Constant Value: constant path not found");
              return {Type(TypeKind::Error), nullptr};
            }
          } else {
            error.occur(pathInExpr->pos,
                        "Invalid Constant Value: constant path not found");
            return {Type(TypeKind::Error), nullptr};
          }
        } else {
          error.occur(pathInExpr->pos,
                      "Invalid Constant Value: constant path not found");
          return {Type(TypeKind::Error), nullptr};
        }
      }
    }
  }
}

auto SemanticAnalyzer::evaluateConstantSymbol(ConstantSymbol *symbol)
    -> const std::pair<Type, std::variant<std::monostate, long long, bool>> {
  auto *initializer = symbol->node->constValue.get();
  if (symbol->dependency == nullptr ||
      symbol->value.state == EvaluatedState::Evaluated) {
    if (symbol->dependency == nullptr) {
      if (symbol->constantType.kind == TypeKind::Bool) {
        symbol->value.value =
            std::get<std::unique_ptr<LiteralExprNode>>(initializer->constValue)
                        ->literalValue == "true"
                ? true
                : false;
      } else {
        if (initializer->isNegative) {
          symbol->value.value = -parseIntegerLiteral(std::get<std::string>(
              std::get<std::unique_ptr<MagnitudeExprNode>>(
                  initializer->constValue)
                  ->magnitude));
        } else {
          symbol->value.value =
              parseIntegerLiteral(std::get<std::unique_ptr<LiteralExprNode>>(
                                      initializer->constValue)
                                      ->literalValue);
        }
      }
      symbol->value.state = EvaluatedState::Evaluated;
    }
    return {symbol->constantType, symbol->value.value};
  }
  if (symbol->value.state == EvaluatedState::Evaluating) {
    error.occur(symbol->node->pos,
                "Invalid Constant: cyclic dependency in constant evaluation");
    symbol->constantType = Type(TypeKind::Error);
    return {symbol->constantType, std::monostate{}};
  }
  symbol->value.state = EvaluatedState::Evaluating;
  const auto &evaluateInfo = evaluateConstantSymbol(symbol->dependency);
  symbol->value.state = EvaluatedState::Evaluated;
  if (evaluateInfo.first.kind == TypeKind::Error) {
    symbol->constantType = Type(TypeKind::Error);
    return {symbol->constantType, std::monostate{}};
  }
  if (initializer->isNegative) {
    symbol->value.value = -std::get<long long>(evaluateInfo.second);
  } else {
    symbol->value.value = evaluateInfo.second;
  }
  return {symbol->constantType, symbol->value.value};
}

void SemanticAnalyzer::evaluate() {
  for (const auto &globalSymbol : globalScopeVisitor->valueNameSpace) {
    if (auto *constantSymbol =
            dynamic_cast<ConstantSymbol *>(globalSymbol.second.get())) {
      if (constantSymbol->value.state == EvaluatedState::NotAccess) {
        evaluateConstantSymbol(constantSymbol);
      }
    }
  }
  for (const auto &globalSymbol : globalScopeVisitor->typeNameSpace) {
    if (auto *structSymbol =
            dynamic_cast<StructSymbol *>(globalSymbol.second.get())) {
      for (const auto &globalSymbol : structSymbol->structSymbols) {
        if (auto *constantSymbol =
                dynamic_cast<ConstantSymbol *>(globalSymbol.second.get())) {
          if (constantSymbol->value.state == EvaluatedState::NotAccess) {
            evaluateConstantSymbol(constantSymbol);
          }
        }
      }
    }
  }
}

void SemanticAnalyzer::analyzeConstant() {
  for (const auto &globalSymbol : globalScopeVisitor->valueNameSpace) {
    if (auto *constantSymbol =
            dynamic_cast<ConstantSymbol *>(globalSymbol.second.get())) {
      analyzeConstantDecl(constantSymbol);
    }
  }
  for (const auto &globalSymbol : globalScopeVisitor->typeNameSpace) {
    if (auto *structSymbol =
            dynamic_cast<StructSymbol *>(globalSymbol.second.get())) {
      for (const auto &globalSymbol : structSymbol->structSymbols) {
        if (auto *constantSymbol =
                dynamic_cast<ConstantSymbol *>(globalSymbol.second.get())) {
          analyzeConstantDecl(constantSymbol, structSymbol);
        }
      }
    }
  }
  if (error.hasError()) {
    return;
  }
  evaluate();
}

std::any SemanticAnalyzer::visitConstExpr(ConstExprNode *node) {
  // especially designed for array length, return Type and Value
  Type expectedType = Type(TypeKind::Usize);
  const auto &[lengthType, dependency] = analyzeConstExpr(node, &expectedType);
  if (lengthType.kind != TypeKind::Usize) {
    error.occur(node->pos, "Invalid array length: array length must be usize");
    return std::pair<Type, long long>(Type(TypeKind::Error), 0);
  } else {
    if (std::holds_alternative<std::unique_ptr<LiteralExprNode>>(
            node->constValue)) {
      auto *literal =
          std::get<std::unique_ptr<LiteralExprNode>>(node->constValue).get();
      return std::pair<Type, long long>(
          Type(TypeKind::Usize), parseIntegerLiteral(literal->literalValue));
    } else {
      return std::pair<Type, long long>(
          Type(TypeKind::Usize), std::get<long long>(dependency->value.value));
    }
  }
}

void SemanticAnalyzer::resolveArrayLengths(Type &type) {
  if (type.elementType) {
    resolveArrayLengths(*type.elementType);
  }
  if (type.kind != TypeKind::Array || !type.arrayLen) {
    return;
  }
  auto &length = *type.arrayLen;
  if (length.state == EvaluatedState::Evaluated) {
    return;
  }
  auto [resultType, value] =
      std::any_cast<std::pair<Type, long long>>(visitConstExpr(length.ctx));
  if (resultType.kind == TypeKind::Error) {
    type.kind = TypeKind::Error;
    return;
  }
  length.value = value;
  length.state = EvaluatedState::Evaluated;
}

void SemanticAnalyzer::preprocessArrayLengths() {
  for (const auto &globalSymbol : globalScopeVisitor->valueNameSpace) {
    if (auto *functionSymbol =
            dynamic_cast<FunctionSymbol *>(globalSymbol.second.get())) {
      resolveArrayLengths(functionSymbol->returnType);
      for (auto &param : functionSymbol->paramTypes) {
        resolveArrayLengths(param);
      }
    }
  }
  for (const auto &globalSymbol : globalScopeVisitor->typeNameSpace) {
    if (auto *structSymbol =
            dynamic_cast<StructSymbol *>(globalSymbol.second.get())) {
      auto parent = std::move(currentScope);
      auto implScope = std::make_unique<implementationScope>(
          structSymbol->structSymbols, globalScopeVisitor->typeNameSpace);
      implScope->structName = structSymbol->name;
      implScope->parentScope = parent.get();
      currentScope = std::move(implScope);
      for (auto &[name, symbol] : structSymbol->structSymbols) {
        if (auto *function = dynamic_cast<FunctionSymbol *>(symbol.get())) {
          resolveArrayLengths(function->returnType);
          for (auto &param : function->paramTypes) {
            resolveArrayLengths(param);
          }
        }
      }
      for (auto &[name, type] : structSymbol->structFields) {
        resolveArrayLengths(type);
      }
      currentScope = std::move(parent);
    }
  }
}

