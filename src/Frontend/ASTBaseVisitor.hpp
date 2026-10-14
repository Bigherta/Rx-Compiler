#pragma once

#include "ASTVisitor.hpp"

// Default visits do nothing and do not traverse children. Selective passes
// override only the nodes they need; ASTVisitor remains the strict interface.
class ASTBaseVisitor : public ASTVisitor {
public:
  // Crate and attribute.
  std::any visitCrate(CrateNode *) override { return {}; }

  std::any visitOuterAttribute(OuterAttributeNode *) override { return {}; }

  // Types.
  std::any visitUnitType(UnitTypeNode *) override { return {}; }

  std::any visitPathInType(PathInTypeNode *) override { return {}; }

  std::any visitReferenceType(ReferenceTypeNode *) override { return {}; }

  std::any visitArrayType(ArrayTypeNode *) override { return {}; }

  // Items.
  std::any visitFunctionDeclItem(FunctionDeclItemNode *) override { return {}; }

  std::any visitStructDeclItem(StructDeclItemNode *) override { return {}; }

  std::any visitConstantDeclItem(ConstantDeclItemNode *) override { return {}; }

  std::any visitImplDeclItem(ImplDeclItemNode *) override { return {}; }

  // Statements.
  std::any visitEmptyStmt(EmptyStmtNode *) override { return {}; }

  std::any visitLetStmt(LetStmtNode *) override { return {}; }

  std::any visitExpressionStmt(ExpressionStmtNode *) override { return {}; }

  // Expressions without blocks.
  std::any visitUnitExpr(UnitExprNode *) override { return {}; }

  std::any visitPathInExpr(PathInExprNode *) override { return {}; }

  std::any visitLiteralExpr(LiteralExprNode *) override { return {}; }

  std::any visitUnaryExpr(UnaryExprNode *) override { return {}; }

  std::any visitBinaryExpr(BinaryExprNode *) override { return {}; }

  std::any visitComparisonExpr(ComparisonExprNode *) override { return {}; }

  std::any visitLogicExpr(LogicExprNode *) override { return {}; }

  std::any visitCastExpr(CastExprNode *) override { return {}; }

  std::any visitBorrowExpr(BorrowExprNode *) override { return {}; }

  std::any visitDereferenceExpr(DereferenceExprNode *) override { return {}; }

  std::any visitAssignmentExpr(AssignmentExprNode *) override { return {}; }

  std::any visitMagnitudeExpr(MagnitudeExprNode *) override { return {}; }

  std::any visitConstExpr(ConstExprNode *) override { return {}; }

  std::any visitArrayExpr(ArrayExprNode *) override { return {}; }

  std::any visitIndexExpr(IndexExprNode *) override { return {}; }

  std::any visitStructExpr(StructExprNode *) override { return {}; }

  std::any visitCallExpr(CallExprNode *) override { return {}; }

  std::any visitMethodCallExpr(MethodCallExprNode *) override { return {}; }

  std::any visitReturnExpr(ReturnExprNode *) override { return {}; }

  std::any visitAccessExpr(AccessExprNode *) override { return {}; }

  std::any visitContinueExpr(ContinueExprNode *) override { return {}; }

  std::any visitBreakExpr(BreakExprNode *) override { return {}; }

  // Expressions with blocks.
  std::any visitBlockExpr(BlockExprNode *) override { return {}; }

  std::any visitIfExpr(IfExprNode *) override { return {}; }

  std::any visitInfiniteLoopExpr(InfiniteLoopExprNode *) override { return {}; }

  std::any visitPredicateLoopExpr(PredicateLoopExprNode *) override {
    return {};
  }
};
