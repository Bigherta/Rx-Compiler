#include "SemanticAnalyzer.hpp"
#include "AST.hpp"
#include "Scope.hpp"
#include "SemanticResult.hpp"
#include "SymbolCollector.hpp"
#include <algorithm>
#include <any>
#include <cstddef>
#include <memory>
#include <optional>
#include <queue>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>
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
                                           std::string literal, bool isNegative,
                                           Type *expected) {
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
    error.occur(position,
                "Invalid Number: integer literal is outside the range "
                "of its selected integer type");
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

FunctionSymbol *SemanticAnalyzer::getOrCreateBuiltinClone(const Type &type) {
  if (!hasTrait(type, BuiltinTrait::Clone)) {
    return nullptr;
  }
  const auto linkedName = encodeType(type) + "_clone";
  if (auto iterator = builtinFunctions.find(linkedName);
      iterator != builtinFunctions.end()) {
    return iterator->second.get();
  }
  Type receiverType(TypeKind::ImmutReference);
  receiverType.elementType = std::make_unique<Type>(type);
  auto symbol = std::make_unique<FunctionSymbol>(
      linkedName, type, std::vector<Type>{receiverType});
  auto *function = symbol.get();
  function->receiver = ReceiverKind::Shared;
  builtinFunctions.emplace(linkedName, std::move(symbol));
  return function;
}

FunctionSymbol *SemanticAnalyzer::getOrCreateBuiltinContainerFunction(
    const Type &containerType, const std::string &methodName) {
  if (containerType.kind != TypeKind::Box &&
      containerType.kind != TypeKind::Vec) {
    return nullptr;
  }
  const auto manglingName = encodeType(containerType) + "_" + methodName;
  if (auto iterator = builtinFunctions.find(manglingName);
      iterator != builtinFunctions.end()) {
    return iterator->second.get();
  }
  const auto &argumentType = *containerType.elementType;
  Type returnType(TypeKind::Unit);
  std::vector<Type> paramTypes;
  ReceiverKind receiver = ReceiverKind::None;
  if (methodName == "new") {
    returnType = containerType;
    if (containerType.kind == TypeKind::Box) {
      paramTypes.push_back(argumentType);
    }
  } else if (containerType.kind == TypeKind::Vec &&
             (methodName == "len" || methodName == "is_empty")) {
    receiver = ReceiverKind::Shared;
    Type receiverType(TypeKind::ImmutReference);
    receiverType.elementType = std::make_unique<Type>(containerType);
    paramTypes.push_back(std::move(receiverType));
    returnType = Type(methodName == "len" ? TypeKind::Usize : TypeKind::Bool);
  } else if (containerType.kind == TypeKind::Vec &&
             (methodName == "push" || methodName == "remove")) {
    receiver = ReceiverKind::Mutable;
    Type receiverType(TypeKind::MutReference);
    receiverType.elementType = std::make_unique<Type>(containerType);
    paramTypes.push_back(std::move(receiverType));
    if (methodName == "push") {
      paramTypes.push_back(argumentType);
    } else {
      paramTypes.emplace_back(TypeKind::Usize);
      returnType = argumentType;
    }
  } else if (methodName == "clone") {
    if (!hasTrait(containerType, BuiltinTrait::Clone)) {
      return nullptr;
    }
    receiver = ReceiverKind::Shared;
    Type receiverType(TypeKind::ImmutReference);
    receiverType.elementType = std::make_unique<Type>(containerType);
    paramTypes.emplace_back(receiverType);
    returnType = containerType;
  } else {
    return nullptr;
  }
  const std::string containerName =
      containerType.kind == TypeKind::Box ? "Box" : "Vec";
  auto symbol = std::make_unique<FunctionSymbol>(
      containerName + "::" + methodName, returnType, paramTypes);
  auto *function = symbol.get();
  function->receiver = receiver;
  builtinFunctions.emplace(manglingName, std::move(symbol));
  return function;
}

FunctionSymbol *SemanticAnalyzer::getOrCreateBuiltinArrayFunction(
    const Type &ArrayType, const std::string &methodName) {
  if (ArrayType.kind != TypeKind::Array) {
    return nullptr;
  }
  const auto manglingName = encodeType(ArrayType) + "_" + methodName;
  if (auto iterator = builtinFunctions.find(manglingName);
      iterator != builtinFunctions.end()) {
    return iterator->second.get();
  }
  Type returnType(TypeKind::Usize);
  std::vector<Type> paramTypes;
  if (methodName == "len") {
    Type receiverType(TypeKind::ImmutReference);
    receiverType.elementType = std::make_unique<Type>(ArrayType);
    paramTypes.push_back(std::move(receiverType));
  } else {
    return nullptr;
  }
  auto symbol = std::make_unique<FunctionSymbol>("Array::" + methodName,
                                                 returnType, paramTypes);
  auto *function = symbol.get();
  function->receiver = ReceiverKind::Shared;
  builtinFunctions.emplace(manglingName, std::move(symbol));
  return function;
}

std::string SemanticAnalyzer::encodeType(const Type &type) const {
  switch (type.kind) {
  case TypeKind::I32:
    return "I32";
  case TypeKind::U32:
    return "U32";
  case TypeKind::Isize:
    return "Isize";
  case TypeKind::Usize:
    return "Usize";
  case TypeKind::Bool:
    return "Bool";
  case TypeKind::Unit:
    return "Unit";
  case TypeKind::Never:
    return "Never";
  case TypeKind::ImmutReference:
    return "_Ref_" + encodeType(*type.elementType);
  case TypeKind::MutReference:
    return "_Ref_Mut_" + encodeType(*type.elementType);
  case TypeKind::Box:
    return "_Box_" + encodeType(*type.elementType);
  case TypeKind::Vec:
    return "_Vec_" + encodeType(*type.elementType);
  case TypeKind::Array:
    return "_Array_" +
           std::to_string(std::get<long long>(type.arrayLen->value)) + "_" +
           encodeType(*type.elementType);
  case TypeKind::Struct: {
    const auto &name = type.structName.value();
    return "_Struct_" + std::to_string(name.size()) + "_" + name;
  }
  case TypeKind::Error:
  case TypeKind::Function:
    throw std::invalid_argument(
        "encodeType requires a resolved, non-error, non-function type");
  }
  std::unreachable();
}

Type SemanticAnalyzer::autoDereference(const Type *refType, bool &mutablePlace,
                                       bool &mutationBlocked,
                                       std::vector<Adjustment> &adjusts) {
  adjusts.clear();
  mutablePlace &= !mutationBlocked;
  while (refType->kind == TypeKind::ImmutReference ||
         refType->kind == TypeKind::MutReference ||
         refType->kind == TypeKind::Box) {
    mutationBlocked |= refType->kind == TypeKind::ImmutReference;
    if (mutationBlocked) {
      mutablePlace = false;
    } else {
      mutablePlace |= refType->kind == TypeKind::MutReference;
    }
    adjusts.emplace_back(
        Adjustment{AdjustmentKind::Deref, *refType->elementType});
    refType = refType->elementType.get();
  }
  return *refType;
}

Type SemanticAnalyzer::saveType(TypeNode *node, Type &&type) {
  node->semantic =
      std::make_unique<TypeSemantic>(TypeSemantic{std::move(type)});
  return node->semantic->semanticType;
}

std::any SemanticAnalyzer::visitUnitType(UnitTypeNode *node) {
  return saveType(node, Type(TypeKind::Unit, node->pos));
}

std::any SemanticAnalyzer::visitPathInType(PathInTypeNode *node) {
  if (node->path.segments.size() > 1) {
    error.occur(node->pos,
                "Invalid Type: a type path must contain a single segment");
    return saveType(node, Type(TypeKind::Error, node->pos));
  }
  const auto &seg = node->path.segments.front();
  if (seg.pathIdentSegment == "i32" || seg.pathIdentSegment == "u32" ||
      seg.pathIdentSegment == "isize" || seg.pathIdentSegment == "usize" ||
      seg.pathIdentSegment == "bool") {
    if (!seg.args.genericArgs.empty() || seg.args.hasLifeTime) {
      error.occur(node->pos,
                  "Invalid Type: primitive type `" + seg.pathIdentSegment +
                      "` does not accept type or lifetime arguments");
      return saveType(node, Type(TypeKind::Error, node->pos));
    }
    if (seg.pathIdentSegment == "i32") {
      return saveType(node, Type(TypeKind::I32, node->pos));
    } else if (seg.pathIdentSegment == "isize") {
      return saveType(node, Type(TypeKind::Isize, node->pos));
    } else if (seg.pathIdentSegment == "u32") {
      return saveType(node, Type(TypeKind::U32, node->pos));
    } else if (seg.pathIdentSegment == "usize") {
      return saveType(node, Type(TypeKind::Usize, node->pos));
    } else {
      return saveType(node, Type(TypeKind::Bool, node->pos));
    }
  }
  if (seg.pathIdentSegment == "Vec" || seg.pathIdentSegment == "Box") {
    if (seg.args.genericArgs.size() != 1u || seg.args.hasLifeTime) {
      error.occur(node->pos, "Invalid Type: `" + seg.pathIdentSegment +
                                 "` requires exactly one type argument "
                                 "and no lifetime arguments");
      return saveType(node, Type(TypeKind::Error, node->pos));
    }
    Type genericType(TypeKind::Error, node->pos);
    genericType.elementType = std::make_unique<Type>(std::any_cast<Type>(
        seg.args.genericArgs.front().genericArgType->accept(this)));
    if (genericType.elementType->kind != TypeKind::Error) {
      genericType.kind =
          seg.pathIdentSegment == "Vec" ? TypeKind::Vec : TypeKind::Box;
    } else {
      genericType.kind = TypeKind::Error;
    }
    return saveType(node, std::move(genericType));
  }
  if (!seg.args.genericArgs.empty()) {
    error.occur(node->pos, "Invalid Type: struct `" + seg.pathIdentSegment +
                               "` does not accept type arguments");
    return saveType(node, Type(TypeKind::Error, node->pos));
  }
  Type structType(TypeKind::Struct, node->pos);
  const auto &structName = node->path.segments.front().pathIdentSegment;
  if (structName == "Self") {
    if (seg.args.hasLifeTime) {
      error.occur(node->pos,
                  "Invalid Type: `Self` does not accept lifetime arguments");
      return saveType(node, Type(TypeKind::Error, node->pos));
    }
    auto currentStruct = getSelfName();
    if (!currentStruct) {
      error.occur(
          node->pos,
          "Invalid Type: `Self` requires an enclosing struct implementation");
      return saveType(node, Type(TypeKind::Error, node->pos));
    } else {
      structType.structName = currentStruct;
    }
  } else {
    auto *structSymbol = globalScopeVisitor->lookupStruct(structName);
    if (structSymbol) {
      structType.structName = structName;
    } else {
      error.occur(node->pos,
                  "Invalid Type: undefined struct `" + structName + "`");
      return saveType(node, Type(TypeKind::Error, node->pos));
    }
  }
  return saveType(node, std::move(structType));
}

std::any SemanticAnalyzer::visitReferenceType(ReferenceTypeNode *node) {
  Type refType(TypeKind::Error, node->pos);
  refType.elementType =
      std::make_unique<Type>(std::any_cast<Type>(node->refType->accept(this)));
  if (refType.elementType->kind != TypeKind::Error) {
    refType.kind =
        node->isMut ? TypeKind::MutReference : TypeKind::ImmutReference;
  } else {
    refType.kind = TypeKind::Error;
  }
  return saveType(node, std::move(refType));
}

std::any SemanticAnalyzer::visitArrayType(ArrayTypeNode *node) {
  Type arrayType(TypeKind::Array, node->pos);
  arrayType.elementType = std::make_unique<Type>(
      std::any_cast<Type>(node->typeParam->accept(this)));

  if (arrayType.elementType->kind == TypeKind::Error) {
    return saveType(node, Type(TypeKind::Error, node->pos));
  }

  arrayType.arrayLen.emplace();
  arrayType.arrayLen->ctx = node->length.get();
  resolveArrayLengths(arrayType);
  return saveType(node, std::move(arrayType));
}

std::any SemanticAnalyzer::visitCrate(CrateNode *node) {
  if (!globalScopeVisitor->lookupFunction("main")) {
    error.occur(node->pos,
                "Invalid Program: no `main` function as the entrance");
  } else {
    for (std::size_t i = 0; i < node->items.size(); ++i) {
      node->items[i]->accept(this);
    }
  }
  return {};
}

std::any SemanticAnalyzer::visitImplDeclItem(ImplDeclItemNode *node) {
  auto *implPath = dynamic_cast<PathInTypeNode *>(node->implType.get());
  if (implPath == nullptr || implPath->path.segments.size() != 1) {
    error.occur(node->implType->pos,
                "Invalid Impl: the target must be a struct type "
                "named by a single path segment");
    return {};
  }
  const auto &structName = implPath->path.segments.front().pathIdentSegment;
  auto *structSymbol = globalScopeVisitor->lookupStruct(structName);
  if (structSymbol == nullptr) {
    error.occur(node->implType->pos,
                "Invalid Impl: cannot implement undefined struct `" +
                    structName + "`");
    return {};
  }

  auto parent = std::move(currentScope);
  auto implScope = std::make_unique<implementationScope>(
      structSymbol->structSymbols, globalScopeVisitor->typeNameSpace);
  implScope->structName = structName;
  implScope->parentScope = parent.get();
  currentScope = std::move(implScope);

  for (auto &item : node->associatedItems) {
    std::visit([this](auto &itemPtr) { itemPtr->accept(this); }, item);
  }

  currentScope = std::move(parent);
  return {};
}

std::any SemanticAnalyzer::visitFunctionDeclItem(FunctionDeclItemNode *node) {
  auto *structScope = dynamic_cast<implementationScope *>(currentScope.get());
  Type returnType(TypeKind::Unit);
  if (node->returnType != std::nullopt) {
    returnType = std::any_cast<Type>(node->returnType->get()->accept(this));
  }
  auto *functionSymbol =
      structScope ? structScope->lookupFunction(node->identifier)
                  : globalScopeVisitor->lookupFunction(node->identifier);
  if (functionSymbol) {
    functionSymbol->returnType = returnType;
    if (structScope) {
      functionSymbol->owner =
          globalScopeVisitor->lookupStruct(structScope->structName);
    }
  }
  auto _ = ScopeGuard(currentScope);
  if (node->selfParam != std::nullopt) {
    if (structScope == nullptr) {
      error.occur(node->pos, "Invalid Function: a top-level function cannot "
                             "have a `self` parameter");
    } else {
      const auto &self = node->selfParam.value();
      Type selfType(structScope->structName);
      if (self.isRef) {
        Type refType;
        refType.kind =
            self.isMut ? TypeKind::MutReference : TypeKind::ImmutReference;
        refType.elementType = std::make_unique<Type>(selfType);
        selfType = std::move(refType);
      }
      localBindings.emplace_back(std::make_unique<LocalSymbol>(
          node->pos, "self", self.isMut && !self.isRef, selfType));
      currentScope->define("self", *localBindings.back());
      node->selfParam->paramSymbol = localBindings.back().get();
    }
  }
  for (auto &param : node->functionParam) {
    if (currentScope->lookup(param.identifier)) {
      error.occur(node->pos, "Invalid Function: parameter name `" +
                                 param.identifier +
                                 "` conflicts with an existing binding");
    } else {
      Type paramType = std::any_cast<Type>(param.paramType->accept(this));
      localBindings.emplace_back(std::make_unique<LocalSymbol>(
          node->pos, param.identifier, param.isMut, paramType));
      currentScope->define(param.identifier, *localBindings.back());
      param.paramSymbol = localBindings.back().get();
    }
  }
  auto _ = FunctionGuard(functionInfoContainer, FunctionContext(&returnType));
  checkExpr(node->functionBody.get(), &returnType);
  return {};
}

void SemanticAnalyzer::analyzeConstantDecl(ConstantSymbol *constant) {
  auto *node = constant->node;
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
    error.occur(node->pos, "Invalid Constant: the initializer of `" +
                               node->identifier +
                               "` must match its declared type");
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

  auto parent = std::move(currentScope);
  auto implScope = std::make_unique<implementationScope>(
      owner->structSymbols, globalScopeVisitor->typeNameSpace);
  implScope->structName = owner->name;
  implScope->parentScope = parent.get();
  currentScope = std::move(implScope);

  Type declaredType = std::any_cast<Type>(node->constType->accept(this));
  auto initialzer = analyzeConstExpr(node->constValue.get(), &declaredType);
  Type initializerType = initialzer.first;
  if (declaredType.kind == TypeKind::Error ||
      initializerType.kind == TypeKind::Error) {
    currentScope = std::move(parent);
    return;
  }
  // A constant is either bool or one of the four integers, so both sides only
  // ever carry a kind: struct name, element type and array length stay empty.
  if (declaredType.kind != initializerType.kind) {
    error.occur(node->pos, "Invalid Constant: the initializer of `" +
                               node->identifier +
                               "` must match its declared type");
    constantSymbol->constantType = Type(TypeKind::Error);
  } else {
    constant->dependency = initialzer.second;
  }
  currentScope = std::move(parent);
}

auto SemanticAnalyzer::analyzeMagnitudeExpr(MagnitudeExprNode *node)
    -> std::pair<Type, ConstantSymbol *> {
  node->semantic = std::make_unique<ExprSemantic>();
  if (std::holds_alternative<std::string>(node->magnitude)) {
    auto type = parseIntegerLiteral(
        node->pos, std::get<std::string>(node->magnitude), true);
    node->semantic->originalType = type;
    node->semantic->adjustedType = type;
    return {type, nullptr};
  } else {
    auto *pathInExpr =
        std::get<std::unique_ptr<PathInExprNode>>(node->magnitude).get();
    pathInExpr->semantic = std::make_unique<ValuePathInExprSemantic>();
    auto *pathSemantic =
        static_cast<ValuePathInExprSemantic *>(pathInExpr->semantic.get());
    if (pathInExpr->path.segments.size() != 1 &&
        pathInExpr->path.segments.size() != 2) {
      error.occur(pathInExpr->pos,
                  "Invalid Constant Value: a constant path must be "
                  "`NAME` or `Type::NAME`");
      return {Type(TypeKind::Error), nullptr};
    }
    for (std::size_t i = 0; i < pathInExpr->path.segments.size(); ++i) {
      const auto &segment = pathInExpr->path.segments[i];
      const bool isStructQualifier = pathInExpr->path.segments.size() == 2 &&
                                     i == 0 &&
                                     segment.pathIdentSegment != "Self" &&
                                     segment.pathIdentSegment != "self";
      if (!segment.args.genericArgs.empty() ||
          (segment.args.hasLifeTime && !isStructQualifier)) {
        error.occur(pathInExpr->pos,
                    "Invalid Constant Value: constant names do not accept "
                    "type or lifetime arguments; struct qualifiers only "
                    "accept lifetime arguments");
        return {Type(TypeKind::Error), nullptr};
      }
    }
    if (pathInExpr->path.segments.size() == 1) {
      const auto &seg = pathInExpr->path.segments.front();
      if (seg.pathIdentSegment == "Self" || seg.pathIdentSegment == "self") {
        error.occur(pathInExpr->pos,
                    "Invalid Constant Value: `Self` and `self` alone "
                    "do not name a constant");
        return {Type(TypeKind::Error), nullptr};
      } else {
        if (auto *constantItem =
                globalScopeVisitor->lookupConstant(seg.pathIdentSegment);
            constantItem) {
          pathSemantic->bindingSymbol = constantItem;
          pathSemantic->originalType = constantItem->constantType;
          pathSemantic->adjustedType = constantItem->constantType;
          if (auto &kind = constantItem->constantType.kind;
              kind != TypeKind::I32 && kind != TypeKind::U32 &&
              kind != TypeKind::Isize && kind != TypeKind::Usize) {
            error.occur(pathInExpr->pos,
                        "Invalid Constant Value: the operand of unary `-` "
                        "must have an integer type");
            return std::pair{Type(TypeKind::Error), nullptr};
          } else {
            node->semantic->originalType = constantItem->constantType;
            node->semantic->adjustedType = constantItem->constantType;
            return {constantItem->constantType, constantItem};
          }
        } else {
          error.occur(pathInExpr->pos,
                      "Invalid Constant Value: undefined constant `" +
                          seg.pathIdentSegment + "`");
          return {Type(TypeKind::Error), nullptr};
        }
      }
    } else {
      auto *implScope = findImplementationScope(currentScope.get());
      const auto &structPath = pathInExpr->path.segments.front();
      const auto &constantPath = pathInExpr->path.segments.back();
      if (structPath.pathIdentSegment == "self") {
        error.occur(pathInExpr->pos,
                    "Invalid Constant Value: `self` cannot qualify a constant "
                    "path; use a struct type or `Self`");
        return {Type(TypeKind::Error), nullptr};
      } else if (structPath.pathIdentSegment == "Self") {
        if (!implScope) {
          error.occur(pathInExpr->pos,
                      "Invalid Constant Value: `Self::NAME` requires "
                      "an enclosing struct implementation");
          return {Type(TypeKind::Error), nullptr};
        }
        if (auto *constantMember =
                implScope->lookupConstant(constantPath.pathIdentSegment);
            constantMember) {
          pathSemantic->bindingSymbol = constantMember;
          pathSemantic->originalType = constantMember->constantType;
          pathSemantic->adjustedType = constantMember->constantType;
          if (auto &kind = constantMember->constantType.kind;
              kind != TypeKind::I32 && kind != TypeKind::U32 &&
              kind != TypeKind::Isize && kind != TypeKind::Usize) {
            error.occur(pathInExpr->pos,
                        "Invalid Constant Value: the operand of unary `-` "
                        "must have an integer type");
            return {Type(TypeKind::Error), nullptr};
          } else {
            node->semantic->originalType = constantMember->constantType;
            node->semantic->adjustedType = constantMember->constantType;
            return {constantMember->constantType, constantMember};
          }
        } else {
          error.occur(
              pathInExpr->pos,
              "Invalid Constant Value: undefined associated constant `Self::" +
                  constantPath.pathIdentSegment + "`");
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
              pathSemantic->bindingSymbol = constantAssociated;
              pathSemantic->originalType = constantAssociated->constantType;
              pathSemantic->adjustedType = constantAssociated->constantType;
              if (auto &kind = constantAssociated->constantType.kind;
                  kind != TypeKind::I32 && kind != TypeKind::U32 &&
                  kind != TypeKind::Isize && kind != TypeKind::Usize) {
                error.occur(pathInExpr->pos,
                            "Invalid Constant Value: the operand of unary `-` "
                            "must have an integer type");
                return {Type(TypeKind::Error), nullptr};
              } else {
                node->semantic->originalType = constantAssociated->constantType;
                node->semantic->adjustedType = constantAssociated->constantType;
                return {constantAssociated->constantType, constantAssociated};
              }
            } else {
              error.occur(pathInExpr->pos,
                          "Invalid Constant Value: `" +
                              structPath.pathIdentSegment +
                              "::" + constantPath.pathIdentSegment +
                              "` is not a constant");
              return {Type(TypeKind::Error), nullptr};
            }
          } else {
            error.occur(
                pathInExpr->pos,
                "Invalid Constant Value: undefined associated constant `" +
                    structPath.pathIdentSegment +
                    "::" + constantPath.pathIdentSegment + "`");
            return {Type(TypeKind::Error), nullptr};
          }
        } else {
          error.occur(pathInExpr->pos,
                      "Invalid Constant Value: undefined struct `" +
                          structPath.pathIdentSegment + "` in constant path");
          return {Type(TypeKind::Error), nullptr};
        }
      }
    }
  }
}

auto SemanticAnalyzer::analyzeConstExpr(ConstExprNode *node, Type *expected)
    -> std::pair<Type, ConstantSymbol *> {
  node->semantic = std::make_unique<ExprSemantic>();
  if (node->isNegative) {
    auto magPair = analyzeMagnitudeExpr(
        std::get<std::unique_ptr<MagnitudeExprNode>>(node->constValue).get());
    auto &magType = magPair.first;
    node->semantic->originalType = magType;
    node->semantic->adjustedType = magType;
    if (magType.kind == TypeKind::U32 || magType.kind == TypeKind::Usize) {
      error.occur(
          node->pos,
          "Invalid Constant Value: unary `-` requires a signed integer, "
          "not `u32` or `usize`");
      node->semantic->adjustedType = Type(TypeKind::Error);
      return {Type(TypeKind::Error), nullptr};
    } else {
      return magPair;
    }
  } else if (std::holds_alternative<std::unique_ptr<LiteralExprNode>>(
                 node->constValue)) {
    auto *literalPtr =
        std::get<std::unique_ptr<LiteralExprNode>>(node->constValue).get();
    Type type(TypeKind::Bool);
    if (literalPtr->literalValue != "true" &&
        literalPtr->literalValue != "false") {
      type = parseIntegerLiteral(node->pos, literalPtr->literalValue, false,
                                 expected);
    }
    literalPtr->semantic = std::make_unique<ExprSemantic>();
    literalPtr->semantic->originalType = type;
    literalPtr->semantic->adjustedType = type;
    node->semantic->originalType = type;
    node->semantic->adjustedType = type;
    return {type, nullptr};
  } else {
    auto *pathInExpr =
        std::get<std::unique_ptr<PathInExprNode>>(node->constValue).get();
    pathInExpr->semantic = std::make_unique<ValuePathInExprSemantic>();
    auto *pathSemantic =
        static_cast<ValuePathInExprSemantic *>(pathInExpr->semantic.get());
    if (pathInExpr->path.segments.size() != 1 &&
        pathInExpr->path.segments.size() != 2) {
      error.occur(pathInExpr->pos,
                  "Invalid Constant Value: a constant path must be "
                  "`NAME` or `Type::NAME`");
      return {Type(TypeKind::Error), nullptr};
    }
    for (std::size_t i = 0; i < pathInExpr->path.segments.size(); ++i) {
      const auto &segment = pathInExpr->path.segments[i];
      const bool isStructQualifier = pathInExpr->path.segments.size() == 2 &&
                                     i == 0 &&
                                     segment.pathIdentSegment != "Self" &&
                                     segment.pathIdentSegment != "self";
      if (!segment.args.genericArgs.empty() ||
          (segment.args.hasLifeTime && !isStructQualifier)) {
        error.occur(pathInExpr->pos,
                    "Invalid Constant Value: constant names do not accept "
                    "type or lifetime arguments; struct qualifiers only "
                    "accept lifetime arguments");
        return {Type(TypeKind::Error), nullptr};
      }
    }
    if (pathInExpr->path.segments.size() == 1) {
      const auto &seg = pathInExpr->path.segments.front();
      if (seg.pathIdentSegment == "Self" || seg.pathIdentSegment == "self") {
        error.occur(pathInExpr->pos,
                    "Invalid Constant Value: `Self` and `self` alone "
                    "do not name a constant");
        return {Type(TypeKind::Error), nullptr};
      } else {
        if (auto *constantItem =
                globalScopeVisitor->lookupConstant(seg.pathIdentSegment);
            constantItem) {
          pathSemantic->bindingSymbol = constantItem;
          pathSemantic->originalType = constantItem->constantType;
          pathSemantic->adjustedType = constantItem->constantType;
          node->semantic->originalType = constantItem->constantType;
          node->semantic->adjustedType = constantItem->constantType;
          return {constantItem->constantType, constantItem};
        } else {
          error.occur(pathInExpr->pos,
                      "Invalid Constant Value: undefined constant `" +
                          seg.pathIdentSegment + "`");
          return {Type(TypeKind::Error), nullptr};
        }
      }
    } else {
      auto *implScope = findImplementationScope(currentScope.get());
      const auto &structPath = pathInExpr->path.segments.front();
      const auto &constantPath = pathInExpr->path.segments.back();
      if (structPath.pathIdentSegment == "self") {
        error.occur(pathInExpr->pos,
                    "Invalid Constant Value: `self` cannot qualify a constant "
                    "path; use a struct type or `Self`");
        return {Type(TypeKind::Error), nullptr};
      } else if (structPath.pathIdentSegment == "Self") {
        if (!implScope) {
          error.occur(pathInExpr->pos,
                      "Invalid Constant Value: `Self::NAME` requires "
                      "an enclosing struct implementation");
          return {Type(TypeKind::Error), nullptr};
        }
        if (auto *constantMember =
                implScope->lookupConstant(constantPath.pathIdentSegment);
            constantMember) {
          pathSemantic->bindingSymbol = constantMember;
          pathSemantic->originalType = constantMember->constantType;
          pathSemantic->adjustedType = constantMember->constantType;
          node->semantic->originalType = constantMember->constantType;
          node->semantic->adjustedType = constantMember->constantType;
          return {constantMember->constantType, constantMember};
        } else {
          error.occur(
              pathInExpr->pos,
              "Invalid Constant Value: undefined associated constant `Self::" +
                  constantPath.pathIdentSegment + "`");
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
              pathSemantic->bindingSymbol = constantAssociated;
              pathSemantic->originalType = constantAssociated->constantType;
              pathSemantic->adjustedType = constantAssociated->constantType;
              node->semantic->originalType = constantAssociated->constantType;
              node->semantic->adjustedType = constantAssociated->constantType;
              return {constantAssociated->constantType, constantAssociated};
            } else {
              error.occur(pathInExpr->pos,
                          "Invalid Constant Value: `" +
                              structPath.pathIdentSegment +
                              "::" + constantPath.pathIdentSegment +
                              "` is not a constant");
              return {Type(TypeKind::Error), nullptr};
            }
          } else {
            error.occur(
                pathInExpr->pos,
                "Invalid Constant Value: undefined associated constant `" +
                    structPath.pathIdentSegment +
                    "::" + constantPath.pathIdentSegment + "`");
            return {Type(TypeKind::Error), nullptr};
          }
        } else {
          error.occur(pathInExpr->pos,
                      "Invalid Constant Value: undefined struct `" +
                          structPath.pathIdentSegment + "` in constant path");
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
    error.occur(
        symbol->node->pos,
        "Invalid Constant: evaluating `" + symbol->name +
            "` creates a cyclic dependency between constant initializers");
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

void SemanticAnalyzer::checkStruct() {
  // All struct symbols exist now, including forward declarations.
  for (auto &globalSymbol : globalScopeVisitor->typeNameSpace) {
    if (auto *structSymbol =
            dynamic_cast<StructSymbol *>(globalSymbol.second.get())) {
      structSymbol->dependency.clear();
      for (const auto &[name, fieldType] : structSymbol->structFields) {
        const Type *typeForValueDependency = &fieldType;
        while (typeForValueDependency->kind == TypeKind::Array) {
          typeForValueDependency = typeForValueDependency->elementType.get();
        }
        if (typeForValueDependency->kind != TypeKind::Struct) {
          continue;
        }
        const auto &dependencyName = typeForValueDependency->structName.value();
        if (auto *dependencySymbol =
                globalScopeVisitor->lookupStruct(dependencyName)) {
          structSymbol->dependency.emplace_back(dependencySymbol);
        } else {
          error.occur(structSymbol->pos,
                      "Invalid Struct: struct `" + structSymbol->name +
                          "` contains a field of undefined struct type `" +
                          dependencyName + "`");
        }
      }
    }
  }
  std::queue<StructSymbol *> copyFailQueue;
  std::queue<StructSymbol *> cloneFailQueue;
  std::queue<StructSymbol *> partialEqFailQueue;
  std::queue<StructSymbol *> eqFailQueue;
  for (auto &globalSymbol : globalScopeVisitor->typeNameSpace) {
    if (auto *structSymbol =
            dynamic_cast<StructSymbol *>(globalSymbol.second.get())) {
      if (structSymbol->outAttribute.test(0) &&
          !structSymbol->outAttribute.test(1)) {
        error.occur(structSymbol->pos,
                    "Invalid Struct: Copy trait must exist with Clone trait");
        structSymbol->traitValid.reset(0);
        structSymbol->traitValid.reset(1);
      } else if (structSymbol->outAttribute.test(3) &&
                 !structSymbol->outAttribute.test(2)) {
        error.occur(structSymbol->pos,
                    "Invalid Struct: Eq trait must exist with PartialEq trait");
        structSymbol->traitValid.reset(2);
        structSymbol->traitValid.reset(3);
      }
      if (structSymbol->dependencyChecked ==
          StructSymbol::dependencyCheckState::NotAccess) {
        analyzeStructDependency(*structSymbol);
      }
      for (const auto &[name, fieldType] : structSymbol->structFields) {
        const Type &typeForTraitDependency = fieldType;
        // checkItemType has already ensured that all the named struct exists in
        // struct entries
        if (structSymbol->outAttribute.test(0)) {
          auto copyDependency =
              structSymbol->checkCopyDepend(typeForTraitDependency);
          if (copyDependency) {
            auto *copyStructSymbol = globalScopeVisitor->lookupStruct(
                std::string(copyDependency.value()));
            copyStructSymbol->copyDependOnThis.emplace_back(structSymbol);
          }
        }
        if (structSymbol->outAttribute.test(1)) {
          auto cloneDependency =
              structSymbol->checkCloneDepend(typeForTraitDependency);
          if (cloneDependency) {
            auto *cloneStructSymbol = globalScopeVisitor->lookupStruct(
                std::string(cloneDependency.value()));
            cloneStructSymbol->cloneDependOnThis.emplace_back(structSymbol);
          }
        }
        if (structSymbol->outAttribute.test(2)) {
          auto partialEqDependency =
              structSymbol->checkPartialEqDepend(typeForTraitDependency);
          if (partialEqDependency) {
            auto *partialEqStructSymbol = globalScopeVisitor->lookupStruct(
                std::string(partialEqDependency.value()));
            partialEqStructSymbol->partialEqDependOnThis.emplace_back(
                structSymbol);
          }
        }
        if (structSymbol->outAttribute.test(3)) {
          auto eqDependency =
              structSymbol->checkEqDepend(typeForTraitDependency);
          if (eqDependency) {
            auto *eqStructSymbol = globalScopeVisitor->lookupStruct(
                std::string(eqDependency.value()));
            eqStructSymbol->eqDependOnThis.emplace_back(structSymbol);
          }
        }
      }
      if (!structSymbol->traitValid.test(0)) {
        copyFailQueue.push(structSymbol);
      }
      if (!structSymbol->traitValid.test(1)) {
        cloneFailQueue.push(structSymbol);
      }
      if (!structSymbol->traitValid.test(2)) {
        partialEqFailQueue.push(structSymbol);
      }
      if (!structSymbol->traitValid.test(3)) {
        eqFailQueue.push(structSymbol);
      }
    }
  }
  while (!copyFailQueue.empty()) {
    auto *failSymbol = copyFailQueue.front();
    copyFailQueue.pop();
    for (auto *dependedSymbol : failSymbol->copyDependOnThis) {
      if (dependedSymbol->traitValid.test(0)) {
        dependedSymbol->traitValid.reset(0);
        copyFailQueue.push(dependedSymbol);
      }
    }
  }
  while (!cloneFailQueue.empty()) {
    auto *failSymbol = cloneFailQueue.front();
    cloneFailQueue.pop();
    for (auto *dependedSymbol : failSymbol->cloneDependOnThis) {
      if (dependedSymbol->traitValid.test(1)) {
        dependedSymbol->traitValid.reset(1);
        cloneFailQueue.push(dependedSymbol);
      }
    }
  }
  while (!partialEqFailQueue.empty()) {
    auto *failSymbol = partialEqFailQueue.front();
    partialEqFailQueue.pop();
    for (auto *dependedSymbol : failSymbol->partialEqDependOnThis) {
      if (dependedSymbol->traitValid.test(2)) {
        dependedSymbol->traitValid.reset(2);
        partialEqFailQueue.push(dependedSymbol);
      }
    }
  }
  while (!eqFailQueue.empty()) {
    auto *failSymbol = eqFailQueue.front();
    eqFailQueue.pop();
    for (auto *dependedSymbol : failSymbol->eqDependOnThis) {
      if (dependedSymbol->traitValid.test(3)) {
        dependedSymbol->traitValid.reset(3);
        eqFailQueue.push(dependedSymbol);
      }
    }
  }
  auto failTrait = [](int i) {
    switch (i) {
    case 0:
      return "Copy";
    case 1:
      return "Clone";
    case 2:
      return "PartialEq";
    case 3:
      return "Eq";
    default:
      std::unreachable();
      break;
    }
  };
  for (auto &globalSymbol : globalScopeVisitor->typeNameSpace) {
    if (auto *structSymbol =
            dynamic_cast<StructSymbol *>(globalSymbol.second.get())) {
      for (int i = 0; i < 4; ++i) {
        if (structSymbol->outAttribute.test(i) &&
            !structSymbol->traitValid.test(i)) {
          error.occur(
              structSymbol->pos,
              std::string(
                  "Invalid Struct: the struct is declared with the trait ") +
                  failTrait(i) + " but does not meet the requirements");
        }
      }
      if (structSymbol->traitValid.test(1)) {
        getOrCreateBuiltinClone(Type(structSymbol->name));
      }
    }
  }
}

void SemanticAnalyzer::analyzeStructDependency(StructSymbol &symbol) {
  symbol.dependencyChecked = StructSymbol::dependencyCheckState::checking;
  for (auto *dependencySymbol : symbol.dependency) {
    switch (dependencySymbol->dependencyChecked) {
    case StructSymbol::dependencyCheckState::NotAccess: {
      analyzeStructDependency(*dependencySymbol);
      break;
    }
    case StructSymbol::dependencyCheckState::checking: {
      error.occur(symbol.pos,
                  "Invalid Struct: by-value dependency from `" + symbol.name +
                      "` to `" + dependencySymbol->name +
                      "` creates a recursive type with no finite size");
      break;
    }
    case StructSymbol::dependencyCheckState::checked: {
      continue;
    }
    }
  }
  symbol.dependencyChecked = StructSymbol::dependencyCheckState::checked;
}

void SemanticAnalyzer::checkConstant() {
  auto checkDeclaredType = [this](ConstantSymbol *constant) {
    auto &type = constant->constantType;
    if (type.kind == TypeKind::Error) {
      return false;
    }
    if (isIntegerType(type) || type.kind == TypeKind::Bool) {
      return true;
    }
    error.occur(
        constant->node->constType->pos,
        "Invalid Constant: `" + constant->name +
            "` must have type `i32`, `u32`, `isize`, `usize`, or `bool`");
    type = Type(TypeKind::Error, type.pos);
    return false;
  };
  for (const auto &globalSymbol : globalScopeVisitor->valueNameSpace) {
    if (auto *constantSymbol =
            dynamic_cast<ConstantSymbol *>(globalSymbol.second.get());
        constantSymbol && checkDeclaredType(constantSymbol)) {
      analyzeConstantDecl(constantSymbol);
    }
  }
  for (const auto &globalSymbol : globalScopeVisitor->typeNameSpace) {
    if (auto *structSymbol =
            dynamic_cast<StructSymbol *>(globalSymbol.second.get())) {
      for (const auto &globalSymbol : structSymbol->structSymbols) {
        if (auto *constantSymbol =
                dynamic_cast<ConstantSymbol *>(globalSymbol.second.get());
            constantSymbol && checkDeclaredType(constantSymbol)) {
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
    error.occur(node->pos,
                "Invalid Array Length: the length must have type `usize`");
    node->semantic->adjustedType = Type(TypeKind::Error);
    return std::pair<Type, long long>(Type(TypeKind::Error), 0);
  } else {
    if (std::holds_alternative<std::unique_ptr<LiteralExprNode>>(
            node->constValue)) {
      auto *literal =
          std::get<std::unique_ptr<LiteralExprNode>>(node->constValue).get();
      auto length = parseIntegerLiteral(literal->literalValue);
      node->semantic = std::make_unique<ConstExprSemantic>(length);
      return std::pair<Type, long long>(Type(TypeKind::Usize), length);
    } else {
      auto length = std::get<long long>(dependency->value.value);
      node->semantic = std::make_unique<ConstExprSemantic>(length);
      return std::pair<Type, long long>(Type(TypeKind::Usize), length);
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

auto SemanticAnalyzer::analyzeLiteralExpr(LiteralExprNode *node, Type *expected,
                                          bool isNegative) -> ExprInfo {
  ExprInfo exprInfo;
  if (node->literalValue == "true" || node->literalValue == "false") {
    exprInfo = ExprInfo{Type(TypeKind::Bool), false};
    node->semantic =
        std::make_unique<LiteralExprSemantic>(node->literalValue == "true");
  } else {
    exprInfo = ExprInfo{parseIntegerLiteral(node->pos, node->literalValue,
                                            isNegative, expected),
                        false};
    if (exprInfo.type.kind != TypeKind::Error) {
      node->semantic = std::make_unique<LiteralExprSemantic>(
          parseIntegerLiteral(node->literalValue), exprInfo.type);
    }
  }
  return exprInfo;
}

std::optional<std::string> SemanticAnalyzer::getSelfName() {
  if (auto *scope = findImplementationScope(currentScope.get())) {
    return scope->structName;
  }
  return std::nullopt;
}

std::any SemanticAnalyzer::visitPathInExpr(PathInExprNode *node) {
  // Invalid paths keep an error semantic; successful lookup selects its kind.
  node->semantic = std::make_unique<ExprSemantic>();
  const auto bindFunction = [&](FunctionSymbol *function) {
    auto semantic = std::make_unique<FunctionPathInExprSemantic>();
    semantic->functionSymbol = function;
    node->semantic = std::move(semantic);
  };
  const auto bindValue = [&](auto *symbol) {
    auto semantic = std::make_unique<ValuePathInExprSemantic>();
    semantic->bindingSymbol = symbol;
    node->semantic = std::move(semantic);
  };
  const auto &segments = node->path.segments;
  if (segments.empty()) {
    error.occur(node->pos,
                "Invalid Path: a value path must contain at least one segment");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  if (segments.size() > 2) {
    error.occur(node->pos,
                "Invalid Path: a value path must be `NAME` or `Type::NAME`");
    return ExprInfo{Type(TypeKind::Error), false};
  }

  if (segments.size() == 1) {
    const auto &seg = segments.front();
    if (!seg.args.genericArgs.empty()) {
      error.occur(node->pos, "Invalid Path: value path `" +
                                 seg.pathIdentSegment +
                                 "` does not accept type arguments");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    if (auto *binding = currentScope->lookup(seg.pathIdentSegment)) {
      if (seg.args.hasLifeTime) {
        error.occur(node->pos, "Invalid Path: variable `" +
                                   seg.pathIdentSegment +
                                   "` does not accept lifetime arguments");
        return ExprInfo{Type(TypeKind::Error), false};
      }
      bindValue(binding);
      return ExprInfo{binding->varType, false, true, binding->isMut};
    }
    if (auto *constant =
            globalScopeVisitor->lookupConstant(seg.pathIdentSegment)) {
      if (seg.args.hasLifeTime) {
        error.occur(node->pos, "Invalid Path: constant `" +
                                   seg.pathIdentSegment +
                                   "` does not accept lifetime arguments");
        return ExprInfo{Type(TypeKind::Error), false};
      }
      bindValue(constant);
      return ExprInfo{constant->constantType, false, false};
    }
    auto *function = globalScopeVisitor->lookupFunction(seg.pathIdentSegment);
    // Only source-visible built-ins participate in unqualified lookup;
    // synthesized container and clone functions remain internal.
    if (!function && !BuiltInNames::checkValueName(seg.pathIdentSegment)) {
      if (auto iterator = builtinFunctions.find(seg.pathIdentSegment);
          iterator != builtinFunctions.end()) {
        function = iterator->second.get();
      }
    }
    if (function) {
      // A function path may carry lifetimes; they do not affect its value.
      Type type(TypeKind::Function);
      type.functionSymbol = function;
      bindFunction(function);
      return ExprInfo{type, false, false};
    }
    error.occur(node->pos,
                "Invalid Path: undefined value `" + seg.pathIdentSegment + "`");
    return ExprInfo{Type(TypeKind::Error), false};
  }

  const auto &typeSeg = segments.front();
  const auto &nameSeg = segments.back();
  if (!nameSeg.args.genericArgs.empty()) {
    error.occur(node->pos, "Invalid Path: associated item `" +
                               nameSeg.pathIdentSegment +
                               "` does not accept type arguments");
    return ExprInfo{Type(TypeKind::Error), false};
  }

  if (typeSeg.pathIdentSegment == "Box" || typeSeg.pathIdentSegment == "Vec") {
    if (typeSeg.args.genericArgs.size() != 1u || typeSeg.args.hasLifeTime) {
      error.occur(node->pos, "Invalid Path: Box or Vec requires exactly one "
                             "type argument and no lifetime arguments");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    auto argumentType = std::any_cast<Type>(
        typeSeg.args.genericArgs.front().genericArgType->accept(this));
    if (argumentType.kind == TypeKind::Error) {
      return ExprInfo{Type(TypeKind::Error), false};
    }
    Type linkedType(typeSeg.pathIdentSegment == "Box" ? TypeKind::Box
                                                      : TypeKind::Vec);
    linkedType.elementType = std::make_unique<Type>(argumentType);
    const auto &methodName = nameSeg.pathIdentSegment;
    auto *function =
        getOrCreateBuiltinContainerFunction(linkedType, methodName);
    if (!function) {
      error.occur(node->pos,
                  "Invalid Path: container `" + typeSeg.pathIdentSegment +
                      "` has no associated item named `" + methodName + "`");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    bindFunction(function);
    return ExprInfo{function->functionalType, false};
  }

  if (!typeSeg.args.genericArgs.empty()) {
    error.occur(node->pos, "Invalid Path: struct qualifier `" +
                               typeSeg.pathIdentSegment +
                               "` does not accept type arguments");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  if (typeSeg.pathIdentSegment == "self") {
    error.occur(node->pos,
                "Invalid Path: `self` is a value, not a type qualifier; "
                "use a struct type or `Self`");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  const auto primitiveClone = [&](TypeKind kind) -> ExprInfo {
    if (typeSeg.args.hasLifeTime) {
      error.occur(node->pos, "Invalid Path: primitive type `" +
                                 typeSeg.pathIdentSegment +
                                 "` does not accept lifetime arguments");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    if (nameSeg.pathIdentSegment != "clone") {
      error.occur(node->pos, "Invalid Path: primitive type `" +
                                 typeSeg.pathIdentSegment +
                                 "` has no associated item named `" +
                                 nameSeg.pathIdentSegment + "`");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    auto *function = getOrCreateBuiltinClone(Type(kind, node->pos));
    bindFunction(function);
    return ExprInfo{function->functionalType, false};
  };
  if (typeSeg.pathIdentSegment == "i32") {
    return primitiveClone(TypeKind::I32);
  }
  if (typeSeg.pathIdentSegment == "u32") {
    return primitiveClone(TypeKind::U32);
  }
  if (typeSeg.pathIdentSegment == "isize") {
    return primitiveClone(TypeKind::Isize);
  }
  if (typeSeg.pathIdentSegment == "usize") {
    return primitiveClone(TypeKind::Usize);
  }
  if (typeSeg.pathIdentSegment == "bool") {
    return primitiveClone(TypeKind::Bool);
  }
  std::optional<std::string> structName;
  if (typeSeg.pathIdentSegment == "Self") {
    if (typeSeg.args.hasLifeTime) {
      error.occur(node->pos,
                  "Invalid Path: `Self` does not accept lifetime arguments");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    structName = getSelfName();
    if (!structName) {
      error.occur(
          node->pos,
          "Invalid Path: `Self` requires an enclosing struct implementation");
      return ExprInfo{Type(TypeKind::Error), false};
    }
  } else {
    // Lifetime arguments on a user struct are legal and have no value effect.
    structName = typeSeg.pathIdentSegment;
  }

  auto *structSymbol = globalScopeVisitor->lookupStruct(structName.value());
  if (structSymbol == nullptr) {
    error.occur(node->pos,
                "Invalid Path: undefined struct `" + structName.value() + "`");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  auto iterator = structSymbol->structSymbols.find(nameSeg.pathIdentSegment);
  if (iterator == structSymbol->structSymbols.end()) {
    if (nameSeg.pathIdentSegment == "clone") {
      auto *function = getOrCreateBuiltinClone(Type(structName.value()));
      if (function) {
        bindFunction(function);
        return ExprInfo{function->functionalType, false};
      }
      error.occur(node->pos, "Invalid Path: the struct has no Clone trait");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    error.occur(node->pos, "Invalid Path: struct `" + structName.value() +
                               "` has no associated item named `" +
                               nameSeg.pathIdentSegment + "`");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  auto *itemSymbol = iterator->second.get();
  if (auto *function = dynamic_cast<FunctionSymbol *>(itemSymbol)) {
    // A function path may carry lifetimes; they do not affect its value.
    Type type(TypeKind::Function);
    type.functionSymbol = function;
    bindFunction(function);
    return ExprInfo{type, false, false};
  }
  if (auto *constant = dynamic_cast<ConstantSymbol *>(itemSymbol)) {
    if (nameSeg.args.hasLifeTime) {
      error.occur(node->pos, "Invalid Path: associated constant `" +
                                 nameSeg.pathIdentSegment +
                                 "` does not accept lifetime arguments");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    bindValue(constant);
    return ExprInfo{constant->constantType, false, false};
  }
  error.occur(node->pos,
              "Invalid Path: associated item `" + nameSeg.pathIdentSegment +
                  "` must be a constant or function to be used as a value");
  return ExprInfo{Type(TypeKind::Error), false};
}
std::any SemanticAnalyzer::visitAccessExpr(AccessExprNode *node) {
  node->semantic = std::make_unique<AccessExprSemantic>();
  auto *accessSemantic =
      static_cast<AccessExprSemantic *>(node->semantic.get());
  auto structInfo = analyzeExpr(node->accessObject.get());
  if (structInfo.type.kind == TypeKind::Error) {
    return ExprInfo{Type(TypeKind::Error), false};
  }
  if (!structInfo.isPlace) {
    // temporary place
    structInfo.isPlace = true;
    structInfo.isMutablePlace = !structInfo.mutationBlocked;
  }
  bool isMutablePlace = structInfo.isMutablePlace;
  bool mutationBlocked = structInfo.mutationBlocked;
  std::vector<Adjustment> adjusts;
  auto innerType = autoDereference(&structInfo.type, isMutablePlace,
                                   mutationBlocked, adjusts);
  if (innerType.kind != TypeKind::Struct) {
    error.occur(node->accessObject->pos,
                "Invalid Access: field access requires a struct value");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  if (auto *structSymbol =
          globalScopeVisitor->lookupStruct(innerType.structName.value());
      !structSymbol) {
    error.occur(node->accessObject->pos,
                "Invalid Access: the target struct type is not defined");
    return ExprInfo{Type(TypeKind::Error), false};
  } else {
    bool found = false;
    for (auto it = structSymbol->layoutFields.begin();
         it != structSymbol->layoutFields.end(); ++it) {
      if (*it == node->identifier) {
        accessSemantic->object = structSymbol;
        accessSemantic->index = it - structSymbol->layoutFields.begin();
        node->accessObject->semantic->adjustments = std::move(adjusts);
        node->accessObject->semantic->adjustedType = innerType;
        found = true;
        break;
      }
    }
    if (!found) {
      error.occur(node->pos,
                  "Invalid Access: struct `" + innerType.structName.value() +
                      "` has no field named `" + node->identifier + "`");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    const auto &field = structSymbol->structFields.at(node->identifier);
    return ExprInfo{field, structInfo.isDiverge, true, isMutablePlace,
                    mutationBlocked};
  }
}

std::any SemanticAnalyzer::visitCallExpr(CallExprNode *node) {
  node->semantic = std::make_unique<CallExprSemantic>();
  auto *calleeSemantic = static_cast<CallExprSemantic *>(node->semantic.get());
  auto functionInfo = analyzeExpr(node->callee.get());
  if (functionInfo.type.kind == TypeKind::Error) {
    return ExprInfo{Type(TypeKind::Error), false};
  }
  if (functionInfo.type.kind != TypeKind::Function) {
    error.occur(node->callee->pos,
                "Invalid Expression: the expression being called "
                "must have a function type");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  calleeSemantic->calleeSymbol = functionInfo.type.functionSymbol.value();
  auto &functionSymbol = calleeSemantic->calleeSymbol;
  // check the params type
  if (functionSymbol->paramTypes.size() != node->callParams.size()) {
    error.occur(node->callee->pos, "Invalid Expression: the size of the "
                                   "parameters must match the function");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  bool isDiverge = functionInfo.isDiverge;
  for (std::size_t i = 0; i < node->callParams.size(); ++i) {
    auto paramInfo =
        checkExpr(node->callParams[i].get(), &functionSymbol->paramTypes[i]);

    if (paramInfo.type.kind == TypeKind::Error) {
      return ExprInfo{Type(TypeKind::Error), false};
    }
    isDiverge |= paramInfo.isDiverge;
  }
  return ExprInfo{functionSymbol->returnType, isDiverge};
}

std::any SemanticAnalyzer::visitLiteralExpr(LiteralExprNode *node) {
  return analyzeLiteralExpr(node, nullptr, false);
}

std::any SemanticAnalyzer::visitUnaryExpr(UnaryExprNode *node) {
  if (node->op == UnaryOperator::NEG) {
    if (auto *literalNode =
            dynamic_cast<LiteralExprNode *>(node->childExpr.get())) {
      auto literalType = analyzeLiteralExpr(literalNode, nullptr, true).type;
      if (literalType.kind == TypeKind::Error) {
        return ExprInfo{Type(TypeKind::Error), false};
      }
      if (literalType.kind == TypeKind::I32 ||
          literalType.kind == TypeKind::Isize) {
        return ExprInfo{literalType, false, false};
      } else {
        error.occur(node->childExpr->pos,
                    "Invalid Unary Expression: unary `-` requires "
                    "a signed integer (`i32` or `isize`)");
        return ExprInfo{Type(TypeKind::Error), false};
      }
    } else {
      auto exprInfo = analyzeExpr(node->childExpr.get());
      if (exprInfo.type.kind == TypeKind::Error) {
        return ExprInfo{Type(TypeKind::Error), false};
      }
      if (exprInfo.type.kind == TypeKind::ImmutReference) {
        auto *innerType = exprInfo.type.elementType.get();
        if (innerType->kind == TypeKind::Error) {
          return ExprInfo{Type(TypeKind::Error), false};
        }
        if (innerType->kind == TypeKind::I32 ||
            innerType->kind == TypeKind::Isize) {
          node->childExpr->semantic->adjustments.emplace_back(
              Adjustment{AdjustmentKind::Deref, *innerType});
          node->childExpr->semantic->adjustedType = *innerType;
          return ExprInfo{*innerType, exprInfo.isDiverge, false};
        } else {
          error.occur(node->childExpr->pos,
                      "Invalid Unary Expression: unary `-` requires "
                      "a signed integer or a shared reference to one");
          return ExprInfo{Type(TypeKind::Error), false};
        }
      } else if (exprInfo.type.kind == TypeKind::I32 ||
                 exprInfo.type.kind == TypeKind::Isize) {
        return ExprInfo{exprInfo.type, exprInfo.isDiverge, false};
      } else {
        error.occur(node->childExpr->pos,
                    "Invalid Unary Expression: unary `-` requires "
                    "a signed integer or a shared reference to one");
        return ExprInfo{Type(TypeKind::Error), false};
      }
    }
  } else {
    auto exprInfo = analyzeExpr(node->childExpr.get());
    if (exprInfo.type.kind == TypeKind::Error) {
      return ExprInfo{Type(TypeKind::Error), false};
    }
    if (exprInfo.type.kind == TypeKind::ImmutReference) {
      auto *innerType = exprInfo.type.elementType.get();
      if (innerType->kind == TypeKind::Error) {
        return ExprInfo{Type(TypeKind::Error), false};
      }
      if (innerType->kind >= TypeKind::I32 &&
          innerType->kind <= TypeKind::Bool) {
        node->childExpr->semantic->adjustments.emplace_back(
            Adjustment{AdjustmentKind::Deref, *innerType});
        node->childExpr->semantic->adjustedType = *innerType;
        return ExprInfo{*innerType, exprInfo.isDiverge, false};
      } else {
        error.occur(node->childExpr->pos,
                    "Invalid Unary Expression: unary `!` requires "
                    "an integer, bool, or a shared reference to one");
        return ExprInfo{Type(TypeKind::Error), false};
      }
    } else if (exprInfo.type.kind >= TypeKind::I32 &&
               exprInfo.type.kind <= TypeKind::Bool) {
      return ExprInfo{exprInfo.type, exprInfo.isDiverge, false};
    } else {
      error.occur(node->childExpr->pos,
                  "Invalid Unary Expression: unary `!` requires "
                  "an integer, bool, or a shared reference to one");
      return ExprInfo{Type(TypeKind::Error), false};
    }
  }
}

std::any SemanticAnalyzer::visitCastExpr(CastExprNode *node) {
  auto childExprInfo = analyzeExpr(node->castValue.get());
  if (childExprInfo.type.kind == TypeKind::Error) {
    return ExprInfo{Type(TypeKind::Error), false};
  }
  auto castType = std::any_cast<Type>(node->castType->accept(this));
  if ((isIntegerType(childExprInfo.type) && isIntegerType(castType)) ||
      (childExprInfo.type.kind == TypeKind::Bool && isIntegerType(castType))) {
    return ExprInfo{castType, childExprInfo.isDiverge, false};
  }
  error.occur(node->pos, "Invalid Cast: `as` supports only integer-to-integer "
                         "and bool-to-integer conversions");
  return ExprInfo{Type(TypeKind::Error), false};
}

std::any SemanticAnalyzer::visitBinaryExpr(BinaryExprNode *node) {
  auto leftExprInfo = analyzeExpr(node->leftChildExpr.get());
  auto rightExprInfo = analyzeExpr(node->rightChildExpr.get());
  auto recordAdjustment = [&] {
    if (leftExprInfo.type.kind == TypeKind::ImmutReference) {
      const auto &target = *leftExprInfo.type.elementType;
      node->leftChildExpr->semantic->adjustments.emplace_back(
          Adjustment{AdjustmentKind::Deref, target});
      node->leftChildExpr->semantic->adjustedType = target;
    }
    if (rightExprInfo.type.kind == TypeKind::ImmutReference) {
      const auto &target = *rightExprInfo.type.elementType;
      node->rightChildExpr->semantic->adjustments.emplace_back(
          Adjustment{AdjustmentKind::Deref, target});
      node->rightChildExpr->semantic->adjustedType = target;
    }
  };
  if (leftExprInfo.type.kind == TypeKind::Error ||
      rightExprInfo.type.kind == TypeKind::Error) {
    return ExprInfo{Type(TypeKind::Error), false};
  }
  auto leftTypeKind = leftExprInfo.type.kind == TypeKind::ImmutReference
                          ? leftExprInfo.type.elementType->kind
                          : leftExprInfo.type.kind;
  auto rightTypeKind = rightExprInfo.type.kind == TypeKind::ImmutReference
                           ? rightExprInfo.type.elementType->kind
                           : rightExprInfo.type.kind;
  const bool isDiverge = leftExprInfo.isDiverge || rightExprInfo.isDiverge;
  if (node->op >= BinaryOperator::Add && node->op <= BinaryOperator::Mod) {
    if (leftTypeKind == rightTypeKind &&
        (leftTypeKind >= TypeKind::I32 && leftTypeKind <= TypeKind::Usize)) {
      recordAdjustment();
      return ExprInfo{Type(leftTypeKind), isDiverge, false};
    } else {
      error.occur(node->pos, "Invalid Operation: arithmetic operands must have "
                             "the same integer type "
                             "or be shared references to that type");
      return ExprInfo{Type(TypeKind::Error), false};
    }
  } else if (node->op >= BinaryOperator::BitAnd &&
             node->op <= BinaryOperator::BitXor) {
    if (leftTypeKind == rightTypeKind &&
        (leftTypeKind >= TypeKind::I32 && leftTypeKind <= TypeKind::Bool)) {
      recordAdjustment();
      return ExprInfo{Type(leftTypeKind), isDiverge, false};
    } else {
      error.occur(node->pos,
                  "Invalid Operation: bitwise operands must have the same "
                  "integer or bool type or be shared references to that type");
      return ExprInfo{Type(TypeKind::Error), false};
    }
  } else {
    if ((leftTypeKind >= TypeKind::I32 && leftTypeKind <= TypeKind::Usize) &&
        (rightTypeKind >= TypeKind::I32 && rightTypeKind <= TypeKind::Usize)) {
      recordAdjustment();
      return ExprInfo{Type(leftTypeKind), isDiverge, false};
    } else {
      error.occur(node->pos, "Invalid Operation: shift operands must be "
                             "integers or shared references "
                             "to integers; the integer types may differ");
      return ExprInfo{Type(TypeKind::Error), false};
    }
  }
}

std::any SemanticAnalyzer::visitLogicExpr(LogicExprNode *node) {
  auto leftExprInfo = analyzeExpr(node->leftChildExpr.get());
  auto rightExprInfo = analyzeExpr(node->rightChildExpr.get());
  if (leftExprInfo.type.kind == TypeKind::Error ||
      rightExprInfo.type.kind == TypeKind::Error) {
    return ExprInfo{Type(TypeKind::Error), false};
  }
  const auto leftTypeKind = leftExprInfo.type.kind;
  const auto rightTypeKind = rightExprInfo.type.kind;
  if (leftTypeKind == rightTypeKind && leftTypeKind == TypeKind::Bool) {
    return ExprInfo{Type(TypeKind::Bool), leftExprInfo.isDiverge, false};
  }
  error.occur(node->pos,
              "Invalid Operation: `&&` and `||` require bool values on both "
              "sides, not references or integers");
  return ExprInfo{Type(TypeKind::Error), false};
}

std::any SemanticAnalyzer::visitLetStmt(LetStmtNode *node) {
  node->semantic = std::make_unique<StmtSemantic>();
  node->semantic->isLet = true;
  auto defineLocalBinding = [&](const LocalSymbol &symbol) {
    auto localBindingPtr = std::make_unique<LocalSymbol>(symbol);
    localBindings.emplace_back(std::move(localBindingPtr));
    node->semantic->statementType = symbol.varType;
    node->semantic->bindingSymbol = localBindings.back().get();
    currentScope->define(node->identifier, *localBindings.back());
  };
  if (node->type) {
    auto expectedType = std::any_cast<Type>(node->type.value()->accept(this));
    if (expectedType.kind == TypeKind::Error) {
      error.occur(node->pos,
                  "Invalid Statement: the type annotation for binding `" +
                      node->identifier + "` is invalid");
    }
    auto actualInfo = checkExpr(node->value.get(), &expectedType);

    LocalSymbol letSymbol(node->pos, node->identifier, node->isMut,
                          actualInfo.type);
    defineLocalBinding(letSymbol);
    node->semantic->isDiverge = actualInfo.isDiverge;
    return StmtInfo{actualInfo.isDiverge};
  } else {
    ExprInfo initializer = checkExpr(node->value.get(), nullptr);
    LocalSymbol letSymbol(node->pos, node->identifier, node->isMut,
                          initializer.type);
    defineLocalBinding(letSymbol);
    node->semantic->isDiverge = initializer.isDiverge;
    return StmtInfo{initializer.isDiverge};
  }
}

auto SemanticAnalyzer::analyzeBlockExpr(BlockExprNode *node, Type *expected)
    -> ExprInfo {
  auto _ = ScopeGuard(currentScope);
  bool isDiverge = false;
  for (auto &childStmt : node->statements) {
    isDiverge |= std::any_cast<StmtInfo>(childStmt->accept(this)).isDiverge;
  }
  if (node->tailExpr) {
    auto *tail = node->tailExpr.value().get();
    auto exprInfo = checkExpr(tail, expected);
    exprInfo.isDiverge |= isDiverge;
    exprInfo.isPlace = false;
    exprInfo.isMutablePlace = false;
    // The block produces a value, not a projection of the tail's place.
    exprInfo.mutationBlocked = false;
    return exprInfo;
  } else {
    if (!isDiverge) {
      return ExprInfo{Type{TypeKind::Unit}, isDiverge};
    } else {
      return ExprInfo{Type{TypeKind::Never}, isDiverge};
    }
  }
}

std::any SemanticAnalyzer::visitBlockExpr(BlockExprNode *node) {
  return analyzeBlockExpr(node, nullptr);
}

std::any SemanticAnalyzer::visitExpressionStmt(ExpressionStmtNode *node) {
  auto exprInfo = analyzeExpr(node->expr.get());
  if (exprInfo.type.kind != TypeKind::Error && !node->hasSemicolon &&
      dynamic_cast<ExprWithBlockNode *>(node->expr.get())) {
    if (exprInfo.type.kind != TypeKind::Unit &&
        exprInfo.type.kind != TypeKind::Never) {
      error.occur(
          node->pos,
          "Invalid Statement: a block-like expression used as a statement "
          "without a semicolon must have type `()` or diverge; "
          "add `;` to discard its value");
    }
  }
  return StmtInfo{exprInfo.isDiverge};
}

std::any SemanticAnalyzer::visitEmptyStmt(EmptyStmtNode *) {
  return StmtInfo{false};
}

std::any SemanticAnalyzer::visitUnitExpr(UnitExprNode *) {
  return ExprInfo{Type(TypeKind::Unit), false};
}

std::any SemanticAnalyzer::visitComparisonExpr(ComparisonExprNode *node) {
  auto leftExprInfo = analyzeExpr(node->leftChildExpr.get());
  auto rightExprInfo = analyzeExpr(node->rightChildExpr.get());
  if (leftExprInfo.type.kind == TypeKind::Error ||
      rightExprInfo.type.kind == TypeKind::Error) {
    return ExprInfo{Type(TypeKind::Error), false};
  }
  const bool isDiverge = leftExprInfo.isDiverge || rightExprInfo.isDiverge;

  if (node->op == ComparisonOperator::equal ||
      node->op == ComparisonOperator::notEqual) {
    if (leftExprInfo.type != rightExprInfo.type) {
      error.occur(node->pos,
                  "Invalid Comparison: equality operands must have "
                  "the same source type; no implicit coercion applies");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    if (!hasTrait(leftExprInfo.type, BuiltinTrait::PartialEq)) {
      error.occur(node->pos,
                  "Invalid Comparison: equality operands must implement "
                  "PartialEq");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    return ExprInfo{Type(TypeKind::Bool), isDiverge};
  }

  auto canOrderCompare = [this](const Type &left, const Type &right) {
    if (left != right) {
      // Only the outermost right-hand &mut T may adjust to &T.
      if (left.kind != TypeKind::ImmutReference ||
          right.kind != TypeKind::MutReference ||
          *left.elementType != *right.elementType) {
        return false;
      }
    }
    const Type *type = &left;
    while (type->kind == TypeKind::ImmutReference ||
           type->kind == TypeKind::MutReference) {
      type = type->elementType.get();
    }

    return isIntegerType(*type) || type->kind == TypeKind::Bool;
  };
  if (!canOrderCompare(leftExprInfo.type, rightExprInfo.type)) {
    error.occur(node->pos,
                "Invalid Comparison: ordering requires matching integer or "
                "bool types, or matching references to them; only a left &T "
                "and right &mut T allow an outer reference adjustment");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  if (leftExprInfo.type != rightExprInfo.type) {
    node->rightChildExpr->semantic->adjustments.emplace_back(
        Adjustment{AdjustmentKind::MutToShared, leftExprInfo.type});
    node->rightChildExpr->semantic->adjustedType = leftExprInfo.type;
  }
  return ExprInfo{Type(TypeKind::Bool), isDiverge};
}

std::any SemanticAnalyzer::visitBorrowExpr(BorrowExprNode *node) {
  auto borrowInfo = analyzeExpr(node->borrowExpr.get());
  if (borrowInfo.type.kind == TypeKind::Error) {
    return ExprInfo{Type(TypeKind::Error), false};
  }
  if (!borrowInfo.isPlace) {
    borrowInfo.isMutablePlace = true;
    borrowInfo.isPlace = true;
  }
  if (node->isMut &&
      (!borrowInfo.isMutablePlace || borrowInfo.mutationBlocked)) {
    error.occur(node->borrowExpr->pos, "Invalid Expression: mutable borrow "
                                       "expression require a mutable place");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  Type borrowType(node->isMut ? TypeKind::MutReference
                              : TypeKind::ImmutReference);
  borrowType.elementType = std::make_unique<Type>(borrowInfo.type);
  return ExprInfo{borrowType, borrowInfo.isDiverge};
}

std::any SemanticAnalyzer::visitDereferenceExpr(DereferenceExprNode *node) {
  auto dereferenceInfo = analyzeExpr(node->dereferenceExpr.get());
  if (dereferenceInfo.type.kind == TypeKind::Error) {
    return ExprInfo{Type(TypeKind::Error), false};
  }
  if (dereferenceInfo.type.kind != TypeKind::ImmutReference &&
      dereferenceInfo.type.kind != TypeKind::MutReference &&
      dereferenceInfo.type.kind != TypeKind::Box) {
    error.occur(node->dereferenceExpr->pos,
                "Invalid Expression: dereference expression must be a "
                "reference or a box");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  if (!dereferenceInfo.isPlace) {
    dereferenceInfo.isMutablePlace = true;
    dereferenceInfo.isPlace = true;
  }
  Type dereferenceType = *dereferenceInfo.type.elementType;
  bool isMutablePlace = false;
  bool isBlocked = false;
  switch (dereferenceInfo.type.kind) {
  case TypeKind::MutReference: {
    isBlocked = dereferenceInfo.mutationBlocked;
    isMutablePlace = !isBlocked;
    break;
  }
  case TypeKind::ImmutReference: {
    isBlocked = true;
    break;
  }
  case TypeKind::Box: {
    isBlocked = dereferenceInfo.mutationBlocked;
    isMutablePlace = dereferenceInfo.isMutablePlace;
    break;
  }
  default:
    std::unreachable();
  }
  return ExprInfo{dereferenceType, dereferenceInfo.isDiverge, true,
                  isMutablePlace, isBlocked};
}

std::any SemanticAnalyzer::visitAssignmentExpr(AssignmentExprNode *node) {
  auto lhsInfo = analyzeExpr(node->leftChildExpr.get());
  if (lhsInfo.type.kind == TypeKind::Error) {
    return ExprInfo{Type(TypeKind::Error), false};
  }
  if (!lhsInfo.isPlace) {
    error.occur(
        node->leftChildExpr->pos,
        "Invalid Expression: an assignment destination must be a place, "
        "such as a variable, field, indexed element, or dereferenced target");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  if (!lhsInfo.isMutablePlace || lhsInfo.mutationBlocked) {
    error.occur(node->leftChildExpr->pos,
                "Invalid Expression: cannot assign to an immutable place; "
                "the assignment destination must allow mutation");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  auto recordAdjustments = [&](const ExprInfo &rhsInfo, const Type *rhsType) {
    if (rhsInfo.type.kind == TypeKind::ImmutReference) {
      node->rightChildExpr->semantic->adjustments.emplace_back(
          Adjustment{AdjustmentKind::Deref, *rhsType});
      node->rightChildExpr->semantic->adjustedType = *rhsType;
    }
  };
  switch (node->assignmentOp) {
  case AssignmentOperator::Assignment: {
    auto rhsInfo = checkExpr(node->rightChildExpr.get(), &lhsInfo.type);
    if (rhsInfo.type.kind == TypeKind::Error) {
      return ExprInfo{Type(TypeKind::Error), false};
    }
    return ExprInfo{Type(TypeKind::Unit),
                    lhsInfo.isDiverge || rhsInfo.isDiverge};
  }
  case AssignmentOperator::additionAssignment:
  case AssignmentOperator::subtractionAssignment:
  case AssignmentOperator::multiplicationAssignment:
  case AssignmentOperator::divisionAssignment:
  case AssignmentOperator::moduloAssignment: {
    if (!isIntegerType(lhsInfo.type)) {
      error.occur(node->leftChildExpr->pos,
                  "Invalid Expression: the left side of a compound arithmetic "
                  "assignment must have an integer type");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    auto rhsInfo = analyzeExpr(node->rightChildExpr.get());
    if (rhsInfo.type.kind == TypeKind::Error) {
      return ExprInfo{Type(TypeKind::Error), false};
    }
    if (rhsInfo.type.kind == TypeKind::Never) {
      return ExprInfo{Type(TypeKind::Unit),
                      lhsInfo.isDiverge || rhsInfo.isDiverge};
    }
    const Type *rhsType = &rhsInfo.type;
    if (rhsType->kind == TypeKind::ImmutReference && rhsType->elementType) {
      rhsType = rhsType->elementType.get();
    }
    if (rhsType->kind == TypeKind::Error) {
      return ExprInfo{Type(TypeKind::Error), false};
    }
    if (*rhsType != lhsInfo.type) {
      error.occur(node->rightChildExpr->pos,
                  "Invalid Expression: the right side of a compound arithmetic "
                  "assignment must have the same integer type as the left side "
                  "or be a shared reference to that type");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    recordAdjustments(rhsInfo, rhsType);
    return ExprInfo{Type(TypeKind::Unit),
                    lhsInfo.isDiverge || rhsInfo.isDiverge};
  }
  case AssignmentOperator::ANDAssignment:
  case AssignmentOperator::ORAssignment:
  case AssignmentOperator::XORAssignment: {
    if (!isIntegerType(lhsInfo.type) && lhsInfo.type.kind != TypeKind::Bool) {
      error.occur(node->leftChildExpr->pos,
                  "Invalid Expression: the left side of a compound bitwise "
                  "assignment must have an integer or bool type");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    auto rhsInfo = analyzeExpr(node->rightChildExpr.get());
    if (rhsInfo.type.kind == TypeKind::Error) {
      return ExprInfo{Type(TypeKind::Error), false};
    }
    if (rhsInfo.type.kind == TypeKind::Never) {
      return ExprInfo{Type(TypeKind::Unit),
                      lhsInfo.isDiverge || rhsInfo.isDiverge};
    }
    const Type *rhsType = &rhsInfo.type;
    if (rhsType->kind == TypeKind::ImmutReference && rhsType->elementType) {
      rhsType = rhsType->elementType.get();
    }
    if (rhsType->kind == TypeKind::Error) {
      return ExprInfo{Type(TypeKind::Error), false};
    }
    if (*rhsType != lhsInfo.type) {
      error.occur(node->rightChildExpr->pos,
                  "Invalid Expression: the right side of a compound bitwise "
                  "assignment must have the same type as the left side "
                  "or be a shared reference to that type");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    recordAdjustments(rhsInfo, rhsType);
    return ExprInfo{Type(TypeKind::Unit),
                    lhsInfo.isDiverge || rhsInfo.isDiverge};
  }
  case AssignmentOperator::leftShiftAssignment:
  case AssignmentOperator::rightShiftAssignment: {
    if (!isIntegerType(lhsInfo.type)) {
      error.occur(node->leftChildExpr->pos,
                  "Invalid Expression: the left side of a compound shift "
                  "assignment must have an integer type");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    auto rhsInfo = analyzeExpr(node->rightChildExpr.get());
    if (rhsInfo.type.kind == TypeKind::Error) {
      return ExprInfo{Type(TypeKind::Error), false};
    }
    if (rhsInfo.type.kind == TypeKind::Never) {
      return ExprInfo{Type(TypeKind::Unit),
                      lhsInfo.isDiverge || rhsInfo.isDiverge};
    }
    const Type *rhsType = &rhsInfo.type;
    if (rhsType->kind == TypeKind::ImmutReference && rhsType->elementType) {
      rhsType = rhsType->elementType.get();
    }
    if (rhsType->kind == TypeKind::Error) {
      return ExprInfo{Type(TypeKind::Error), false};
    }
    if (!isIntegerType(*rhsType)) {
      error.occur(node->rightChildExpr->pos,
                  "Invalid Expression: the right side of a compound shift "
                  "assignment must have an integer type or be a shared "
                  "reference to an integer; the integer types may differ");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    recordAdjustments(rhsInfo, rhsType);
    return ExprInfo{Type(TypeKind::Unit),
                    lhsInfo.isDiverge || rhsInfo.isDiverge};
  }
  }
  return ExprInfo{Type(TypeKind::Error), false};
}

std::any SemanticAnalyzer::visitArrayExpr(ArrayExprNode *node) {
  return analyzeArrayExpr(node, nullptr);
}

std::any SemanticAnalyzer::visitIndexExpr(IndexExprNode *node) {
  auto baseInfo = analyzeExpr(node->arrayExpr.get());
  if (baseInfo.type.kind == TypeKind::Error) {
    return ExprInfo{Type(TypeKind::Error), false};
  }
  if (!baseInfo.isPlace) {
    // temporary place
    baseInfo.isPlace = true;
    baseInfo.isMutablePlace = !baseInfo.mutationBlocked;
  }
  bool isMutablePlace = baseInfo.isMutablePlace;
  bool mutationBlocked = baseInfo.mutationBlocked;
  std::vector<Adjustment> adjusts;
  auto innerType =
      autoDereference(&baseInfo.type, isMutablePlace, mutationBlocked, adjusts);
  if (innerType.kind != TypeKind::Array && innerType.kind != TypeKind::Vec) {
    error.occur(node->arrayExpr->pos,
                "Invalid Expression: the base must be an array or Vec");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  Type usizeType(TypeKind::Usize);
  auto indexInfo = checkExpr(node->indexExpr.get(), &usizeType);
  if (indexInfo.type.kind == TypeKind::Error) {
    return ExprInfo{Type(TypeKind::Error), false};
  }
  // Mutable references stored in an element cannot bypass an immutable
  // container access.
  if (innerType.kind == TypeKind::Vec && !isMutablePlace) {
    mutationBlocked = true;
  }
  node->arrayExpr->semantic->adjustments = std::move(adjusts);
  node->arrayExpr->semantic->adjustedType = innerType;
  return ExprInfo{*innerType.elementType,
                  baseInfo.isDiverge || indexInfo.isDiverge, true,
                  isMutablePlace, mutationBlocked};
}

std::any SemanticAnalyzer::visitStructExpr(StructExprNode *node) {
  auto *pathPtr = node->path.get();
  pathPtr->semantic = std::make_unique<StructPathInExprSemantic>();
  auto *pathSemantic =
      static_cast<StructPathInExprSemantic *>(pathPtr->semantic.get());
  if (pathPtr->path.segments.size() != 1) {
    error.occur(pathPtr->pos, "Invalid Expression: a struct constructor path "
                              "must contain a single segment");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  const auto &segment = pathPtr->path.segments.front();
  std::string typeName = segment.pathIdentSegment;
  if (typeName == "Self") {
    if (segment.args.hasLifeTime || !segment.args.genericArgs.empty()) {
      error.occur(pathPtr->pos,
                  "Invalid Expression: `Self` in a struct constructor "
                  "does not accept type or lifetime arguments");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    if (auto structName = getSelfName()) {
      typeName = structName.value();
    } else {
      error.occur(pathPtr->pos,
                  "Invalid Expression: constructing `Self` requires "
                  "an enclosing struct implementation");
      return ExprInfo{Type(TypeKind::Error), false};
    }
  } else if (!segment.args.genericArgs.empty()) {
    error.occur(pathPtr->pos, "Invalid Expression: struct constructor `" +
                                  typeName +
                                  "` does not accept type arguments");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  if (auto *structSymbol = globalScopeVisitor->lookupStruct(typeName)) {
    pathSemantic->structSymbol = structSymbol;
    pathSemantic->originalType = Type(typeName);
    pathSemantic->adjustedType = pathSemantic->originalType;
    if (node->structExprFields.size() != structSymbol->structFields.size()) {
      error.occur(pathPtr->pos,
                  "Invalid Expression: constructor for struct `" + typeName +
                      "` must initialize every field exactly once");
      return ExprInfo{Type(TypeKind::Error), false};
    }
    std::unordered_set<std::string> declaredEntries;
    ExprInfo structInfo;
    structInfo.type = Type(TypeKind::Struct);
    structInfo.type.structName = typeName;
    for (auto &field : node->structExprFields) {
      if (declaredEntries.contains(field.identifier)) {
        error.occur(pathPtr->pos,
                    "Invalid Expression: field `" + field.identifier +
                        "` is initialized more than once in struct `" +
                        typeName + "`");
        return ExprInfo{Type(TypeKind::Error), false};
      }
      auto iterator = structSymbol->structFields.find(field.identifier);
      if (iterator == structSymbol->structFields.end()) {
        error.occur(pathPtr->pos, "Invalid Expression: struct `" + typeName +
                                      "` has no field named `" +
                                      field.identifier + "`");
        return ExprInfo{Type(TypeKind::Error), false};
      } else {
        auto &expectedType = iterator->second;
        auto exprInfo = checkExpr(field.fieldExpr.get(), &expectedType);
        if (exprInfo.type.kind == TypeKind::Error) {
          return ExprInfo{Type(TypeKind::Error), false};
        } else {
          declaredEntries.emplace(field.identifier);
          structInfo.isDiverge |= exprInfo.isDiverge;
        }
      }
    }
    return structInfo;
  } else {
    error.occur(pathPtr->pos,
                "Invalid Expression: cannot construct undefined struct `" +
                    typeName + "`");
    return ExprInfo{Type(TypeKind::Error), false};
  }
}

std::vector<ReceiverContext>
SemanticAnalyzer::generatePossibleQueue(const Type *calleeType, bool isMutable,
                                        bool isBlocked) {
  isMutable &= !isBlocked;
  std::vector<ReceiverContext> ctx;
  std::size_t dereferenceTimes = 0;
  while (calleeType->kind == TypeKind::ImmutReference ||
         calleeType->kind == TypeKind::MutReference ||
         calleeType->kind == TypeKind::Box) {
    ctx.emplace_back(ReceiverContext{dereferenceTimes, isMutable, isBlocked,
                                     ReceiverKind::Value});
    ctx.emplace_back(ReceiverContext{dereferenceTimes, isMutable, isBlocked,
                                     ReceiverKind::Shared});
    ctx.emplace_back(ReceiverContext{dereferenceTimes, isMutable, isBlocked,
                                     ReceiverKind::Mutable});
    if (calleeType->kind == TypeKind::ImmutReference) {
      isBlocked = true;
    }
    if (isBlocked) {
      isMutable = false;
    } else {
      isMutable |= calleeType->kind == TypeKind::MutReference;
    }
    ++dereferenceTimes;
    calleeType = calleeType->elementType.get();
  }
  ctx.emplace_back(ReceiverContext{dereferenceTimes, isMutable, isBlocked,
                                   ReceiverKind::Value});
  ctx.emplace_back(ReceiverContext{dereferenceTimes, isMutable, isBlocked,
                                   ReceiverKind::Shared});
  ctx.emplace_back(ReceiverContext{dereferenceTimes, isMutable, isBlocked,
                                   ReceiverKind::Mutable});
  return ctx;
}

std::any SemanticAnalyzer::visitMethodCallExpr(MethodCallExprNode *node) {
  node->semantic = std::make_unique<MethodCallExprSemantic>();
  auto *methodSemantic =
      static_cast<MethodCallExprSemantic *>(node->semantic.get());
  if (!node->callMethod.args.genericArgs.empty()) {
    error.occur(
        node->pos,
        "Invalid Expression: cannot accept a type argument in a method");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  auto calleeInfo = analyzeExpr(node->callee.get());
  if (calleeInfo.type.kind == TypeKind::Error) {
    return ExprInfo{Type(TypeKind::Error), false};
  }
  if (!calleeInfo.isPlace) {
    calleeInfo.isPlace = true;
    calleeInfo.isMutablePlace = true;
  }
  auto possibleQueue = generatePossibleQueue(
      &calleeInfo.type, calleeInfo.isMutablePlace, calleeInfo.mutationBlocked);
  auto methodName = node->callMethod.pathIdentSegment;
  FunctionSymbol *finalFunc = nullptr;
  bool finalMutable = calleeInfo.isMutablePlace;
  bool finalBlocked = calleeInfo.mutationBlocked;
  auto checkSelfParam = [](FunctionSymbol *func, const Type &self) {
    if (!func || func->paramTypes.empty()) {
      return false;
    }
    if (const auto &selfType = func->paramTypes.front();
        selfType == self && func->receiver != ReceiverKind::None) {
      return true;
    }
    return false;
  };
  auto defineFinalState = [&](const Type &finalType,
                              const ReceiverContext &ctx) {
    finalBlocked = ctx.isBlocked;
    finalMutable =
        !finalBlocked &&
        (ctx.isMutable || (ctx.kind == ReceiverKind::Value &&
                           finalType.kind == TypeKind::MutReference));
  };
  std::vector<Adjustment> adjusts;
  for (auto &ctx : possibleQueue) {
    adjusts.clear();
    adjusts.reserve(ctx.dereferenceTimes);
    auto *possibleTypePtr = &calleeInfo.type;
    for (std::size_t times = 0; times < ctx.dereferenceTimes; ++times) {
      adjusts.emplace_back(
          Adjustment{AdjustmentKind::Deref, *possibleTypePtr->elementType});
      possibleTypePtr = possibleTypePtr->elementType.get();
    }
    auto possibleType = *possibleTypePtr;
    if (ctx.kind == ReceiverKind::Shared) {
      possibleType = Type(TypeKind::ImmutReference);
      possibleType.elementType = std::make_unique<Type>(*possibleTypePtr);
      adjusts.emplace_back(Adjustment{AdjustmentKind::Borrow, possibleType});
    }
    if (ctx.kind == ReceiverKind::Mutable) {
      possibleType = Type(TypeKind::MutReference);
      possibleType.elementType = std::make_unique<Type>(*possibleTypePtr);
      adjusts.emplace_back(Adjustment{AdjustmentKind::Borrow, possibleType});
    }
    if (possibleType.kind != TypeKind::ImmutReference &&
        possibleType.kind != TypeKind::MutReference) {
      if (possibleType.kind == TypeKind::Struct) {
        auto *structSymbol =
            globalScopeVisitor->lookupStruct(possibleType.structName.value());
        if (!structSymbol) {
          error.occur(node->callee->pos,
                      "Invalid Expression: the struct is not found");
          return ExprInfo{Type(TypeKind::Error), false};
        } else {
          auto iterator = structSymbol->structSymbols.find(methodName);
          if (iterator == structSymbol->structSymbols.end()) {
            continue;
          }
          if (auto *funcSymbol =
                  dynamic_cast<FunctionSymbol *>(iterator->second.get())) {
            if (checkSelfParam(funcSymbol, possibleType)) {
              finalFunc = funcSymbol;
              defineFinalState(possibleType, ctx);
              break;
            } else {
              continue;
            }
          } else {
            continue;
          }
        }
      } else if (possibleType.kind == TypeKind::Vec ||
                 possibleType.kind == TypeKind::Box) {
        if (auto *builtinFunc =
                getOrCreateBuiltinContainerFunction(possibleType, methodName)) {
          if (checkSelfParam(builtinFunc, possibleType)) {
            finalFunc = builtinFunc;
            defineFinalState(possibleType, ctx);
            break;
          } else {
            continue;
          }
        } else {
          continue;
        }
      } else if (possibleType.kind == TypeKind::Array) {
        if (auto *builtinFunc =
                getOrCreateBuiltinArrayFunction(possibleType, methodName)) {
          if (checkSelfParam(builtinFunc, possibleType)) {
            finalFunc = builtinFunc;
            defineFinalState(possibleType, ctx);
            break;
          } else {
            continue;
          }
        } else {
          continue;
        }
      } else {
        continue;
      }
    } else {
      bool notFound = false;
      auto *innerType = possibleType.elementType.get();
      if (innerType->kind == TypeKind::Struct) {
        auto *structSymbol =
            globalScopeVisitor->lookupStruct(innerType->structName.value());
        if (!structSymbol) {
          error.occur(node->callee->pos,
                      "Invalid Expression: the struct is not found");
          return ExprInfo{Type(TypeKind::Error), false};
        } else {
          auto iterator = structSymbol->structSymbols.find(methodName);
          if (iterator == structSymbol->structSymbols.end()) {
            notFound = true;
          } else if (auto *funcSymbol = dynamic_cast<FunctionSymbol *>(
                         iterator->second.get())) {
            if (checkSelfParam(funcSymbol, possibleType)) {
              finalFunc = funcSymbol;
              defineFinalState(possibleType, ctx);
              break;
            } else {
              notFound = true;
            }
          } else {
            notFound = true;
          }
        }
      } else if (innerType->kind == TypeKind::Vec ||
                 innerType->kind == TypeKind::Box) {
        if (auto *builtinFunc =
                getOrCreateBuiltinContainerFunction(*innerType, methodName)) {
          if (checkSelfParam(builtinFunc, possibleType)) {
            finalFunc = builtinFunc;
            defineFinalState(possibleType, ctx);
            break;
          } else {
            notFound = true;
          }
        } else {
          notFound = true;
        }
      } else if (innerType->kind == TypeKind::Array) {
        if (auto *builtinFunc =
                getOrCreateBuiltinArrayFunction(*innerType, methodName)) {
          if (checkSelfParam(builtinFunc, possibleType)) {
            finalFunc = builtinFunc;
            defineFinalState(possibleType, ctx);
            break;
          } else {
            notFound = true;
          }
        } else {
          notFound = true;
        }
      } else {
        notFound = true;
      }
      if (notFound) {
        if (methodName == "clone") {
          auto *cloneFunc = getOrCreateBuiltinClone(*possibleType.elementType);
          if (checkSelfParam(cloneFunc, possibleType)) {
            finalFunc = cloneFunc;
            defineFinalState(possibleType, ctx);
            break;
          } else
            continue;
        }
      }
    }
  }
  if (!finalFunc) {
    error.occur(node->pos, "Invalid Expression: method not found");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  if (finalFunc->receiver == ReceiverKind::Mutable &&
      (finalBlocked || !finalMutable)) {
    error.occur(node->pos, "Invalid Expression: method cannot access");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  methodSemantic->calleeSymbol = finalFunc;
  // check the params type
  if (finalFunc->paramTypes.size() != node->callParams.size() + 1) {
    error.occur(node->callee->pos, "Invalid Expression: the size of the "
                                   "parameters must match the function");
    return ExprInfo{Type(TypeKind::Error), false};
  }
  bool isDiverge = calleeInfo.isDiverge;
  for (std::size_t i = 0; i < node->callParams.size(); ++i) {
    auto paramInfo =
        checkExpr(node->callParams[i].get(), &finalFunc->paramTypes[i + 1]);

    if (paramInfo.type.kind == TypeKind::Error) {
      return ExprInfo{Type(TypeKind::Error), false};
    }
    isDiverge |= paramInfo.isDiverge;
  }
  methodSemantic->receiverAdjustments = std::move(adjusts);
  return ExprInfo{finalFunc->returnType, isDiverge};
}

std::any SemanticAnalyzer::visitReturnExpr(ReturnExprNode *node) {
  if (node->returnValue) {
    auto returnInfo = checkExpr(node->returnValue.value().get(),
                                functionInfoContainer->expected);
    if (returnInfo.type.kind == TypeKind::Error) {
      return ExprInfo{Type(TypeKind::Error), false};
    }
  } else {
    auto returnNode = std::make_unique<UnitExprNode>(node->pos);
    auto returnInfo =
        checkExpr(returnNode.get(), functionInfoContainer->expected);
    if (returnInfo.type.kind == TypeKind::Error) {
      return ExprInfo{Type(TypeKind::Error), false};
    }
  }
  return ExprInfo{Type(TypeKind::Never), true};
}

std::any SemanticAnalyzer::visitContinueExpr(ContinueExprNode *node) {
  if (loopStack.empty()) {
    error.occur(node->pos, "Invalid Expression: `continue` is only allowed "
                           "inside a `loop` or `while` loop");
    return ExprInfo{Type(TypeKind::Error), false};
  } else if (const auto &topCtx = loopStack.top();
             topCtx.kind == LoopKind::While) {
    if (topCtx.checkingCondition) {
      error.occur(node->pos,
                  "Invalid Expression: `continue` cannot target a `while` loop "
                  "from within that loop's condition");
      return ExprInfo{Type(TypeKind::Error), false};
    }
  }
  return ExprInfo{Type(TypeKind::Never), true};
}

std::any SemanticAnalyzer::visitBreakExpr(BreakExprNode *node) {
  if (loopStack.empty()) {
    error.occur(node->pos, "Invalid Expression: `break` is only allowed "
                           "inside a `loop` or `while` loop");
    return ExprInfo{Type(TypeKind::Error), false};
  } else {
    auto &topCtx = loopStack.top();
    if (topCtx.kind == LoopKind::While) {
      if (node->hasExplicitValue) {
        error.occur(node->pos,
                    "Invalid Expression: `break` targeting a `while` loop "
                    "cannot carry a value; use `break;`");
        return ExprInfo{Type(TypeKind::Error), false};
      }
      if (topCtx.checkingCondition) {
        error.occur(node->pos,
                    "Invalid Expression: `break` cannot target a `while` loop "
                    "from within that loop's condition");
        return ExprInfo{Type(TypeKind::Error), false};
      }
      checkExpr(node->loopValue.get(), nullptr);
    } else {
      topCtx.breakInfos.emplace_back(
          checkExpr(node->loopValue.get(), topCtx.expected));
      topCtx.breakNodes.push_back(node);
    }
  }
  return ExprInfo{Type(TypeKind::Never), true};
}

std::any SemanticAnalyzer::visitIfExpr(IfExprNode *node) {
  return analyzeIfExpr(node, nullptr);
}

std::any SemanticAnalyzer::visitInfiniteLoopExpr(InfiniteLoopExprNode *node) {
  return analyzeLoopExpr(node, nullptr);
}

std::any SemanticAnalyzer::visitPredicateLoopExpr(PredicateLoopExprNode *node) {
  auto _ = LoopGuard(loopStack, LoopContext(LoopKind::While));
  Type boolType(TypeKind::Bool);
  loopStack.top().checkingCondition = true;
  auto conditionInfo = checkExpr(node->conditions.get(), &boolType);
  loopStack.top().checkingCondition = false;
  if (conditionInfo.type.kind == TypeKind::Error) {
    return ExprInfo{Type(TypeKind::Error), false};
  }
  Type unitType(TypeKind::Unit);
  auto bodyInfo = checkExpr(node->body.get(), &unitType);
  if (bodyInfo.type.kind == TypeKind::Error) {
    return ExprInfo{Type(TypeKind::Error), false};
  }
  return ExprInfo{Type(TypeKind::Unit), conditionInfo.isDiverge};
}

Type SemanticAnalyzer::LeastUpperBoundCoercion(const std::vector<Type> &types) {
  if (std::any_of(types.begin(), types.end(), [](const Type &type) {
        return type.kind == TypeKind::Error;
      })) {
    return Type(TypeKind::Error);
  }
  bool foundNotNeverType = false;
  Type targetType(TypeKind::Never);
  for (std::size_t i = 0; i < types.size(); ++i) {
    const auto &type = types[i];
    if (type.kind != TypeKind::Never && !foundNotNeverType) {
      targetType = type;
      foundNotNeverType = true;
    }
    if (foundNotNeverType) {
      if (checkCoercionEnability(type, targetType)) {
        continue;
      } else {
        bool allcheckCoercionEnability = true;
        for (std::size_t j = 0; j < i; ++j) {
          allcheckCoercionEnability &= checkCoercionEnability(types[j], type);
        }
        if (checkCoercionEnability(targetType, type) &&
            allcheckCoercionEnability) {
          targetType = type;
        } else {
          error.occur(type.pos,
                      "Invalid Type: expression results cannot be coerced "
                      "to a common type");
          targetType = Type(TypeKind::Error);
        }
      }
    }
  }
  return targetType;
}

bool SemanticAnalyzer::checkCoercionEnability(const Type &from,
                                              const Type &to) {
  if (from.kind == TypeKind::Never) {
    return true;
  }
  if (from == to) {
    return true;
  }
  if (from.kind == TypeKind::MutReference &&
      to.kind == TypeKind::ImmutReference &&
      *from.elementType == *to.elementType) {
    return true;
  }
  bool canReachTarget = false;
  if (to.kind == TypeKind::ImmutReference &&
      (from.kind == TypeKind::ImmutReference ||
       from.kind == TypeKind::MutReference)) {
    const auto *fromCopy = from.elementType.get();
    do {
      if (*fromCopy == *to.elementType) {
        canReachTarget = true;
        break;
      }
      if ((fromCopy->kind == TypeKind::ImmutReference ||
           fromCopy->kind == TypeKind::MutReference ||
           fromCopy->kind == TypeKind::Box) &&
          fromCopy->elementType) {
        fromCopy = fromCopy->elementType.get();
      } else
        break;
    } while (fromCopy->kind == TypeKind::ImmutReference ||
             fromCopy->kind == TypeKind::MutReference ||
             fromCopy->kind == TypeKind::Box);
    canReachTarget |= *fromCopy == *to.elementType;
    if (canReachTarget) {
      return true;
    }
  }
  bool isShared = false;
  if (to.kind == TypeKind::MutReference &&
      from.kind == TypeKind::MutReference) {
    const auto *fromCopy = from.elementType.get();
    do {
      if (*fromCopy == *to.elementType) {
        canReachTarget = true;
        break;
      }
      if ((fromCopy->kind == TypeKind::ImmutReference ||
           fromCopy->kind == TypeKind::MutReference ||
           fromCopy->kind == TypeKind::Box) &&
          fromCopy->elementType) {
        isShared |= fromCopy->kind == TypeKind::ImmutReference;
        fromCopy = fromCopy->elementType.get();
      } else
        break;
    } while (fromCopy->kind == TypeKind::ImmutReference ||
             fromCopy->kind == TypeKind::MutReference ||
             fromCopy->kind == TypeKind::Box);
    canReachTarget |= *fromCopy == *to.elementType;
    if (canReachTarget && !isShared) {
      return true;
    }
  }
  return false;
}

bool SemanticAnalyzer::checkCoercionEnability(const Type &from, const Type &to,
                                              std::vector<Adjustment> &adjust) {
  if (from == to) {
    return true;
  }
  if (from.kind == TypeKind::Never) {
    adjust.emplace_back(Adjustment{AdjustmentKind::NeverToAny, to});
    return true;
  }
  if (from.kind == TypeKind::MutReference &&
      to.kind == TypeKind::ImmutReference &&
      *from.elementType == *to.elementType) {
    adjust.emplace_back(Adjustment{AdjustmentKind::MutToShared, to});
    return true;
  }
  bool canReachTarget = false;
  if (to.kind == TypeKind::ImmutReference &&
      (from.kind == TypeKind::ImmutReference ||
       from.kind == TypeKind::MutReference)) {
    const auto *fromCopy = from.elementType.get();
    adjust.emplace_back(Adjustment{AdjustmentKind::Deref, *fromCopy});
    do {
      if (*fromCopy == *to.elementType) {
        break;
      }
      if ((fromCopy->kind == TypeKind::ImmutReference ||
           fromCopy->kind == TypeKind::MutReference ||
           fromCopy->kind == TypeKind::Box) &&
          fromCopy->elementType) {
        adjust.emplace_back(
            Adjustment{AdjustmentKind::Deref, *fromCopy->elementType});
        fromCopy = fromCopy->elementType.get();
      } else
        break;
    } while (fromCopy->kind == TypeKind::ImmutReference ||
             fromCopy->kind == TypeKind::MutReference ||
             fromCopy->kind == TypeKind::Box);
    canReachTarget |= *fromCopy == *to.elementType;
    if (*fromCopy == *to.elementType) {
      adjust.emplace_back(Adjustment{AdjustmentKind::Borrow, to});
      canReachTarget = true;
    }
    if (canReachTarget) {
      return true;
    } else {
      adjust.clear();
    }
  }
  bool isShared = false;
  if (to.kind == TypeKind::MutReference &&
      from.kind == TypeKind::MutReference) {
    const auto *fromCopy = from.elementType.get();
    adjust.emplace_back(Adjustment{AdjustmentKind::Deref, *fromCopy});
    do {
      if (*fromCopy == *to.elementType) {
        break;
      }
      if ((fromCopy->kind == TypeKind::ImmutReference ||
           fromCopy->kind == TypeKind::MutReference ||
           fromCopy->kind == TypeKind::Box) &&
          fromCopy->elementType) {
        isShared |= fromCopy->kind == TypeKind::ImmutReference;
        adjust.emplace_back(
            Adjustment{AdjustmentKind::Deref, *fromCopy->elementType});
        fromCopy = fromCopy->elementType.get();
      } else
        break;
    } while (fromCopy->kind == TypeKind::ImmutReference ||
             fromCopy->kind == TypeKind::MutReference ||
             fromCopy->kind == TypeKind::Box);
    if (*fromCopy == *to.elementType) {
      adjust.emplace_back(Adjustment{AdjustmentKind::Borrow, to});
      canReachTarget = true;
    }
    if (canReachTarget && !isShared) {
      return true;
    } else {
      adjust.clear();
    }
  }
  return false;
}

bool SemanticAnalyzer::checkCoercionPermission(const ExprInfo &from,
                                               const Type &to) {
  // we assume that type check has already done
  if (from.type.kind == TypeKind::MutReference &&
      to.kind == TypeKind::MutReference && from.mutationBlocked) {
    {
      return false;
    }
  }
  return true;
}

void SemanticAnalyzer::resolveType(Type &type,
                                   std::optional<std::string> structName) {
  if (type.kind == TypeKind::Struct) {
    if (type.structName == "Self") {
      if (!structName || !globalScopeVisitor->lookupStruct(*structName)) {
        error.occur(type.pos,
                    "Invalid Type: `Self` requires an enclosing implementation "
                    "of a defined struct");
        type = Type(TypeKind::Error, type.pos);
        return;
      }
      type.structName = structName;
    }
    auto *structSymbol =
        globalScopeVisitor->lookupStruct(type.structName.value());
    if (!structSymbol) {
      error.occur(type.pos, "Invalid Type: undefined struct `" +
                                type.structName.value() + "`");
      type = Type(TypeKind::Error, type.pos);
    }
  } else if (type.kind >= TypeKind::MutReference &&
             type.kind <= TypeKind::Box) {
    resolveType(*type.elementType, structName);
    if (type.elementType->kind == TypeKind::Error) {
      type = Type(TypeKind::Error, type.pos);
    }
  } else if (type.kind == TypeKind::Array) {
    resolveType(*type.elementType, structName);
    if (type.elementType->kind == TypeKind::Error) {
      type = Type(TypeKind::Error, type.pos);
      return;
    }
    resolveArrayLengths(type);
  }
}

void SemanticAnalyzer::typeResolveForFunction(FunctionSymbol *symbol,
                                              const std::string *structName) {
  std::optional<std::string> optionalStructName =
      structName ? std::optional<std::string>(*structName) : std::nullopt;
  resolveType(symbol->returnType, optionalStructName);
  for (auto &type : symbol->paramTypes) {
    resolveType(type, optionalStructName);
  }
}

void SemanticAnalyzer::typeResolveForStruct(StructSymbol *symbol) {
  for (auto &[name, type] : symbol->structFields) {
    resolveType(type, symbol->name);
  }
  for (auto &[name, associate] : symbol->structSymbols) {
    if (auto *function = dynamic_cast<FunctionSymbol *>(associate.get())) {
      typeResolveForFunction(function, &symbol->name);
    }
  }
}

void SemanticAnalyzer::checkItemType() {
  for (const auto &globalSymbol : globalScopeVisitor->typeNameSpace) {
    if (auto *structSymbol =
            dynamic_cast<StructSymbol *>(globalSymbol.second.get())) {
      typeResolveForStruct(structSymbol);
    }
  }
  for (const auto &globalSymbol : globalScopeVisitor->valueNameSpace) {
    if (auto *functionSymbol =
            dynamic_cast<FunctionSymbol *>(globalSymbol.second.get())) {
      typeResolveForFunction(functionSymbol);
    }
  }
}

ExprInfo SemanticAnalyzer::analyzeExpr(ExprNode *node, Type *expected) {
  auto saveSemantic = [node](ExprInfo info) {
    // Keep specialized semantics (bindings, callees and fields) intact.
    if (!node->semantic) {
      node->semantic = std::make_unique<ExprSemantic>();
    }
    node->semantic->adjustments.clear();
    node->semantic->originalType = info.type;
    node->semantic->adjustedType = info.type;
    node->semantic->isPlace = info.isPlace;
    node->semantic->isMutablePlace = info.isMutablePlace;
    node->semantic->isDiverge = info.isDiverge;
    return info;
  };
  switch (node->kind) {
  case nodeKind::LiteralExpr:
    return saveSemantic(analyzeLiteralExpr(static_cast<LiteralExprNode *>(node),
                                           expected, false));
  case nodeKind::ArrayExpr:
    return saveSemantic(
        analyzeArrayExpr(static_cast<ArrayExprNode *>(node), expected));
  case nodeKind::BlockExpr:
    return saveSemantic(
        analyzeBlockExpr(static_cast<BlockExprNode *>(node), expected));
  case nodeKind::IfExpr:
    return saveSemantic(
        analyzeIfExpr(static_cast<IfExprNode *>(node), expected));
  case nodeKind::InfiniteLoopExpr:
    return saveSemantic(
        analyzeLoopExpr(static_cast<InfiniteLoopExprNode *>(node), expected));
  default:
    return saveSemantic(std::any_cast<ExprInfo>(node->accept(this)));
  }
}

auto SemanticAnalyzer::analyzeIfExpr(IfExprNode *node, Type *expected)
    -> ExprInfo {
  Type boolType(TypeKind::Bool);
  auto condition = checkExpr(node->conditions.get(), &boolType);
  if (condition.type.kind == TypeKind::Error) {
    return ExprInfo{Type(TypeKind::Error), false};
  }
  if (expected) {
    if (!node->elseExpr && !node->elseIfExpr) {
      Type unitType(TypeKind::Unit);
      auto exprInfo = checkExpr(node->body.get(), &unitType);
      if (exprInfo.type.kind == TypeKind::Error) {
        return ExprInfo{Type(TypeKind::Error), false};
      }
      if (!checkCoercionEnability(unitType, *expected)) {
        error.occur(
            expected->pos,
            "Invalid Expression: an `if` expression without `else` "
            "has type `()`, which is incompatible with the expected type");
        return ExprInfo{Type(TypeKind::Error), false};
      }
      return ExprInfo{*expected, condition.isDiverge};
    } else {
      if (node->elseExpr) {
        auto bodyInfo = checkExpr(node->body.get(), expected);
        auto elseInfo = checkExpr(node->elseExpr.value().get(), expected);
        if (bodyInfo.type.kind == TypeKind::Error ||
            elseInfo.type.kind == TypeKind::Error) {
          return ExprInfo{Type(TypeKind::Error), false};
        }
        return ExprInfo{*expected, condition.isDiverge || (bodyInfo.isDiverge &&
                                                           elseInfo.isDiverge)};
      } else {
        auto bodyInfo = checkExpr(node->body.get(), expected);
        auto elseIfInfo = checkExpr(node->elseIfExpr.value().get(), expected);
        if (bodyInfo.type.kind == TypeKind::Error ||
            elseIfInfo.type.kind == TypeKind::Error) {
          return ExprInfo{Type(TypeKind::Error), false};
        }
        return ExprInfo{*expected,
                        condition.isDiverge ||
                            (bodyInfo.isDiverge && elseIfInfo.isDiverge)};
      }
    }
  } else {
    if (!node->elseExpr && !node->elseIfExpr) {
      Type unitType(TypeKind::Unit);
      auto exprInfo = checkExpr(node->body.get(), &unitType);
      if (exprInfo.type.kind == TypeKind::Error) {
        return ExprInfo{Type(TypeKind::Error), false};
      }
      return ExprInfo{unitType, condition.isDiverge};
    } else {
      if (node->elseExpr) {
        auto bodyInfo = checkExpr(node->body.get(), expected);
        auto elseInfo = checkExpr(node->elseExpr.value().get(), expected);
        if (bodyInfo.type.kind == TypeKind::Error ||
            elseInfo.type.kind == TypeKind::Error) {
          return ExprInfo{Type(TypeKind::Error), false};
        }
        std::vector<Type> branchTypes;
        branchTypes.emplace_back(bodyInfo.type);
        branchTypes.emplace_back(elseInfo.type);
        auto mergeType = LeastUpperBoundCoercion(branchTypes);
        if (mergeType.kind != TypeKind::Error) {
          std::vector<Adjustment> bodyAdjustments, elseAdjustments;
          if (!checkCoercionEnability(bodyInfo.type, mergeType,
                                      bodyAdjustments) ||
              !checkCoercionPermission(bodyInfo, mergeType) ||
              !checkCoercionEnability(elseInfo.type, mergeType,
                                      elseAdjustments) ||
              !checkCoercionPermission(elseInfo, mergeType)) {
            error.occur(node->pos,
                        "Invalid Expression: branches cannot coerce to the "
                        "common type with the required mutability");
            return ExprInfo{Type(TypeKind::Error), false};
          }
          node->body->semantic->adjustments = std::move(bodyAdjustments);
          node->body->semantic->adjustedType = mergeType;
          node->elseExpr.value()->semantic->adjustments =
              std::move(elseAdjustments);
          node->elseExpr.value()->semantic->adjustedType = mergeType;
        }
        return ExprInfo{mergeType, condition.isDiverge || (bodyInfo.isDiverge &&
                                                           elseInfo.isDiverge)};
      } else {
        auto bodyInfo = checkExpr(node->body.get(), expected);
        auto elseIfInfo = checkExpr(node->elseIfExpr.value().get(), expected);
        if (bodyInfo.type.kind == TypeKind::Error ||
            elseIfInfo.type.kind == TypeKind::Error) {
          return ExprInfo{Type(TypeKind::Error), false};
        }
        std::vector<Type> branchTypes;
        branchTypes.emplace_back(bodyInfo.type);
        branchTypes.emplace_back(elseIfInfo.type);
        auto mergeType = LeastUpperBoundCoercion(branchTypes);
        if (mergeType.kind != TypeKind::Error) {
          std::vector<Adjustment> bodyAdjustments, elseIfAdjustments;
          if (!checkCoercionEnability(bodyInfo.type, mergeType,
                                      bodyAdjustments) ||
              !checkCoercionPermission(bodyInfo, mergeType) ||
              !checkCoercionEnability(elseIfInfo.type, mergeType,
                                      elseIfAdjustments) ||
              !checkCoercionPermission(elseIfInfo, mergeType)) {
            error.occur(node->pos,
                        "Invalid Expression: branches cannot coerce to the "
                        "common type with the required mutability");
            return ExprInfo{Type(TypeKind::Error), false};
          }
          node->body->semantic->adjustments = std::move(bodyAdjustments);
          node->body->semantic->adjustedType = mergeType;
          node->elseIfExpr.value()->semantic->adjustments =
              std::move(elseIfAdjustments);
          node->elseIfExpr.value()->semantic->adjustedType = mergeType;
        }
        return ExprInfo{mergeType,
                        condition.isDiverge ||
                            (bodyInfo.isDiverge && elseIfInfo.isDiverge)};
      }
    }
  }
}

auto SemanticAnalyzer::analyzeArrayExpr(ArrayExprNode *node, Type *expected)
    -> ExprInfo {
  bool isDiverge = false;
  Type arrayType = Type(TypeKind::Array);
  auto checkRepeatedElementCopy = [&]() {
    if (arrayType.kind == TypeKind::Array) {
      auto length = std::get<long long>(arrayType.arrayLen->value);
      if (length > 1 && !hasTrait(arrayType, BuiltinTrait::Copy)) {
        error.occur(node->pos,
                    "Invalid Expression: the element type cannot copy");
        arrayType = Type(TypeKind::Error);
      }
    }
  };
  if (!expected || expected->kind != TypeKind::Array) {
    if (std::holds_alternative<std::vector<std::unique_ptr<ExprNode>>>(
            node->arrayEntries)) {
      const auto &entries =
          std::get<std::vector<std::unique_ptr<ExprNode>>>(node->arrayEntries);
      std::vector<Type> arrayTypes;
      arrayTypes.reserve(entries.size());
      std::vector<ExprInfo> arrayInfos;
      for (const auto &childExpr : entries) {
        arrayInfos.emplace_back(analyzeExpr(childExpr.get()));
        auto &exprInfo = arrayInfos.back();
        isDiverge |= exprInfo.isDiverge;
        arrayTypes.emplace_back(exprInfo.type);
      }
      auto entryType = LeastUpperBoundCoercion(arrayTypes);
      if (entryType.kind == TypeKind::Error) {
        return ExprInfo{Type(TypeKind::Error), false};
      }
      for (std::size_t i = 0; i < arrayInfos.size(); ++i) {
        auto &exprInfo = arrayInfos[i];
        std::vector<Adjustment> pending;
        if (!checkCoercionEnability(exprInfo.type, entryType, pending) ||
            !checkCoercionPermission(exprInfo, entryType)) {
          error.occur(entries[i]->pos,
                      "Invalid Expression: array element cannot coerce to the "
                      "common type with the required mutability");
          return ExprInfo{Type(TypeKind::Error), false};
        }
        entries[i]->semantic->adjustments = std::move(pending);
        entries[i]->semantic->adjustedType = entryType;
      }
      arrayType.elementType = std::make_unique<Type>(std::move(entryType));
      ConstValue length;
      length.state = EvaluatedState::Evaluated;
      length.value = static_cast<long long>(entries.size());
      arrayType.arrayLen.emplace(length);
    } else {
      const auto &[value, length] = std::get<
          std::pair<std::unique_ptr<ExprNode>, std::unique_ptr<ConstExprNode>>>(
          node->arrayEntries);
      auto exprInfo = analyzeExpr(value.get());
      if (exprInfo.type.kind == TypeKind::Error) {
        return ExprInfo{Type(TypeKind::Error), false};
      }
      isDiverge = exprInfo.isDiverge;
      auto entryType = std::make_unique<Type>(exprInfo.type);
      arrayType.elementType = std::move(entryType);
      arrayType.arrayLen.emplace();
      arrayType.arrayLen->ctx = length.get();
      resolveArrayLengths(arrayType);
      checkRepeatedElementCopy();
    }
  } else {
    auto *expectedType = expected->elementType.get();
    auto expectedLength = std::get<long long>(expected->arrayLen->value);
    if (std::holds_alternative<std::vector<std::unique_ptr<ExprNode>>>(
            node->arrayEntries)) {
      const auto &entries =
          std::get<std::vector<std::unique_ptr<ExprNode>>>(node->arrayEntries);
      for (const auto &childExpr : entries) {
        auto exprInfo = checkExpr(childExpr.get(), expectedType);
        isDiverge |= exprInfo.isDiverge;
        if (exprInfo.type.kind == TypeKind::Error) {
          arrayType.kind = TypeKind::Error;
        }
      }
      if (arrayType.kind == TypeKind::Array) {
        arrayType.elementType = std::make_unique<Type>(*expectedType);
      }
      if (!std::cmp_equal(expectedLength, entries.size())) {
        error.occur(node->pos,
                    "Invalid Expression: array literal has " +
                        std::to_string(entries.size()) +
                        " elements, but the expected array length is " +
                        std::to_string(expectedLength));
        arrayType = Type(TypeKind::Error);
      } else {
        ConstValue length;
        length.state = EvaluatedState::Evaluated;
        length.value = static_cast<long long>(entries.size());
        arrayType.arrayLen.emplace(length);
      }
    } else {
      const auto &[value, length] = std::get<
          std::pair<std::unique_ptr<ExprNode>, std::unique_ptr<ConstExprNode>>>(
          node->arrayEntries);
      auto exprInfo = checkExpr(value.get(), expectedType);
      isDiverge = exprInfo.isDiverge;
      if (exprInfo.type.kind == TypeKind::Error) {
        arrayType.kind = TypeKind::Error;
      }
      if (arrayType.kind == TypeKind::Array) {
        arrayType.elementType = std::make_unique<Type>(*expectedType);
        arrayType.arrayLen.emplace();
        arrayType.arrayLen->ctx = length.get();
        resolveArrayLengths(arrayType);
        if (arrayType.kind == TypeKind::Error) {
          return ExprInfo{Type(TypeKind::Error), false};
        }
        auto actualLength = std::get<long long>(arrayType.arrayLen->value);
        if (actualLength != expectedLength) {
          error.occur(node->pos,
                      "Invalid Expression: repeated array has length " +
                          std::to_string(actualLength) +
                          ", but the expected array length is " +
                          std::to_string(expectedLength));
          arrayType = Type(TypeKind::Error);
        }
        checkRepeatedElementCopy();
      }
    }
  }
  return ExprInfo{arrayType, arrayType.kind != TypeKind::Error && isDiverge};
}

auto SemanticAnalyzer::analyzeLoopExpr(InfiniteLoopExprNode *node,
                                       Type *expected) -> ExprInfo {
  auto _ = LoopGuard(loopStack, LoopContext(LoopKind::Loop));
  loopStack.top().expected = expected;
  Type unitType(TypeKind::Unit);
  auto bodyInfo = checkExpr(node->body.get(), &unitType);
  if (bodyInfo.type.kind == TypeKind::Error) {
    return ExprInfo{Type(TypeKind::Error), false};
  }
  auto &ctx = loopStack.top();
  if (ctx.breakInfos.empty()) {
    return ExprInfo{Type(TypeKind::Never), true};
  } else {
    // The loop diverges only if every break operand diverges.
    bool isDiverge = true;
    if (expected) {
      bool hasErrorType = false;
      for (auto &info : ctx.breakInfos) {
        isDiverge &= info.isDiverge;
        hasErrorType |= info.type.kind == TypeKind::Error;
      }
      if (hasErrorType) {
        return ExprInfo{Type(TypeKind::Error), isDiverge};
      } else {
        return ExprInfo{*expected, isDiverge};
      }
    } else {
      std::vector<Type> breakTypes;
      for (auto &info : ctx.breakInfos) {
        isDiverge &= info.isDiverge;
        breakTypes.emplace_back(info.type);
      }
      auto mergeType = LeastUpperBoundCoercion(breakTypes);
      if (mergeType.kind != TypeKind::Error) {
        for (std::size_t i = 0; i < ctx.breakNodes.size(); ++i) {
          auto &info = ctx.breakInfos[i];
          auto *valueNode = ctx.breakNodes[i]->loopValue.get();
          std::vector<Adjustment> pending;
          if (!checkCoercionEnability(info.type, mergeType, pending) ||
              !checkCoercionPermission(info, mergeType)) {
            error.occur(valueNode->pos,
                        "Invalid Expression: break value cannot coerce to the "
                        "common type with the required mutability");
            return ExprInfo{Type(TypeKind::Error), false};
          }
          valueNode->semantic->adjustments = std::move(pending);
          valueNode->semantic->adjustedType = mergeType;
        }
      }
      return ExprInfo{mergeType, isDiverge};
    }
  }
}

ExprInfo SemanticAnalyzer::checkExpr(ExprNode *node, Type *expected) {
  auto saveAdjustedSemantic = [node](ExprInfo info) {
    node->semantic->adjustedType = info.type;
    return info;
  };
  auto exprInfo = analyzeExpr(node, expected);
  if (exprInfo.type.kind == TypeKind::Error ||
      (expected && expected->kind == TypeKind::Error)) {
    exprInfo.type = Type(TypeKind::Error);
  } else {
    if (expected) {
      std::vector<Adjustment> pending;
      bool enability =
          checkCoercionEnability(exprInfo.type, *expected, pending);
      bool permission = checkCoercionPermission(exprInfo, *expected);
      if (!enability) {
        error.occur(node->pos,
                    "Invalid Expression: the expression type is incompatible "
                    "with the expected type; no supported coercion applies");
        exprInfo.type = Type(TypeKind::Error);
      } else if (!permission) {
        error.occur(node->pos,
                    "Invalid Expression: mutability-blocked type cannot "
                    "coercion to a mutable reference");
        exprInfo.type = Type(TypeKind::Error);
      } else {
        exprInfo.type = *expected;
        node->semantic->adjustments = std::move(pending);
      }
    }
  }
  return saveAdjustedSemantic(std::move(exprInfo));
}

bool SemanticAnalyzer::hasTrait(const Type &type, BuiltinTrait trait) {
  if (type.kind >= TypeKind::I32 && type.kind <= TypeKind::Bool) {
    return true;
  } else if (type.kind == TypeKind::ImmutReference) {
    switch (trait) {
    case BuiltinTrait::Copy:
    case BuiltinTrait::Clone:
      return true;
    case BuiltinTrait::PartialEq:
    case BuiltinTrait::Eq:
      return hasTrait(*type.elementType, trait);
    }
  } else if (type.kind == TypeKind::MutReference) {
    switch (trait) {
    case BuiltinTrait::Copy:
    case BuiltinTrait::Clone:
      return false;
    case BuiltinTrait::PartialEq:
    case BuiltinTrait::Eq:
      return hasTrait(*type.elementType, trait);
    }
  } else if (type.kind == TypeKind::Array) {
    return hasTrait(*type.elementType, trait);
  } else if (type.kind == TypeKind::Struct) {
    const auto *symbol =
        globalScopeVisitor->lookupStruct(type.structName.value());
    switch (trait) {
    case BuiltinTrait::Copy:
      return symbol->traitValid.test(0);
    case BuiltinTrait::Clone:
      return symbol->traitValid.test(1);
    case BuiltinTrait::PartialEq:
      return symbol->traitValid.test(2);
    case BuiltinTrait::Eq:
      return symbol->traitValid.test(3);
    }
  } else if (type.kind == TypeKind::Box || type.kind == TypeKind::Vec) {
    switch (trait) {
    case BuiltinTrait::Copy:
      return false;
    case BuiltinTrait::Clone:
    case BuiltinTrait::PartialEq:
    case BuiltinTrait::Eq:
      return hasTrait(*type.elementType, trait);
    }
  }
  return false;
}
