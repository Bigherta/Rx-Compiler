#pragma once

#include "AST.hpp"
#include "ASTBaseVisitor.hpp"
#include <bitset>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

enum class TypeKind {
  I32,
  U32,
  Isize,
  Usize,
  Bool,
  Unit,
  Never,
  Error,
  MutReference,
  ImmutReference,
  Vec,
  Box,
  Array,
  Struct,
  Function,
};

enum class EvaluatedState {
  NotAccess,
  Evaluating,
  Evaluated,
};

enum class BuiltinTrait {
  Copy,
  Clone,
  PartialEq,
  Eq,
};

enum class ReceiverKind { None, Value, Shared, Mutable };

struct ConstValue {
  ConstExprNode *ctx = nullptr; // first pass store the AST context
  EvaluatedState state =
      EvaluatedState::NotAccess; // seconde pass evaluate the constant
  std::variant<std::monostate, long long, bool> value =
      std::monostate{}; // Wide enough for both signed integers and the full
                        // 32-bit usize
  // range.
  ConstValue() = default;
  ConstValue(ConstValue &&other) {
    ctx = std::move(other.ctx);
    state = other.state;
    value = other.value;
  }
  ConstValue(const ConstValue &other) {
    ctx = other.ctx;
    state = other.state;
    value = other.value;
  }
  ConstValue &operator=(const ConstValue &other) {
    if (this != &other) {
      ctx = other.ctx;
      state = other.state;
      value = other.value;
    }
    return *this;
  }
  ConstValue &operator=(ConstValue &&other) {
    if (this != &other) {
      ctx = std::move(other.ctx);
      state = other.state;
      value = other.value;
    }
    return *this;
  }
};

struct FunctionSymbol;

struct Type {
  TypeKind kind;
  position pos{}; // Source range of the type syntax; zero for synthetic types.
  std::optional<std::string> structName;
  std::optional<ConstValue> arrayLen;
  std::optional<FunctionSymbol *> functionSymbol;
  std::unique_ptr<Type>
      elementType; // for the inner type of Reference, Array, Vec or Box
  Type() = default;
  explicit Type(TypeKind kind, const position &pos = {})
      : kind(kind), pos(pos) {}
  explicit Type(const std::string &name) : structName(name) {
    kind = TypeKind::Struct;
  }
  Type(const Type &other) {
    kind = other.kind;
    pos = other.pos;
    structName = other.structName;
    arrayLen = other.arrayLen;
    functionSymbol = other.functionSymbol;
    if (other.elementType) {
      elementType = std::make_unique<Type>(*other.elementType);
    } else
      elementType = nullptr;
  }
  Type &operator=(const Type &other) {
    if (this == &other)
      return *this;
    kind = other.kind;
    pos = other.pos;
    structName = other.structName;
    arrayLen = other.arrayLen;
    functionSymbol = other.functionSymbol;
    if (other.elementType) {
      elementType = std::make_unique<Type>(*other.elementType);
    } else
      elementType = nullptr;
    return *this;
  }
  Type(Type &&) noexcept = default;
  Type &operator=(Type &&) noexcept = default;
  bool operator==(const Type &other) const {
    if (kind != other.kind) {
      return false;
    }

    switch (kind) {
    // primative types
    case TypeKind::I32:
    case TypeKind::U32:
    case TypeKind::Isize:
    case TypeKind::Usize:
    case TypeKind::Bool:
    case TypeKind::Unit:
    case TypeKind::Never:
      return true;
    // types with inner type
    case TypeKind::MutReference:
    case TypeKind::ImmutReference:
    case TypeKind::Vec:
    case TypeKind::Box:
      return elementType && other.elementType &&
             *elementType == *other.elementType;
    // array needs to compare the length
    case TypeKind::Array:
      return elementType && other.elementType &&
             *elementType == *other.elementType && arrayLen && other.arrayLen &&
             arrayLen->state == EvaluatedState::Evaluated &&
             other.arrayLen->state == EvaluatedState::Evaluated &&
             std::holds_alternative<long long>(arrayLen->value) &&
             std::holds_alternative<long long>(other.arrayLen->value) &&
             arrayLen->value == other.arrayLen->value;
    // struct needs to compare the name
    case TypeKind::Struct:
      return structName && other.structName && structName == other.structName;
    case TypeKind::Function:
      return functionSymbol && other.functionSymbol &&
             functionSymbol == other.functionSymbol;
    default:
      return false;
    }
  }
};

struct LocalSymbol {
  position varPos;
  std::string varName;
  bool isMut;
  Type varType;
  LocalSymbol(const position &pos, const std::string &name, bool isMut,
              const Type &type)
      : varPos(pos), varName(name), isMut(isMut), varType(type) {}
};

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
  position pos{};
  explicit GlobalSymbol(symbolType symbolType) : type(symbolType) {}
  virtual ~GlobalSymbol() = default;
};

struct FunctionSymbol : GlobalSymbol {
  std::string name;
  Type functionalType;
  Type returnType;
  std::vector<Type> paramTypes;
  ReceiverKind receiver = ReceiverKind::None;
  FunctionSymbol(const std::string &funcName)
      : GlobalSymbol(symbolType::Function), name(funcName),
        returnType(TypeKind::Unit) {
    functionalType.kind = TypeKind::Function;
    functionalType.functionSymbol = this;
  }
  FunctionSymbol(const std::string &funcName, const Type &returnType,
                 const std::vector<Type> &funcParams)
      : GlobalSymbol(symbolType::Function), name(funcName),
        returnType(returnType) {
    paramTypes = funcParams;
    functionalType.kind = TypeKind::Function;
    functionalType.functionSymbol = this;
  }
  FunctionSymbol(const std::string &funcName,
                 const std::vector<Type> &funcParams)
      : FunctionSymbol(funcName, Type(TypeKind::Unit), funcParams) {
    functionalType.kind = TypeKind::Function;
    functionalType.functionSymbol = this;
  }
  FunctionSymbol(FunctionSymbol &&other) noexcept
      : GlobalSymbol(other), name(std::move(other.name)),
        functionalType(std::move(other.functionalType)),
        returnType(std::move(other.returnType)),
        paramTypes(std::move(other.paramTypes)), receiver(other.receiver) {
    // The function type stores a self pointer, so a moved symbol must point at
    // its final address instead of the source object.
    functionalType.functionSymbol = this;
  }
};

struct ConstantSymbol : GlobalSymbol {
  std::string name;
  Type constantType;
  ConstantDeclItemNode *node = nullptr;
  ConstantSymbol *dependency = nullptr;
  ConstValue value;
  ConstantSymbol(const std::string &constantName, const Type &constantType)
      : GlobalSymbol(symbolType::Constant), name(constantName),
        constantType(constantType) {}
};

struct StructSymbol : GlobalSymbol {
  enum class dependencyCheckState {
    NotAccess,
    checking,
    checked,
  };
  std::bitset<4> outAttribute; // 4 bits represent the 4 attributes
  std::bitset<4> traitValid;
  std::string name;
  std::unordered_map<std::string, Type> structFields;
  std::unordered_map<std::string, std::unique_ptr<GlobalSymbol>> structSymbols;
  std::vector<StructSymbol *> dependency;
  std::vector<StructSymbol *> copyDependOnThis;
  std::vector<StructSymbol *> cloneDependOnThis;
  std::vector<StructSymbol *> partialEqDependOnThis;
  std::vector<StructSymbol *> eqDependOnThis;
  dependencyCheckState dependencyChecked =
      StructSymbol::dependencyCheckState::NotAccess;
  StructSymbol(const std::string &structName)
      : GlobalSymbol(symbolType::Struct), name(structName) {}

private:
  std::optional<std::string_view> checkTraitDepend(const Type &type,
                                                   BuiltinTrait trait) {
    switch (type.kind) {
    case TypeKind::I32:
    case TypeKind::U32:
    case TypeKind::Isize:
    case TypeKind::Usize:
    case TypeKind::Bool:
      return std::nullopt;
    case TypeKind::Struct: {
      return std::string_view(*type.structName);
    }
    case TypeKind::Array: {
      return checkTraitDepend(*type.elementType, trait);
    }
    case TypeKind::ImmutReference: {
      if (trait == BuiltinTrait::Copy || trait == BuiltinTrait::Clone) {
        return std::nullopt;
      } else {
        return checkTraitDepend(*type.elementType, trait);
      }
    }
    case TypeKind::MutReference: {
      if (trait == BuiltinTrait::Copy) {
        traitValid[0] = 0;
        return std::nullopt;
      } else if (trait == BuiltinTrait::Clone) {
        traitValid[1] = 0;
        return std::nullopt;
      } else {
        return checkTraitDepend(*type.elementType, trait);
      }
    }
    case TypeKind::Box:
    case TypeKind::Vec: {
      if (trait == BuiltinTrait::Copy) {
        traitValid[0] = 0;
        return std::nullopt;
      }
      return checkTraitDepend(*type.elementType, trait);
    }
    case TypeKind::Unit:
    case TypeKind::Never:
    case TypeKind::Error:
    case TypeKind::Function:
      traitValid.reset();
      return std::nullopt;
    }
  }

public:
  std::optional<std::string_view> checkCopyDepend(const Type &type) {
    return checkTraitDepend(type, BuiltinTrait::Copy);
  }
  std::optional<std::string_view> checkCloneDepend(const Type &type) {
    return checkTraitDepend(type, BuiltinTrait::Clone);
  }
  std::optional<std::string_view> checkPartialEqDepend(const Type &type) {
    return checkTraitDepend(type, BuiltinTrait::PartialEq);
  }
  std::optional<std::string_view> checkEqDepend(const Type &type) {
    return checkTraitDepend(type, BuiltinTrait::Eq);
  }
};

struct ImplSymbol : GlobalSymbol {
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
  bool hasError() const { return !diagnostics.empty(); }
  std::size_t size() const { return diagnostics.size(); }
  const std::map<position, std::string> &all() const { return diagnostics; }
  void report(std::ostream &out) const {
    for (const auto &[pos, text] : diagnostics) {
      out << pos.startLine << ":" << pos.startColumn << ": " << text << '\n';
    }
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
