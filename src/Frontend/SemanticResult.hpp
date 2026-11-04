#pragma once

#include "AST.hpp"
#include "SymbolCollector.hpp"
#include <cstddef>
#include <limits>
#include <variant>
#include <vector>
struct TypeSemantic {
  Type semanticType{TypeKind::Error};
};

enum class AdjustmentKind {
  Deref,
  Borrow,
  MutToShared,
  NeverToAny,
};

struct Adjustment {
  AdjustmentKind kind;
  Type targetType{TypeKind::Error};
};

struct ExprSemantic {
  virtual ~ExprSemantic() = default;
  // Contextually inferred type, before the enclosing checkExpr coercion.
  Type originalType{TypeKind::Error};
  Type adjustedType{TypeKind::Error};
  std::vector<Adjustment> adjustments;

  bool isPlace = false;
  bool isMutablePlace = false;
  bool isDiverge = false;
};

struct ConstExprSemantic : ExprSemantic {
  long long value;
  ConstExprSemantic(long long length) : value(length) {
    originalType = Type(TypeKind::Usize);
    adjustedType = originalType;
  }
};

struct FunctionPathInExprSemantic : public ExprSemantic {
  const FunctionSymbol *functionSymbol = nullptr;
};

struct StructPathInExprSemantic : public ExprSemantic {
  const StructSymbol *structSymbol = nullptr;
};
struct LiteralExprSemantic : ExprSemantic {
  std::variant<long long, bool> value;
  LiteralExprSemantic(long long interger, const Type &type) : value(interger) {
    originalType = type;
    adjustedType = originalType;
  }
  LiteralExprSemantic(bool boolean) : value(boolean) {
    originalType = Type(TypeKind::Bool);
    adjustedType = originalType;
  }
};

struct ValuePathInExprSemantic : public ExprSemantic {
  std::variant<const LocalSymbol *, const ConstantSymbol *> bindingSymbol;
};

struct CallExprSemantic : public ExprSemantic {
  FunctionSymbol *calleeSymbol = nullptr;
};

struct MethodCallExprSemantic : public ExprSemantic {
  FunctionSymbol *calleeSymbol = nullptr;
  std::vector<Adjustment> receiverAdjustments;
};

struct AccessExprSemantic : public ExprSemantic {
  StructSymbol *object = nullptr;
  std::size_t index = std::numeric_limits<std::size_t>::max();
};

struct StmtSemantic {
  bool isLet = false;
  bool isDiverge = false;
  LocalSymbol *bindingSymbol = nullptr;
  Type statementType{TypeKind::Error};
};
