#include "SymbolCollector.hpp"
#include "AST.hpp"
#include <any>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>
std::any SymbolCollector::visitUnitType(UnitTypeNode *node) {
  return Type(TypeKind::Unit);
}

std::any SymbolCollector::visitPathInType(PathInTypeNode *node) {
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

std::any SymbolCollector::visitReferenceType(ReferenceTypeNode *node) {
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

std::any SymbolCollector::visitArrayType(ArrayTypeNode *node) {
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

std::any SymbolCollector::visitCrate(CrateNode *node) {
  for (std::size_t i = 0; i < node->items.size(); ++i) {
    node->items[i]->accept(this);
  }
  return {};
}
std::any SymbolCollector::visitFunctionDeclItem(FunctionDeclItemNode *node) {
  if (!BuiltInNames::checkValueName(node->identifier)) {
    error.occur(node->pos, "Invalid Item: invalid function name");
    return {};
  }
  if (table.valueNameSpace.contains(node->identifier)) {
    error.occur(node->pos,
                "Invalid Item: function name has existed in value namespace");
    return {};
  }
  if (!node->args.genericArgs.empty()) {
    error.occur(node->pos,
                "Invalid Item: function cannot receive type arguments");
    return {};
  }
  if (node->identifier == "main") {
    if (node->args.hasLifeTime) {
      error.occur(node->pos,
                  "Invalid Item: main function cannot receive any arguments");
      return {};
    }
  }
  if (node->selfParam != std::nullopt) {
    error.occur(node->pos,
                "Invalid Item: top functions cannot receive self param");
    return {};
  }
  FunctionSymbol func(node->identifier);
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
                  "Invalid Item: main function cannot receive any params");
      return {};
    }
    if (func.returnType.kind != TypeKind::Unit) {
      error.occur(node->pos,
                  "Invalid Item: main function must return unit type");
      return {};
    }
  }
  table.valueNameSpace.emplace(
      node->identifier, std::make_unique<FunctionSymbol>(std::move(func)));
  return {};
}

std::any SymbolCollector::visitStructDeclItem(StructDeclItemNode *node) {
  if (!BuiltInNames::checkTypeName(node->identifier)) {
    error.occur(node->pos, "Invalid Item: invalid struct name");
    return {};
  }
  if (table.typeNameSpace.contains(node->identifier)) {
    error.occur(node->pos, "Invalid Item: struct cannot be redeclared");
    return {};
  }
  StructSymbol structSymbol(node->identifier);
  bool errorOccur = false;
  for (const auto &attributes : node->outerAttribute) {
    auto checkAttribute = [&](std::size_t pos) {
      if (structSymbol.outAttribute.test(pos) && !errorOccur) {
        errorOccur = true;
        error.occur(attributes->pos,
                    "Invalid outer attribute : duplicated attribute");
      }
      structSymbol.outAttribute.set(pos);
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
  for (const auto &field : node->structFields) {
    if (structSymbol.structFields.contains(field.identifier)) {
      error.occur(node->pos, "Invalid Item: struct field cannot be redeclared");
      return {};
    }
    structSymbol.structFields.emplace(
        field.identifier, std::any_cast<Type>(field.fieldType->accept(this)));
  }
  table.typeNameSpace.emplace(
      node->identifier, std::make_unique<StructSymbol>(std::move(structSymbol)));
  return {};
}

std::any SymbolCollector::visitConstantDeclItem(ConstantDeclItemNode *node) {
  if (!BuiltInNames::checkValueName(node->identifier)) {
    error.occur(node->pos, "Invalid Item: invalid constant name");
    return {};
  }
  if (table.valueNameSpace.contains(node->identifier)) {
    error.occur(node->pos,
                "Invalid Item: constant name has existed in value namespace");
    return {};
  }
  ConstantSymbol constant(node->identifier,
                          std::any_cast<Type>(node->constType->accept(this)));
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
                "Invalid Item: implementation obnject must be a struct");
    return {};
  }
  impl.name = implType.structName.value();
  for (const auto &associate : node->associatedItems) {
    std::vector<ASTNode *> pending;
    if (std::holds_alternative<std::unique_ptr<ConstantDeclItemNode>>(
            associate)) {
      const auto &constantNode =
          std::get<std::unique_ptr<ConstantDeclItemNode>>(associate);
      pending.push_back(constantNode->constType.get());
      pending.push_back(constantNode->constValue.get());
    } else {
      const auto &functionNode =
          std::get<std::unique_ptr<FunctionDeclItemNode>>(associate);
      if (functionNode->returnType) {
        pending.push_back(functionNode->returnType->get());
      }
      for (const auto &param : functionNode->functionParam) {
        pending.push_back(param.paramType.get());
      }
    }
    while (!pending.empty()) {
      auto *currentNode = pending.back();
      pending.pop_back();
      switch (currentNode->kind) {
      case nodeKind::PathInType: {
        auto &path = static_cast<PathInTypeNode *>(currentNode)->path;
        if (!path.segments.empty() &&
            path.segments.front().pathIdentSegment == "Self") {
          path.segments.front().pathIdentSegment = impl.name;
        }
        for (auto &segment : path.segments) {
          for (const auto &arg : segment.args.genericArgs) {
            pending.push_back(arg.genericArgType.get());
          }
        }
        break;
      }
      case nodeKind::ReferenceType:
        pending.push_back(
            static_cast<ReferenceTypeNode *>(currentNode)->refType.get());
        break;
      case nodeKind::ArrayType: {
        auto *arrayNode = static_cast<ArrayTypeNode *>(currentNode);
        pending.push_back(arrayNode->typeParam.get());
        pending.push_back(arrayNode->length.get());
        break;
      }
      case nodeKind::ConstExpr: {
        auto *constNode = static_cast<ConstExprNode *>(currentNode);
        if (std::holds_alternative<std::unique_ptr<PathInExprNode>>(
                constNode->constValue)) {
          pending.push_back(std::get<std::unique_ptr<PathInExprNode>>(
                                constNode->constValue)
                                .get());
        } else if (std::holds_alternative<std::unique_ptr<MagnitudeExprNode>>(
                       constNode->constValue)) {
          pending.push_back(std::get<std::unique_ptr<MagnitudeExprNode>>(
                                constNode->constValue)
                                .get());
        }
        break;
      }
      case nodeKind::MagnitudeExpr: {
        auto *magnitudeNode = static_cast<MagnitudeExprNode *>(currentNode);
        if (std::holds_alternative<std::unique_ptr<PathInExprNode>>(
                magnitudeNode->magnitude)) {
          pending.push_back(std::get<std::unique_ptr<PathInExprNode>>(
                                magnitudeNode->magnitude)
                                .get());
        }
        break;
      }
      case nodeKind::PathInExpr: {
        auto &path = static_cast<PathInExprNode *>(currentNode)->path;
        // A bare Self is not a constant value; preserve it for diagnostics.
        if (path.segments.size() > 1 &&
            path.segments.front().pathIdentSegment == "Self") {
          path.segments.front().pathIdentSegment = impl.name;
        }
        for (auto &segment : path.segments) {
          for (const auto &arg : segment.args.genericArgs) {
            pending.push_back(arg.genericArgType.get());
          }
        }
        break;
      }
      default:
        break;
      }
    }
    if (std::holds_alternative<std::unique_ptr<ConstantDeclItemNode>>(
            associate)) {
      const auto &constantNode =
          std::get<std::unique_ptr<ConstantDeclItemNode>>(associate);
      impl.itemPos.push_back(constantNode->pos);
      ConstantSymbol constant(
          constantNode->identifier,
          std::any_cast<Type>(constantNode->constType->accept(this)));
      constant.node = constantNode.get();
      impl.associateSymbols.emplace_back(
          std::make_unique<ConstantSymbol>(std::move(constant)));
    } else if (std::holds_alternative<std::unique_ptr<FunctionDeclItemNode>>(
                   associate)) {
      const auto &functionNode =
          std::get<std::unique_ptr<FunctionDeclItemNode>>(associate);
      impl.itemPos.push_back(functionNode->pos);
      if (!functionNode->args.genericArgs.empty()) {
        error.occur(
            functionNode->pos,
            "Invalid Associate Item: function cannot receive type arguments");
        return {};
      }
      FunctionSymbol func(functionNode->identifier);
      if (functionNode->returnType != std::nullopt) {
        func.returnType =
            std::any_cast<Type>(functionNode->returnType->get()->accept(this));
      }
      if (functionNode->selfParam != std::nullopt) {
        const auto &self = functionNode->selfParam.value();
        if (self.isRef) {
          Type refSelfType;
          refSelfType.kind =
              self.isMut ? TypeKind::MutReference : TypeKind::ImmutReference;
          refSelfType.elementType = std::make_unique<Type>(impl.name);
          func.paramTypes.push_back(refSelfType);
        } else {
          func.paramTypes.push_back(Type(impl.name));
        }
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
