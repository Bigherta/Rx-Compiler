#pragma once

#include "AST.hpp"
#include "ASTBaseVisitor.hpp"
#include "Scope.hpp"
#include "SymbolCollector.hpp"
#include <memory>
#include <variant>

struct ExprInfo {
  Type type;
  bool isPlace;
  bool isMutablePlace;
};

class SemanticAnalyzer : public ASTBaseVisitor {
private:
  Type parseIntegerLiteral(const position &pos, std::string literal,
                           DiagnosticError *err, bool isNegative,
                           const Type *expected = nullptr);
  long long parseIntegerLiteral(std::string literal);
  void analyzeConstantDecl(ConstantSymbol *constant);
  void analyzeConstantDecl(ConstantSymbol *constant, StructSymbol *owner);
  auto analyzeConstExpr(ConstExprNode *node, const Type *expected)
      -> std::pair<Type, ConstantSymbol *>;
  auto analyzeMagnitudeExpr(MagnitudeExprNode *node)
      -> std::pair<Type, ConstantSymbol *>;
  void evaluate();
  auto evaluateConstantSymbol(ConstantSymbol *)
      -> const std::pair<Type, std::variant<std::monostate, long long, bool>>;
  implementationScope *findImplementationScope(Scope *scope);
  void resolveArrayLengths(Type &type);

public:
  DiagnosticError error;
  std::unique_ptr<Scope> currentScope;
  globalScope *globalScopeVisitor;
  explicit SemanticAnalyzer(SymbolCollector &&collector) {
    error = collector.error;
    currentScope = std::make_unique<globalScope>(std::move(collector), &error);
    globalScopeVisitor = static_cast<globalScope *>(currentScope.get());
  }

  void analyzeConstant();

  void preprocessArrayLengths();

  // Crate and attribute.
  virtual std::any visitCrate(CrateNode *node);

  // Types.
  virtual std::any visitUnitType(UnitTypeNode *node);

  virtual std::any visitPathInType(PathInTypeNode *node);

  virtual std::any visitReferenceType(ReferenceTypeNode *node);

  virtual std::any visitArrayType(ArrayTypeNode *node);

  // Items.
  virtual std::any visitFunctionDeclItem(FunctionDeclItemNode *node);

  virtual std::any visitConstantDeclItem(ConstantDeclItemNode *node);

  virtual std::any visitImplDeclItem(ImplDeclItemNode *node);

  // Statements.
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

  virtual std::any visitMagnitudeExpr(MagnitudeExprNode *node);

  // Array lengths return std::pair<Type, long long>; Error makes the value
  // invalid.
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
