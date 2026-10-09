#pragma once

#include "AST.hpp"
#include <any>

class ASTVisitor {
public:
  virtual ~ASTVisitor() = default;

  // Crate and attribute.
  virtual std::any visitCrate(CrateNode *node) = 0;

  virtual std::any visitOuterAttribute(OuterAttributeNode *node) = 0;

  // Types.
  virtual std::any visitUnitType(UnitTypeNode *node) = 0;

  virtual std::any visitPathInType(PathInTypeNode *node) = 0;

  virtual std::any visitReferenceType(ReferenceTypeNode *node) = 0;

  virtual std::any visitArrayType(ArrayTypeNode *node) = 0;

  // Items.
  virtual std::any visitFunctionDeclItem(FunctionDeclItemNode *node) = 0;

  virtual std::any visitStructDeclItem(StructDeclItemNode *node) = 0;

  virtual std::any visitConstantDeclItem(ConstantDeclItemNode *node) = 0;

  virtual std::any visitImplDeclItem(ImplDeclItemNode *node) = 0;

  // Statements.
  virtual std::any visitEmptyStmt(EmptyStmtNode *node) = 0;

  virtual std::any visitLetStmt(LetStmtNode *node) = 0;

  virtual std::any visitExpressionStmt(ExpressionStmtNode *node) = 0;

  // Expressions without blocks.
  virtual std::any visitUnitExpr(UnitExprNode *node) = 0;

  virtual std::any visitPathInExpr(PathInExprNode *node) = 0;

  virtual std::any visitLiteralExpr(LiteralExprNode *node) = 0;

  virtual std::any visitUnaryExpr(UnaryExprNode *node) = 0;

  virtual std::any visitBinaryExpr(BinaryExprNode *node) = 0;

  virtual std::any visitComparisonExpr(ComparisonExprNode *node) = 0;

  virtual std::any visitLogicExpr(LogicExprNode *node) = 0;

  virtual std::any visitCastExpr(CastExprNode *node) = 0;

  virtual std::any visitBorrowExpr(BorrowExprNode *node) = 0;

  virtual std::any visitDereferenceExpr(DereferenceExprNode *node) = 0;

  virtual std::any visitAssignmentExpr(AssignmentExprNode *node) = 0;

  virtual std::any visitMagnitudeExpr(MagnitudeExprNode *node) = 0;

  virtual std::any visitConstExpr(ConstExprNode *node) = 0;

  virtual std::any visitArrayExpr(ArrayExprNode *node) = 0;

  virtual std::any visitIndexExpr(IndexExprNode *node) = 0;

  virtual std::any visitStructExpr(StructExprNode *node) = 0;

  virtual std::any visitCallExpr(CallExprNode *node) = 0;

  virtual std::any visitMethodCallExpr(MethodCallExprNode *node) = 0;

  virtual std::any visitReturnExpr(ReturnExprNode *node) = 0;

  virtual std::any visitAccessExpr(AccessExprNode *node) = 0;

  virtual std::any visitContinueExpr(ContinueExprNode *node) = 0;

  virtual std::any visitBreakExpr(BreakExprNode *node) = 0;

  // Expressions with blocks.
  virtual std::any visitBlockExpr(BlockExprNode *node) = 0;

  virtual std::any visitIfExpr(IfExprNode *node) = 0;

  virtual std::any visitInfiniteLoopExpr(InfiniteLoopExprNode *node) = 0;

  virtual std::any visitPredicateLoopExpr(PredicateLoopExprNode *node) = 0;
};
