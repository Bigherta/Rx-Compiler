#include "SymbolCollector.hpp"
#include "AST.hpp"
#include <any>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>
std::any SymbolCollector::visitUnitType(UnitTypeNode *node) {
  return Type(TypeKind::Unit, node->pos);
}

std::any SymbolCollector::visitPathInType(PathInTypeNode *node) {
  if (node->path.segments.size() > 1) {
    error.occur(node->pos,
                "Invalid Type: a type path must contain a single segment");
    return Type(TypeKind::Error, node->pos);
  }
  const auto &seg = node->path.segments.front();
  if (seg.pathIdentSegment == "i32" || seg.pathIdentSegment == "u32" ||
      seg.pathIdentSegment == "isize" || seg.pathIdentSegment == "usize" ||
      seg.pathIdentSegment == "bool") {
    if (!seg.args.genericArgs.empty() || seg.args.hasLifeTime) {
      error.occur(node->pos,
                  "Invalid Type: primitive type `" + seg.pathIdentSegment +
                      "` does not accept type or lifetime arguments");
      return Type(TypeKind::Error, node->pos);
    }
    if (seg.pathIdentSegment == "i32") {
      return Type(TypeKind::I32, node->pos);
    } else if (seg.pathIdentSegment == "isize") {
      return Type(TypeKind::Isize, node->pos);
    } else if (seg.pathIdentSegment == "u32") {
      return Type(TypeKind::U32, node->pos);
    } else if (seg.pathIdentSegment == "usize") {
      return Type(TypeKind::Usize, node->pos);
    } else {
      return Type(TypeKind::Bool, node->pos);
    }
  }
  if (seg.pathIdentSegment == "Vec" || seg.pathIdentSegment == "Box") {
    if (seg.args.genericArgs.size() != 1u || seg.args.hasLifeTime) {
      error.occur(node->pos, "Invalid Type: `" + seg.pathIdentSegment +
                                 "` requires exactly one type argument "
                                 "and no lifetime arguments");
      return Type(TypeKind::Error, node->pos);
    }
    Type genericType;
    genericType.pos = node->pos;
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
    error.occur(node->pos, "Invalid Type: struct `" + seg.pathIdentSegment +
                               "` does not accept type arguments");
    return Type(TypeKind::Error, node->pos);
  }
  Type structType(TypeKind::Struct, node->pos);
  structType.structName = node->path.segments.front().pathIdentSegment;
  if (node->path.segments.front().args.hasLifeTime &&
      node->path.segments.front().pathIdentSegment == "Self") {
    error.occur(node->pos,
                "Invalid Type: `Self` does not accept lifetime arguments");
    return Type(TypeKind::Error, node->pos);
  }
  return structType;
}

std::any SymbolCollector::visitReferenceType(ReferenceTypeNode *node) {
  Type refType;
  refType.pos = node->pos;
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

std::any SymbolCollector::visitArrayType(ArrayTypeNode *node) {
  Type arrayType;
  arrayType.pos = node->pos;
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

std::any SymbolCollector::visitCrate(CrateNode *node) {
  for (std::size_t i = 0; i < node->items.size(); ++i) {
    node->items[i]->accept(this);
  }
  return {};
}
std::any SymbolCollector::visitFunctionDeclItem(FunctionDeclItemNode *node) {
  if (!BuiltInNames::checkValueName(node->identifier)) {
    error.occur(node->pos, "Invalid Function: `" + node->identifier +
                               "` is a reserved built-in name");
    return {};
  }
  if (table.valueNameSpace.contains(node->identifier)) {
    error.occur(node->pos, "Invalid Function: name `" + node->identifier +
                               "` is already defined in the value namespace");
    return {};
  }
  if (!node->args.genericArgs.empty()) {
    error.occur(node->pos, "Invalid Function: function `" + node->identifier +
                               "` cannot declare type parameters");
    return {};
  }
  if (node->identifier == "main") {
    if (node->args.hasLifeTime) {
      error.occur(
          node->pos,
          "Invalid Function: `main` cannot declare lifetime parameters");
      return {};
    }
  }
  if (node->selfParam != std::nullopt) {
    error.occur(node->pos, "Invalid Function: a top-level function "
                           "cannot have a `self` parameter");
    return {};
  }
  FunctionSymbol func(node->identifier);
  func.pos = node->pos;
  if (node->returnType != std::nullopt) {
    func.returnType =
        std::any_cast<Type>(node->returnType->get()->accept(this));
  }
  for (const auto &param : node->functionParam) {
    func.paramTypes.push_back(
        std::any_cast<Type>(param.paramType->accept(this)));
  }
  if (node->identifier == "main") {
    if (!func.paramTypes.empty()) {
      error.occur(node->pos,
                  "Invalid Function: `main` must not have parameters");
      return {};
    }
    if (func.returnType.kind != TypeKind::Unit) {
      error.occur(node->pos,
                  "Invalid Function: `main` must return the unit type `()`");
      return {};
    }
  }
  table.valueNameSpace.emplace(
      node->identifier, std::make_unique<FunctionSymbol>(std::move(func)));
  return {};
}

std::any SymbolCollector::visitStructDeclItem(StructDeclItemNode *node) {
  if (!BuiltInNames::checkTypeName(node->identifier)) {
    error.occur(node->pos, "Invalid Struct: `" + node->identifier +
                               "` is a reserved built-in type name");
    return {};
  }
  if (table.typeNameSpace.contains(node->identifier)) {
    error.occur(node->pos, "Invalid Struct: name `" + node->identifier +
                               "` is already defined in the type namespace");
    return {};
  }
  StructSymbol structSymbol(node->identifier);
  structSymbol.pos = node->pos;
  bool errorOccur = false;
  for (const auto &attributes : node->outerAttribute) {
    auto checkAttribute = [&](std::size_t pos) {
      if (structSymbol.outAttribute.test(pos) && !errorOccur) {
        errorOccur = true;
        error.occur(attributes->pos, "Invalid Attribute: the same derive trait "
                                     "cannot be listed more than once");
      }
      structSymbol.outAttribute.set(pos);
      structSymbol.traitValid.set(pos);
    };
    if (!errorOccur) {
      for (const auto &attibute : attributes->outerAttributes) {
        switch (attibute) {
        case Attribute::Copy: {
          checkAttribute(0);
          break;
        }
        case Attribute::Clone: {
          checkAttribute(1);
          break;
        }
        case Attribute::PartialEq: {
          checkAttribute(2);
          break;
        }
        case Attribute::Eq: {
          checkAttribute(3);
          break;
        }
        }
      }
    }
  }
  if (errorOccur)
    return {};
  std::function<std::optional<std::string>(const Type &)> checkDependency =
      [&](const Type &type) -> std::optional<std::string> {
    if (type.kind != TypeKind::Struct && type.kind != TypeKind::Array) {
      return std::nullopt;
    }
    if (type.kind == TypeKind::Struct) {
      return type.structName;
    } else {
      return checkDependency(*type.elementType);
    }
  };
  for (const auto &field : node->structFields) {
    if (structSymbol.structFields.contains(field.identifier)) {
      error.occur(node->pos, "Invalid Struct: field `" + field.identifier +
                                 "` is declared more than once in struct `" +
                                 node->identifier + "`");
      return {};
    }
    auto fieldType = std::any_cast<Type>(field.fieldType->accept(this));
    auto dependency = checkDependency(fieldType);
    if (dependency) {
      auto dependencyStructName = dependency.value();
      if (dependencyStructName == node->identifier ||
          dependencyStructName == "Self") {
        error.occur(node->pos, "Invalid Struct: field `" + field.identifier +
                                   "` contains struct `" + node->identifier +
                                   "` by value, creating a recursive type "
                                   "with no finite size");
        return {};
      }
    }
    structSymbol.structFields.emplace(field.identifier, fieldType);
  }
  table.typeNameSpace.emplace(node->identifier, std::make_unique<StructSymbol>(
                                                    std::move(structSymbol)));
  return {};
}

std::any SymbolCollector::visitConstantDeclItem(ConstantDeclItemNode *node) {
  if (!BuiltInNames::checkValueName(node->identifier)) {
    error.occur(node->pos, "Invalid Constant: `" + node->identifier +
                               "` is a reserved built-in name");
    return {};
  }
  if (table.valueNameSpace.contains(node->identifier)) {
    error.occur(node->pos, "Invalid Constant: name `" + node->identifier +
                               "` is already defined in the value namespace");
    return {};
  }
  ConstantSymbol constant(node->identifier,
                          std::any_cast<Type>(node->constType->accept(this)));
  constant.pos = node->pos;
  constant.node = node;
  table.valueNameSpace.emplace(
      node->identifier, std::make_unique<ConstantSymbol>(std::move(constant)));
  return {};
}

std::any SymbolCollector::visitImplDeclItem(ImplDeclItemNode *node) {
  ImplSymbol impl;
  impl.pos = node->pos;
  const auto &implType = std::any_cast<Type>(node->implType->accept(this));
  if (implType.structName == std::nullopt) {
    error.occur(node->implType->pos,
                "Invalid Impl: an implementation target must be a struct type");
    return {};
  }
  impl.name = implType.structName.value();
  for (const auto &associate : node->associatedItems) {
    if (std::holds_alternative<std::unique_ptr<ConstantDeclItemNode>>(
            associate)) {
      const auto &constantNode =
          std::get<std::unique_ptr<ConstantDeclItemNode>>(associate);
      impl.itemPos.push_back(constantNode->pos);
      ConstantSymbol constant(
          constantNode->identifier,
          std::any_cast<Type>(constantNode->constType->accept(this)));
      constant.pos = constantNode->pos;
      constant.node = constantNode.get();
      impl.associateSymbols.emplace_back(
          std::make_unique<ConstantSymbol>(std::move(constant)));
    } else if (std::holds_alternative<std::unique_ptr<FunctionDeclItemNode>>(
                   associate)) {
      const auto &functionNode =
          std::get<std::unique_ptr<FunctionDeclItemNode>>(associate);
      impl.itemPos.push_back(functionNode->pos);
      if (!functionNode->args.genericArgs.empty()) {
        error.occur(functionNode->pos, "Invalid Associated Item: function `" +
                                           functionNode->identifier +
                                           "` cannot declare type parameters");
        return {};
      }
      FunctionSymbol func(functionNode->identifier);
      func.pos = functionNode->pos;
      if (functionNode->returnType != std::nullopt) {
        func.returnType =
            std::any_cast<Type>(functionNode->returnType->get()->accept(this));
      }
      if (functionNode->selfParam) {
        const auto &self = *functionNode->selfParam;
        Type selfType(TypeKind::Struct, functionNode->pos);
        selfType.structName = "Self";
        func.receiver = ReceiverKind::Value;
        if (self.isRef) {
          Type refType(self.isMut ? TypeKind::MutReference
                                  : TypeKind::ImmutReference,
                       functionNode->pos);
          func.receiver =
              self.isMut ? ReceiverKind::Mutable : ReceiverKind::Shared;
          refType.elementType = std::make_unique<Type>(std::move(selfType));
          selfType = std::move(refType);
        }
        func.paramTypes.push_back(std::move(selfType));
      }
      for (const auto &param : functionNode->functionParam) {
        auto paramType = std::any_cast<Type>(param.paramType->accept(this));
        func.paramTypes.push_back(std::move(paramType));
      }
      impl.associateSymbols.emplace_back(
          std::make_unique<FunctionSymbol>(std::move(func)));
    }
  }
  table.implementations.emplace_back(impl.name, std::move(impl));
  return {};
}
