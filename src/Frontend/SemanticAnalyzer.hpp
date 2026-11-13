#pragma once

#include "AST.hpp"
#include "ASTBaseVisitor.hpp"
#include "Scope.hpp"
#include "SemanticResult.hpp"
#include "SymbolCollector.hpp"
#include <cstddef>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

struct ExprInfo {
  Type type;
  bool isPlace = false;
  bool isMutablePlace = false;
  bool isDiverge = false;
  // A shared dereference or immutable Vec index prevents later &mut
  // dereferences from restoring mutable access along this place path.
  bool mutationBlocked = false;
  ExprInfo() = default;
  ExprInfo(const Type &type, bool isDiverge, bool isPlace = false,
           bool isMutablePlace = false, bool mutationBlocked = false)
      : type(type), isPlace(isPlace),
        isMutablePlace(isMutablePlace && !mutationBlocked),
        isDiverge(isDiverge), mutationBlocked(mutationBlocked) {}
};

struct StmtInfo {
  bool isDiverge = false;
};

enum class LoopKind {
  Loop,
  While,
};

struct LoopContext {
  LoopKind kind;
  Type *expected = nullptr;
  std::vector<ExprInfo> breakInfos;
  std::vector<BreakExprNode *> breakNodes;
  bool checkingCondition = false;
  LoopContext(LoopKind kind) : kind(kind) {}
};

class LoopGuard {
private:
  std::deque<LoopContext> &owner;

public:
  LoopGuard() = delete;
  LoopGuard(std::deque<LoopContext> &loopStack, LoopContext ctx)
      : owner(loopStack) {
    owner.emplace_back(ctx);
  }
  ~LoopGuard() { owner.pop_back(); }
};

struct FunctionContext {
  Type *expected = nullptr;
};

class FunctionGuard {
private:
  std::optional<FunctionContext> &owner;

public:
  FunctionGuard() = delete;
  FunctionGuard(std::optional<FunctionContext> &functionInfoContainer,
                FunctionContext ctx)
      : owner(functionInfoContainer) {
    functionInfoContainer.emplace(ctx);
  }
  ~FunctionGuard() { owner.reset(); }
};

struct ReceiverContext {
  std::size_t dereferenceTimes;
  bool isMutable;
  bool isBlocked;
  ReceiverKind kind;
};

class SemanticAnalyzer : public ASTBaseVisitor {
  friend class IRGenerator;

private:
  std::deque<LoopContext> loopStack;
  std::optional<FunctionContext> functionInfoContainer;
  void analyzeStructDependency(StructSymbol &symbol);
  Type parseIntegerLiteral(const position &pos, std::string literal,
                           bool isNegative, Type *expected = nullptr);
  long long parseIntegerLiteral(std::string literal);

  void analyzeConstantDecl(ConstantSymbol *constant);
  void analyzeConstantDecl(ConstantSymbol *constant, StructSymbol *owner);
  auto analyzeConstExpr(ConstExprNode *node, Type *expected)
      -> std::pair<Type, ConstantSymbol *>;
  auto analyzeMagnitudeExpr(MagnitudeExprNode *node)
      -> std::pair<Type, ConstantSymbol *>;
  auto analyzeExpr(ExprNode *node, Type *expected = nullptr) -> ExprInfo;
  auto analyzeLiteralExpr(LiteralExprNode *node, Type *expected,
                          bool isNegative) -> ExprInfo;
  auto analyzeBlockExpr(BlockExprNode *node,
                        Type *expected)
      -> ExprInfo; // construct the new scope before analyze
  auto analyzeIfExpr(IfExprNode *node, Type *expected) -> ExprInfo;
  auto analyzeLoopExpr(InfiniteLoopExprNode *node, Type *expected) -> ExprInfo;
  auto analyzeArrayExpr(ArrayExprNode *node, Type *expected) -> ExprInfo;
  void evaluate();
  auto evaluateConstantSymbol(ConstantSymbol *)
      -> const std::pair<Type, std::variant<std::monostate, long long, bool>>;
  implementationScope *findImplementationScope(Scope *scope);
  void resolveArrayLengths(Type &type);
  std::optional<std::string> getSelfName();
  static bool isIntegerType(const Type &type) {
    return type.kind >= TypeKind::I32 && type.kind <= TypeKind::Usize;
  }

  bool checkCoercionEnability(const Type &from, const Type &to);
  bool checkCoercionEnability(const Type &from, const Type &to,
                              std::vector<Adjustment> &);
  bool checkCoercionPermission(const ExprInfo &from, const Type &to);
  Type LeastUpperBoundCoercion(const std::vector<Type> &);
  void typeResolveForFunction(FunctionSymbol *,
                              const std::string *structName = nullptr);
  void typeResolveForStruct(StructSymbol *);
  void resolveType(Type &type, std::optional<std::string> structName);
  ExprInfo checkExpr(ExprNode *node, Type *expected);
  bool hasTrait(const Type &type, BuiltinTrait trait);
  FunctionSymbol *getOrCreateBuiltinClone(const Type &type);
  FunctionSymbol *
  getOrCreateBuiltinContainerFunction(const Type &containerType,
                                      const std::string &methodName);
  FunctionSymbol *
  getOrCreateBuiltinArrayFunction(const Type &ArrayType,
                                  const std::string &methodName);
  static std::string encodeType(const Type &type);
  Type autoDereference(const Type *refType, bool &mutablePlace,
                       bool &mutationBlocked, std::vector<Adjustment> &adjusts);
  std::vector<ReceiverContext>
  generatePossibleQueue(const Type *calleeType, bool isMutable, bool isBlocked);
  Type saveType(TypeNode *node, Type &&type);

public:
  DiagnosticError error;
  std::unique_ptr<Scope> currentScope;
  globalScope *globalScopeVisitor;
  std::unordered_map<std::string, std::unique_ptr<FunctionSymbol>>
      builtinFunctions;
  std::vector<std::unique_ptr<LocalSymbol>> localBindings;
  explicit SemanticAnalyzer(SymbolCollector &&collector) {
    error = collector.error;
    currentScope = std::make_unique<globalScope>(std::move(collector), &error);
    globalScopeVisitor = static_cast<globalScope *>(currentScope.get());
    builtinFunctions.emplace(
        "get_i32", std::make_unique<FunctionSymbol>(
                       "get_i32", Type{TypeKind::I32}, std::vector<Type>{}));
    builtinFunctions.emplace("print_i32",
                             std::make_unique<FunctionSymbol>(
                                 "print_i32", Type{TypeKind::Unit},
                                 std::vector<Type>{Type{TypeKind::I32}}));
    builtinFunctions.emplace("println_i32",
                             std::make_unique<FunctionSymbol>(
                                 "println_i32", Type{TypeKind::Unit},
                                 std::vector<Type>{Type{TypeKind::I32}}));
  }

  void checkConstant();

  void preprocessArrayLengths();

  void checkStruct();

  void checkItemType();

  // Crate and attribute.
  virtual std::any visitCrate(CrateNode *node);

  // Types.
  virtual std::any visitUnitType(UnitTypeNode *node);

  virtual std::any visitPathInType(PathInTypeNode *node);

  virtual std::any visitReferenceType(ReferenceTypeNode *node);

  virtual std::any visitArrayType(ArrayTypeNode *node);

  // Items.
  virtual std::any visitFunctionDeclItem(FunctionDeclItemNode *node);

  virtual std::any visitImplDeclItem(ImplDeclItemNode *node);

  // Statements.
  virtual std::any visitEmptyStmt(EmptyStmtNode *node);

  virtual std::any visitLetStmt(LetStmtNode *node);

  virtual std::any visitExpressionStmt(ExpressionStmtNode *node);

  // Expressions without blocks.
  virtual std::any visitUnitExpr(UnitExprNode *node);

  virtual std::any visitPathInExpr(PathInExprNode *node);

  virtual std::any visitLiteralExpr(LiteralExprNode *node);

  virtual std::any visitUnaryExpr(UnaryExprNode *node);

  virtual std::any visitBinaryExpr(BinaryExprNode *node);

  virtual std::any visitComparisonExpr(ComparisonExprNode *node);

  virtual std::any visitLogicExpr(LogicExprNode *node);

  virtual std::any visitCastExpr(CastExprNode *node);

  virtual std::any visitBorrowExpr(BorrowExprNode *node);

  virtual std::any visitDereferenceExpr(DereferenceExprNode *node);

  virtual std::any visitAssignmentExpr(AssignmentExprNode *node);

  virtual std::any visitConstExpr(ConstExprNode *node);

  virtual std::any visitArrayExpr(ArrayExprNode *node);

  virtual std::any visitIndexExpr(IndexExprNode *node);

  virtual std::any visitStructExpr(StructExprNode *node);

  virtual std::any visitCallExpr(CallExprNode *node);

  virtual std::any visitMethodCallExpr(MethodCallExprNode *node);

  virtual std::any visitReturnExpr(ReturnExprNode *node);

  virtual std::any visitAccessExpr(AccessExprNode *node);

  virtual std::any visitContinueExpr(ContinueExprNode *node);

  virtual std::any visitBreakExpr(BreakExprNode *node);

  // Expressions with blocks.
  virtual std::any visitBlockExpr(BlockExprNode *node);

  virtual std::any visitIfExpr(IfExprNode *node);

  virtual std::any visitInfiniteLoopExpr(InfiniteLoopExprNode *node);

  virtual std::any visitPredicateLoopExpr(PredicateLoopExprNode *node);
};
