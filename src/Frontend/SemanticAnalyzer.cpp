#include "SemanticAnalyzer.hpp"
#include "AST.hpp"
#include "Scope.hpp"
#include <memory>

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
  Type arrayType;
  arrayType.elementType = std::make_unique<Type>(
      std::any_cast<Type>(node->typeParam->accept(this)));
  if (arrayType.elementType->kind != TypeKind::Error) {
    arrayType.kind = TypeKind::Array;
  } else {
    arrayType.kind = TypeKind::Error;
  }
  arrayType.arrayLen.emplace();
  arrayType.arrayLen->ctx = node->length.get();
  return arrayType;
}

std::any SemanticAnalyzer::visitCrate(CrateNode *node) {
  for (std::size_t i = 0; i < node->items.size(); ++i) {
    node->items[i]->accept(this);
  }
  return {};
}

std::any SemanticAnalyzer::visitFunctionDeclItem(FunctionDeclItemNode *node) {
  auto *structScope = dynamic_cast<implementationScope *>(currentScope.get());
  auto *globalScope =
      structScope ? currentScope->parentScope : currentScope.get();
  auto modifySelfName = [&](std::unique_ptr<Type> &type) {
    auto *visitorPtr = type.get();
    while (visitorPtr->elementType) {
      visitorPtr = visitorPtr->elementType.get();
    }
    if (auto structName = visitorPtr->structName;
        structName && structName == "Self") {
      if (currentScope.get() == globalScope) {
        error.occur(node->pos,
                    "Invalid Function: Self type cannot exist in top function");
      } else {
        visitorPtr->structName = structScope->structName;
      }
    }
  };
  if (node->returnType != std::nullopt) {
    node->actualReturnType = std::make_unique<Type>(
        std::any_cast<Type>(node->returnType->get()->accept(this)));
    modifySelfName(node->actualReturnType);
  } else {
    node->actualReturnType = std::make_unique<Type>(TypeKind::Unit);
  }
  auto parent = std::move(currentScope);
  currentScope = std::make_unique<Scope>(parent.get());
  for (auto &param : node->functionParam) {
    if (currentScope->lookup(param.identifier)) {
      error.occur(node->pos, "Invalid Function: duplicated parameter name");
    } else {
      param.paramType->actualType = std::make_unique<Type>(
          std::any_cast<Type>(param.paramType->accept(this)));
      modifySelfName(param.paramType->actualType);
      currentScope->define(param.identifier,
                           LocalSymbol(node->pos, param.identifier, param.isMut,
                                       *param.paramType->actualType));
    }
  }
  node->functionBody->accept(this);
  currentScope = std::move(parent);
  return {};
}

std::any SemanticAnalyzer::visitConstantDeclItem(ConstantDeclItemNode *node){

}
