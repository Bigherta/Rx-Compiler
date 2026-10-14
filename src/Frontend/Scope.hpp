#pragma once

#include "AST.hpp"
#include "SymbolCollector.hpp"
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>

struct Variable {
  Type type;
  bool isMut;
  position pos;
};

class Scope {
private:
  // Symbols and their types in the current scope.
  std::unordered_map<std::string, LocalSymbol> members;

public:
  Scope *parentScope;

  explicit Scope(Scope *parentScope = nullptr) : parentScope(parentScope) {}
  virtual ~Scope() = default;

  void define(const std::string &name, const LocalSymbol &symbol) {
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
};

class globalScope : public Scope {
private:
  std::unordered_map<std::string, std::unique_ptr<GlobalSymbol>> valueNameSpace;

  std::unordered_map<std::string, std::unique_ptr<GlobalSymbol>> typeNameSpace;

public:
  explicit globalScope() = delete;
  globalScope(SymbolCollector &&collector, DiagnosticError *error)
      : valueNameSpace(std::move(collector.table.valueNameSpace)),
        typeNameSpace(std::move(collector.table.typeNameSpace)) {
    for (auto &[implName, impl] : collector.table.implementations) {
      if (!typeNameSpace.contains(implName)) {
        error->occur(impl.pos, "Invalid Impl: object not found");
      } else {
        auto &object =
            dynamic_cast<StructSymbol &>(*typeNameSpace.at(implName));
        for (std::size_t i = 0; i < impl.associateSymbols.size(); ++i) {
          auto &symbol = impl.associateSymbols[i];
          if (symbol->type == symbolType::Function) {
            const auto &func = dynamic_cast<const FunctionSymbol &>(*symbol);
            if (object.structSymbols.contains(func.name)) {
              error->occur(
                  impl.itemPos[i],
                  "Invalid Associate Item: the function name has existed");
            } else {
              object.structSymbols.emplace(func.name, std::move(symbol));
            }
          } else {
            const auto &constant =
                dynamic_cast<const ConstantSymbol &>(*symbol);
            if (object.structSymbols.contains(constant.name)) {
              error->occur(
                  impl.itemPos[i],
                  "Invalid Associate Item: the constant name has existed");
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
    if (typeNameSpace.contains(name)) {
      return dynamic_cast<StructSymbol *>(valueNameSpace[name].get());
    }
    return nullptr;
  }
};
