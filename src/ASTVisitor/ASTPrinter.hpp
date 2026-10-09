#pragma once

#include "ASTVisitor.hpp"

class ASTPrinter : public ASTVisitor {
private:
  std::string prefix = "";

public:
  // Crate and attribute.
  virtual std::any visitCrate(CrateNode *node) override;

  virtual std::any visitOuterAttribute(OuterAttributeNode *node) override;

  // Types.
  virtual std::any visitUnitType(UnitTypeNode *node) override;

  virtual std::any visitPathInType(PathInTypeNode *node) override;

  virtual std::any visitReferenceType(ReferenceTypeNode *node) override;

  virtual std::any visitArrayType(ArrayTypeNode *node) override;

  // Items.
  virtual std::any visitFunctionDeclItem(FunctionDeclItemNode *node) override;

  virtual std::any visitStructDeclItem(StructDeclItemNode *node) override;

  virtual std::any visitConstantDeclItem(ConstantDeclItemNode *node) override;

  virtual std::any visitImplDeclItem(ImplDeclItemNode *node) override;

  // Statements.
  virtual std::any visitEmptyStmt(EmptyStmtNode *node) override;

  virtual std::any visitLetStmt(LetStmtNode *node) override;

  virtual std::any visitExpressionStmt(ExpressionStmtNode *node) override;

  // Expressions without blocks.
  virtual std::any visitUnitExpr(UnitExprNode *node) override;

  virtual std::any visitPathInExpr(PathInExprNode *node) override;

  virtual std::any visitLiteralExpr(LiteralExprNode *node) override;

  virtual std::any visitUnaryExpr(UnaryExprNode *node) override;

  virtual std::any visitBinaryExpr(BinaryExprNode *node) override;

  virtual std::any visitComparisonExpr(ComparisonExprNode *node) override;

  virtual std::any visitLogicExpr(LogicExprNode *node) override;

  virtual std::any visitCastExpr(CastExprNode *node) override;

  virtual std::any visitBorrowExpr(BorrowExprNode *node) override;

  virtual std::any visitDereferenceExpr(DereferenceExprNode *node) override;

  virtual std::any visitAssignmentExpr(AssignmentExprNode *node) override;

  virtual std::any visitMagnitudeExpr(MagnitudeExprNode *node) override;

  virtual std::any visitConstExpr(ConstExprNode *node) override;

  virtual std::any visitArrayExpr(ArrayExprNode *node) override;

  virtual std::any visitIndexExpr(IndexExprNode *node) override;

  virtual std::any visitStructExpr(StructExprNode *node) override;

  virtual std::any visitCallExpr(CallExprNode *node) override;

  virtual std::any visitMethodCallExpr(MethodCallExprNode *node) override;

  virtual std::any visitReturnExpr(ReturnExprNode *node) override;

  virtual std::any visitAccessExpr(AccessExprNode *node) override;

  virtual std::any visitContinueExpr(ContinueExprNode *node) override;

  virtual std::any visitBreakExpr(BreakExprNode *node) override;

  // Expressions with blocks.
  virtual std::any visitBlockExpr(BlockExprNode *node) override;

  virtual std::any visitIfExpr(IfExprNode *node) override;

  virtual std::any visitInfiniteLoopExpr(InfiniteLoopExprNode *node) override;

  virtual std::any visitPredicateLoopExpr(PredicateLoopExprNode *node) override;
};
