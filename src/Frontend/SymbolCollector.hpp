#pragma once

#include "ASTBaseVisitor.hpp"
#include <bitset>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
class BuiltInNames {
private:
  static inline std::unordered_set<std::string> typeBuiltInNames = {
      "i32", "u32",  "isize", "usize",     "bool", "Box",
      "Vec", "Copy", "Clone", "PartialEq", "Eq"};
  static inline std::unordered_set<std::string> valueBuiltInNames = {
      "get_i32", "print_i32", "println_i32"};
  static inline std::unordered_set<std::string> deriveBuiltInNames = {
      "Copy", "Clone", "PartialEq", "Eq"};

public:
  static bool checkTypeName(const std::string &type) {
    return !typeBuiltInNames.contains(type);
  } // confirm that the type name is not in the builtin protected names
  static bool checkValueName(const std::string &value) {
    return !valueBuiltInNames.contains(value);
  } // confirm that the value name is not in the builtin protected names
  static bool isValidDerive(const std::string &derive) {
    return deriveBuiltInNames.contains(derive);
  } // confirm that the derive name is valid
};

enum class symbolType {
  Function,
  Struct,
  Constant,
  Implementation,
};

struct GlobalSymbol {
  symbolType type;
  explicit GlobalSymbol(symbolType symbolType) : type(symbolType) {}
  virtual ~GlobalSymbol() = default;
};

struct FunctionSymbol : GlobalSymbol {
  std::string name;
  Type returnType;
  std::vector<Type> paramTypes;
  FunctionSymbol(const std::string &funcName)
      : GlobalSymbol(symbolType::Function), name(funcName),
        returnType(TypeKind::Unit) {}
  FunctionSymbol(const std::string &funcName, const Type &returnType,
                 const std::vector<Type> &funcParams)
      : GlobalSymbol(symbolType::Function), name(funcName),
        returnType(returnType) {
    paramTypes = funcParams;
  }
  FunctionSymbol(const std::string &funcName,
                 const std::vector<Type> &funcParams)
      : FunctionSymbol(funcName, Type(TypeKind::Unit), funcParams) {}
};

struct ConstantSymbol : GlobalSymbol {
  std::string name;
  Type constantType;
  ConstValue constantValue;
  ConstantSymbol* dependency;
  ConstantSymbol(const std::string &constantName, const Type &constantType)
      : GlobalSymbol(symbolType::Constant), name(constantName),
        constantType(constantType) {}
};

struct StructSymbol : GlobalSymbol {
  std::bitset<4> outAttribute; // 4 bits represent the 4 attributes
  std::string name;
  std::unordered_map<std::string, Type> structFields;
  std::unordered_map<std::string, std::unique_ptr<GlobalSymbol>> structSymbols;
  StructSymbol(const std::string &structName)
      : GlobalSymbol(symbolType::Struct), name(structName) {}
};

struct ImplSymbol : GlobalSymbol {
  position pos;
  std::vector<position> itemPos;
  std::string name;
  std::vector<std::unique_ptr<GlobalSymbol>> associateSymbols;
  ImplSymbol() : GlobalSymbol(symbolType::Implementation) {}
};

class SymbolTable {
public:
  std::unordered_map<std::string, std::unique_ptr<GlobalSymbol>> valueNameSpace;

  std::unordered_map<std::string, std::unique_ptr<GlobalSymbol>> typeNameSpace;

  std::vector<std::pair<std::string, ImplSymbol>> implementations;
  SymbolTable() {
    valueNameSpace.emplace(
        "get_i32", std::make_unique<FunctionSymbol>(
                       "get_i32", Type{TypeKind::I32}, std::vector<Type>{}));
    valueNameSpace.emplace("print_i32",
                           std::make_unique<FunctionSymbol>(
                               "print_i32", Type{TypeKind::Unit},
                               std::vector<Type>{Type{TypeKind::I32}}));
    valueNameSpace.emplace("println_i32",
                           std::make_unique<FunctionSymbol>(
                               "println_i32", Type{TypeKind::Unit},
                               std::vector<Type>{Type{TypeKind::I32}}));
  }
};

class DiagnosticError {
private:
  std::map<position, std::string> diagnostics;

public:
  void occur(const position &pos, const std::string &text) {
    diagnostics.emplace(pos, text);
  }
};

class SymbolCollector : public ASTBaseVisitor {
public:
  SymbolTable table;
  DiagnosticError error;
  // Crate.
  virtual std::any visitCrate(CrateNode *node) override;
  // Items.
  virtual std::any visitFunctionDeclItem(FunctionDeclItemNode *node) override;

  virtual std::any visitStructDeclItem(StructDeclItemNode *node) override;

  virtual std::any visitConstantDeclItem(ConstantDeclItemNode *node) override;

  virtual std::any visitImplDeclItem(ImplDeclItemNode *node) override;

  // Types.
  virtual std::any visitUnitType(UnitTypeNode *node) override;

  virtual std::any visitPathInType(PathInTypeNode *node) override;

  virtual std::any visitReferenceType(ReferenceTypeNode *node) override;

  virtual std::any visitArrayType(ArrayTypeNode *node) override;
};
