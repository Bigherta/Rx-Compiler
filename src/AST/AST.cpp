#include "AST.hpp"
#include "../Frontend/ASTVisitor.hpp"

// Type is complete here, so unique_ptr<Type> can be safely destroyed.
ExprNode &ExprNode::operator=(ExprNode &&) noexcept = default;

ExprNode::~ExprNode() = default;

PathInExprNode &PathInExprNode::operator=(PathInExprNode &&) noexcept = default;

PathInExprNode::~PathInExprNode() = default;

std::any CrateNode::accept(ASTVisitor *visitor) {
  return visitor->visitCrate(this);
}

std::any OuterAttributeNode::accept(ASTVisitor *visitor) {
  return visitor->visitOuterAttribute(this);
}

std::any UnitTypeNode::accept(ASTVisitor *visitor) {
  return visitor->visitUnitType(this);
}

std::any PathInTypeNode::accept(ASTVisitor *visitor) {
  return visitor->visitPathInType(this);
}

std::any ReferenceTypeNode::accept(ASTVisitor *visitor) {
  return visitor->visitReferenceType(this);
}

std::any ArrayTypeNode::accept(ASTVisitor *visitor) {
  return visitor->visitArrayType(this);
}

std::any FunctionDeclItemNode::accept(ASTVisitor *visitor) {
  return visitor->visitFunctionDeclItem(this);
}

std::any StructDeclItemNode::accept(ASTVisitor *visitor) {
  return visitor->visitStructDeclItem(this);
}

std::any ConstantDeclItemNode::accept(ASTVisitor *visitor) {
  return visitor->visitConstantDeclItem(this);
}

std::any ImplDeclItemNode::accept(ASTVisitor *visitor) {
  return visitor->visitImplDeclItem(this);
}

std::any EmptyStmtNode::accept(ASTVisitor *visitor) {
  return visitor->visitEmptyStmt(this);
}

std::any LetStmtNode::accept(ASTVisitor *visitor) {
  return visitor->visitLetStmt(this);
}

std::any ExpressionStmtNode::accept(ASTVisitor *visitor) {
  return visitor->visitExpressionStmt(this);
}

std::any UnitExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitUnitExpr(this);
}

std::any PathInExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitPathInExpr(this);
}

std::any LiteralExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitLiteralExpr(this);
}

std::any UnaryExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitUnaryExpr(this);
}

std::any BinaryExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitBinaryExpr(this);
}

std::any ComparisonExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitComparisonExpr(this);
}

std::any LogicExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitLogicExpr(this);
}

std::any CastExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitCastExpr(this);
}

std::any BorrowExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitBorrowExpr(this);
}

std::any DereferenceExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitDereferenceExpr(this);
}

std::any AssignmentExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitAssignmentExpr(this);
}

std::any MagnitudeExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitMagnitudeExpr(this);
}

std::any ConstExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitConstExpr(this);
}

std::any ArrayExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitArrayExpr(this);
}

std::any IndexExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitIndexExpr(this);
}

std::any StructExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitStructExpr(this);
}

std::any CallExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitCallExpr(this);
}

std::any MethodCallExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitMethodCallExpr(this);
}

std::any ReturnExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitReturnExpr(this);
}

std::any AccessExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitAccessExpr(this);
}

std::any ContinueExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitContinueExpr(this);
}

std::any BreakExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitBreakExpr(this);
}

std::any BlockExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitBlockExpr(this);
}

std::any IfExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitIfExpr(this);
}

std::any InfiniteLoopExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitInfiniteLoopExpr(this);
}

std::any PredicateLoopExprNode::accept(ASTVisitor *visitor) {
  return visitor->visitPredicateLoopExpr(this);
}
