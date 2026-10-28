#pragma once

#include "SymbolCollector.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

class Scope {
private:
  // Symbols and their types in the current scope.
  std::unordered_map<std::string, LocalSymbol> members;

public:
  Scope *parentScope;

  explicit Scope(Scope *parentScope = nullptr) : parentScope(parentScope) {}
  virtual ~Scope() = default;

  void define(const std::string &name, const LocalSymbol &symbol) {
    if (members.contains(name)) {
      members.erase(name);
    }
    members.emplace(name, symbol);
  }

  const LocalSymbol *lookup(const std::string &name) const {
    auto it = members.find(name);
    if (it != members.end()) {
      return &it->second;
    }
    return parentScope ? parentScope->lookup(name) : nullptr;
  }

  bool existenceCheck(const std::string &name) const {
    return members.contains(name);
  }
};

class implementationScope : public Scope {
private:
  std::unordered_map<std::string, std::unique_ptr<GlobalSymbol>>
      &valueNameSpace;

  const std::unordered_map<std::string, std::unique_ptr<GlobalSymbol>>
      &typeNameSpace;

public:
  std::string structName;

  explicit implementationScope() = delete;

  implementationScope(
      std::unordered_map<std::string, std::unique_ptr<GlobalSymbol>>
          &valueNameSpace,
      const std::unordered_map<std::string, std::unique_ptr<GlobalSymbol>>
          &typeNameSpace)
      : valueNameSpace(valueNameSpace), typeNameSpace(typeNameSpace) {}
  ConstantSymbol *lookupConstant(const std::string &name) {
    if (valueNameSpace.contains(name)) {
      return dynamic_cast<ConstantSymbol *>(valueNameSpace[name].get());
    }
    return nullptr;
  }
  FunctionSymbol *lookupFunction(const std::string &name) {
    if (valueNameSpace.contains(name)) {
      return dynamic_cast<FunctionSymbol *>(valueNameSpace[name].get());
    }
    return nullptr;
  }
};

class globalScope : public Scope {
public:
  std::unordered_map<std::string, std::unique_ptr<GlobalSymbol>> valueNameSpace;

  std::unordered_map<std::string, std::unique_ptr<GlobalSymbol>> typeNameSpace;

  explicit globalScope() = delete;
  globalScope(SymbolCollector &&collector, DiagnosticError *error)
      : valueNameSpace(std::move(collector.table.valueNameSpace)),
        typeNameSpace(std::move(collector.table.typeNameSpace)) {
    for (auto &[implName, impl] : collector.table.implementations) {
      auto typeIterator = typeNameSpace.find(implName);
      if (typeIterator == typeNameSpace.end()) {
        error->occur(impl.pos,
                     "Invalid Impl: cannot implement undefined struct `" +
                         implName + "`");
      } else {
        auto &object = dynamic_cast<StructSymbol &>(*typeIterator->second);
        for (std::size_t i = 0; i < impl.associateSymbols.size(); ++i) {
          auto &symbol = impl.associateSymbols[i];
          if (symbol->type == symbolType::Function) {
            const auto &func = dynamic_cast<const FunctionSymbol &>(*symbol);
            if (object.structSymbols.contains(func.name)) {
              error->occur(
                  impl.itemPos[i],
                  "Invalid Associated Item: cannot define function `" +
                      func.name + "`; name `" + func.name +
                      "` is already defined in struct `" + implName + "`");
            } else {
              object.structSymbols.emplace(func.name, std::move(symbol));
            }
          } else {
            const auto &constant =
                dynamic_cast<const ConstantSymbol &>(*symbol);
            if (object.structSymbols.contains(constant.name)) {
              error->occur(
                  impl.itemPos[i],
                  "Invalid Associated Item: cannot define constant `" +
                      constant.name + "`; name `" + constant.name +
                      "` is already defined in struct `" + implName + "`");
            } else {
              object.structSymbols.emplace(constant.name, std::move(symbol));
            }
          }
        }
      }
    }
  }
  ConstantSymbol *lookupConstant(const std::string &name) {
    if (valueNameSpace.contains(name)) {
      return dynamic_cast<ConstantSymbol *>(valueNameSpace[name].get());
    }
    return nullptr;
  }
  FunctionSymbol *lookupFunction(const std::string &name) {
    if (valueNameSpace.contains(name)) {
      return dynamic_cast<FunctionSymbol *>(valueNameSpace[name].get());
    }
    return nullptr;
  }
  StructSymbol *lookupStruct(const std::string &name) {
    auto iterator = typeNameSpace.find(name);
    if (iterator != typeNameSpace.end()) {
      return dynamic_cast<StructSymbol *>(iterator->second.get());
    }
    return nullptr;
  }
};

class ScopeGuard {
private:
  std::unique_ptr<Scope> &owner;
  std::unique_ptr<Scope> parent;

public:
  ScopeGuard() = delete;
  ScopeGuard(std::unique_ptr<Scope> &owner) : owner(owner) {
    parent = std::move(owner);
    owner = std::make_unique<Scope>(parent.get());
  }
  ~ScopeGuard() { owner = std::move(parent); }
};
