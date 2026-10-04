#pragma once
#include "AST.hpp"
#include "RxParser.h"
#include <memory>

class ASTBuilder {
public:
  std::unique_ptr<CrateNode>
  visitCrate(RxParser::CrateContext *ctx); // build the tree root

  std::unique_ptr<ItemNode> visitItem(RxParser::ItemContext *ctx);

  std::unique_ptr<FunctionDeclItemNode>
  visitFunctionDeclItem(RxParser::FunctionDefinitionContext *ctx);

  std::unique_ptr<OuterAttributeNode>
  visitOuterAttributeNode(RxParser::OuterAttributeContext *ctx);

  std::unique_ptr<ItemNode>
  visitStructDeclItem(RxParser::StructDefinitionContext *ctx);

  std::unique_ptr<ConstantDeclItemNode>
  visitConstantDeclItem(RxParser::ConstantItemContext *ctx);

  std::unique_ptr<ItemNode>
  visitImplDeclItem(RxParser::InherentImplContext *ctx);

  std::unique_ptr<TypeNode> visitTypeNode(RxParser::TypeRefContext *ctx);

  std::unique_ptr<TypeNode> visitArrayTypeNode(RxParser::ArrayTypeContext *ctx);

  std::unique_ptr<TypeNode>
  visitReferenceTypeNode(RxParser::ReferenceTypeContext *ctx);

  std::unique_ptr<TypeNode> visitPathInTypeNode(RxParser::TypePathContext *ctx);

  std::unique_ptr<ExprNode> visitExprNode(RxParser::ExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitExprNode(RxParser::StatementExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitExprNode(RxParser::ConditionExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitExprNode(RxParser::ConditionBreakExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitAssignmentExprNode(RxParser::AssignmentExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitAssignmentExprNode(RxParser::StatementAssignmentExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitAssignmentExprNode(RxParser::ConditionAssignmentExpressionContext *ctx);

  std::unique_ptr<ExprNode> visitAssignmentExprNode(
      RxParser::ConditionBreakAssignmentExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitLogicalOrExprNode(RxParser::LogicalOrExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitLogicalOrExprNode(RxParser::StatementLogicalOrExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitLogicalOrExprNode(RxParser::ConditionLogicalOrExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitLogicalOrExprNode(RxParser::ConditionBreakLogicalOrExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitLogicalAndExprNode(RxParser::LogicalAndExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitLogicalAndExprNode(RxParser::StatementLogicalAndExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitLogicalAndExprNode(RxParser::ConditionLogicalAndExpressionContext *ctx);

  std::unique_ptr<ExprNode> visitLogicalAndExprNode(
      RxParser::ConditionBreakLogicalAndExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitComparisonExprNode(RxParser::ComparisonExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitComparisonExprNode(RxParser::StatementComparisonExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitComparisonExprNode(RxParser::ConditionComparisonExpressionContext *ctx);

  std::unique_ptr<ExprNode> visitComparisonExprNode(
      RxParser::ConditionBreakComparisonExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitOrExprNode(RxParser::BitOrExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitOrExprNode(RxParser::ClosedBitOrExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitOrExprNode(RxParser::StatementBitOrExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitOrExprNode(RxParser::StatementClosedBitOrExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitOrExprNode(RxParser::ConditionBitOrExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitOrExprNode(RxParser::ConditionClosedBitOrExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitOrExprNode(RxParser::ConditionBreakBitOrExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitOrExprNode(RxParser::ConditionBreakClosedBitOrExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitXorExprNode(RxParser::BitXorExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitXorExprNode(RxParser::ClosedBitXorExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitXorExprNode(RxParser::StatementBitXorExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitXorExprNode(RxParser::StatementClosedBitXorExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitXorExprNode(RxParser::ConditionBitXorExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitXorExprNode(RxParser::ConditionClosedBitXorExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitXorExprNode(RxParser::ConditionBreakBitXorExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitXorExprNode(RxParser::ConditionBreakClosedBitXorExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitAndExprNode(RxParser::BitAndExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitAndExprNode(RxParser::ClosedBitAndExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitAndExprNode(RxParser::StatementBitAndExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitAndExprNode(RxParser::StatementClosedBitAndExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitAndExprNode(RxParser::ConditionBitAndExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitAndExprNode(RxParser::ConditionClosedBitAndExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitAndExprNode(RxParser::ConditionBreakBitAndExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitBitAndExprNode(RxParser::ConditionBreakClosedBitAndExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitShiftExprNode(RxParser::ShiftExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitShiftExprNode(RxParser::ClosedShiftExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitShiftExprNode(RxParser::StatementShiftExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitShiftExprNode(RxParser::StatementClosedShiftExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitShiftExprNode(RxParser::ConditionShiftExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitShiftExprNode(RxParser::ConditionClosedShiftExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitShiftExprNode(RxParser::ConditionBreakShiftExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitShiftExprNode(RxParser::ConditionBreakClosedShiftExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitAdditiveExprNode(RxParser::AdditiveExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitAdditiveExprNode(RxParser::ClosedAdditiveExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitAdditiveExprNode(RxParser::StatementAdditiveExpressionContext *ctx);

  std::unique_ptr<ExprNode> visitAdditiveExprNode(
      RxParser::StatementClosedAdditiveExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitAdditiveExprNode(RxParser::ConditionAdditiveExpressionContext *ctx);

  std::unique_ptr<ExprNode> visitAdditiveExprNode(
      RxParser::ConditionClosedAdditiveExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitAdditiveExprNode(RxParser::ConditionBreakAdditiveExpressionContext *ctx);

  std::unique_ptr<ExprNode> visitAdditiveExprNode(
      RxParser::ConditionBreakClosedAdditiveExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitMultiplicativeExprNode(RxParser::MultiplicativeExpressionContext *ctx);

  std::unique_ptr<ExprNode> visitMultiplicativeExprNode(
      RxParser::ClosedMultiplicativeExpressionContext *ctx);

  std::unique_ptr<ExprNode> visitMultiplicativeExprNode(
      RxParser::StatementMultiplicativeExpressionContext *ctx);

  std::unique_ptr<ExprNode> visitMultiplicativeExprNode(
      RxParser::StatementClosedMultiplicativeExpressionContext *ctx);

  std::unique_ptr<ExprNode> visitMultiplicativeExprNode(
      RxParser::ConditionMultiplicativeExpressionContext *ctx);

  std::unique_ptr<ExprNode> visitMultiplicativeExprNode(
      RxParser::ConditionClosedMultiplicativeExpressionContext *ctx);

  std::unique_ptr<ExprNode> visitMultiplicativeExprNode(
      RxParser::ConditionBreakMultiplicativeExpressionContext *ctx);

  std::unique_ptr<ExprNode> visitMultiplicativeExprNode(
      RxParser::ConditionBreakClosedMultiplicativeExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitCastExprNode(RxParser::CastExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitCastExprNode(RxParser::ClosedCastExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitCastExprNode(RxParser::StatementCastExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitCastExprNode(RxParser::StatementClosedCastExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitCastExprNode(RxParser::ConditionCastExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitCastExprNode(RxParser::ConditionClosedCastExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitCastExprNode(RxParser::ConditionBreakCastExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitCastExprNode(RxParser::ConditionBreakClosedCastExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitUnaryExprNode(RxParser::UnaryExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitUnaryExprNode(RxParser::StatementUnaryExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitUnaryExprNode(RxParser::ConditionUnaryExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitUnaryExprNode(RxParser::ConditionBreakUnaryExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitPostfixExprNode(RxParser::PostfixExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitPostfixExprNode(RxParser::StatementPostfixExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitPostfixExprNode(RxParser::ConditionPostfixExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitPostfixExprNode(RxParser::ConditionBreakPostfixExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitPrimaryExprNode(RxParser::PrimaryExpressionContext *ctx);

  std::unique_ptr<ExprNode>
  visitPrimaryExprNode(RxParser::ConditionPrimaryContext *ctx);

  std::unique_ptr<ExprNode> visitConditionPrimaryWithoutBareBlockExprNode(
      RxParser::ConditionPrimaryWithoutBareBlockContext *ctx);

  std::unique_ptr<ExprNode>
  visitNonBlockPrimaryExprNode(RxParser::NonBlockPrimaryContext *ctx);

  std::unique_ptr<ExprNode>
  visitWithBlockExprNode(RxParser::ExpressionWithBlockContext *ctx);

  std::unique_ptr<ConstExprNode>
  visitConstExprNode(RxParser::ConstValueContext *ctx);

  std::unique_ptr<MagnitudeExprNode>
  visitMagnitudeExprNode(RxParser::MagnitudeContext *ctx);

  std::unique_ptr<BlockExprNode>
  visitBlockExprNode(RxParser::BlockExpressionContext *ctx);

  std::unique_ptr<IfExprNode>
  visitIfExprNode(RxParser::IfExpressionContext *ctx);

  std::unique_ptr<LoopExprNode>
  visitLoopExprNode(RxParser::ExpressionWithBlockContext *ctx);

    std::unique_ptr<LoopExprNode>
  visitLoopExprNode(RxParser::ConditionPrimaryWithoutBareBlockContext *ctx);

  std::unique_ptr<PathInExprNode>
  visitPathInExprNode(RxParser::PathInExpressionContext *ctx);

  std::unique_ptr<StmtNode> visitStmtNode(RxParser::StatementContext *ctx);

  std::unique_ptr<StmtNode>
  visitLetStmtNode(RxParser::LetStatementContext *ctx);

  std::unique_ptr<StmtNode> visitExprStmtNode(RxParser::StatementContext *ctx);

  std::unique_ptr<TypeNode>
  visitClosedCastTypeNode(RxParser::ClosedCastTypeContext *ctx);
};
