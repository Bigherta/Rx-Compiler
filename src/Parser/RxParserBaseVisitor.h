
// Generated from RxParser.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "RxParserVisitor.h"


/**
 * This class provides an empty implementation of RxParserVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  RxParserBaseVisitor : public RxParserVisitor {
public:

  virtual std::any visitCrate(RxParser::CrateContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitItem(RxParser::ItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUseDeclaration(RxParser::UseDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUseTree(RxParser::UseTreeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUsePath(RxParser::UsePathContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUsePathSegment(RxParser::UsePathSegmentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFunctionDefinition(RxParser::FunctionDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFunctionParameters(RxParser::FunctionParametersContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSelfParam(RxParser::SelfParamContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFunctionParam(RxParser::FunctionParamContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStructDefinition(RxParser::StructDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStructField(RxParser::StructFieldContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOuterAttribute(RxParser::OuterAttributeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDeriveName(RxParser::DeriveNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstantItem(RxParser::ConstantItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInherentImpl(RxParser::InherentImplContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAssociatedItem(RxParser::AssociatedItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGenericParams(RxParser::GenericParamsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLifetimeParam(RxParser::LifetimeParamContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLifetime(RxParser::LifetimeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLifetimeBounds(RxParser::LifetimeBoundsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTypeParamBounds(RxParser::TypeParamBoundsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWhereClause(RxParser::WhereClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWhereClauseItem(RxParser::WhereClauseItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTypeRef(RxParser::TypeRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReferenceType(RxParser::ReferenceTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitArrayType(RxParser::ArrayTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTypePath(RxParser::TypePathContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTypePathSegment(RxParser::TypePathSegmentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPathInExpression(RxParser::PathInExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPathExprSegment(RxParser::PathExprSegmentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPathIdentSegment(RxParser::PathIdentSegmentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGenericArgs(RxParser::GenericArgsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGenericArg(RxParser::GenericArgContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGenericClose(RxParser::GenericCloseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClosedCastType(RxParser::ClosedCastTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstValue(RxParser::ConstValueContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMagnitude(RxParser::MagnitudeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifierBinding(RxParser::IdentifierBindingContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLetStatement(RxParser::LetStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlockExpression(RxParser::BlockExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatement(RxParser::StatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpressionWithBlock(RxParser::ExpressionWithBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIfExpression(RxParser::IfExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpression(RxParser::ExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAssignmentExpression(RxParser::AssignmentExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLogicalOrExpression(RxParser::LogicalOrExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLogicalAndExpression(RxParser::LogicalAndExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitComparisonExpression(RxParser::ComparisonExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBitOrExpression(RxParser::BitOrExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClosedBitOrExpression(RxParser::ClosedBitOrExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBitXorExpression(RxParser::BitXorExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClosedBitXorExpression(RxParser::ClosedBitXorExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBitAndExpression(RxParser::BitAndExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClosedBitAndExpression(RxParser::ClosedBitAndExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShiftExpression(RxParser::ShiftExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClosedShiftExpression(RxParser::ClosedShiftExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAdditiveExpression(RxParser::AdditiveExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClosedAdditiveExpression(RxParser::ClosedAdditiveExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMultiplicativeExpression(RxParser::MultiplicativeExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClosedMultiplicativeExpression(RxParser::ClosedMultiplicativeExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCastExpression(RxParser::CastExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClosedCastExpression(RxParser::ClosedCastExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryExpression(RxParser::UnaryExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPostfixExpression(RxParser::PostfixExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionExpression(RxParser::ConditionExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionAssignmentExpression(RxParser::ConditionAssignmentExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionLogicalOrExpression(RxParser::ConditionLogicalOrExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionLogicalAndExpression(RxParser::ConditionLogicalAndExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionComparisonExpression(RxParser::ConditionComparisonExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBitOrExpression(RxParser::ConditionBitOrExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionClosedBitOrExpression(RxParser::ConditionClosedBitOrExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBitXorExpression(RxParser::ConditionBitXorExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionClosedBitXorExpression(RxParser::ConditionClosedBitXorExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBitAndExpression(RxParser::ConditionBitAndExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionClosedBitAndExpression(RxParser::ConditionClosedBitAndExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionShiftExpression(RxParser::ConditionShiftExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionClosedShiftExpression(RxParser::ConditionClosedShiftExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionAdditiveExpression(RxParser::ConditionAdditiveExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionClosedAdditiveExpression(RxParser::ConditionClosedAdditiveExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionMultiplicativeExpression(RxParser::ConditionMultiplicativeExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionClosedMultiplicativeExpression(RxParser::ConditionClosedMultiplicativeExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionCastExpression(RxParser::ConditionCastExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionClosedCastExpression(RxParser::ConditionClosedCastExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionUnaryExpression(RxParser::ConditionUnaryExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionPostfixExpression(RxParser::ConditionPostfixExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakExpression(RxParser::ConditionBreakExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakAssignmentExpression(RxParser::ConditionBreakAssignmentExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakLogicalOrExpression(RxParser::ConditionBreakLogicalOrExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakLogicalAndExpression(RxParser::ConditionBreakLogicalAndExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakComparisonExpression(RxParser::ConditionBreakComparisonExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakBitOrExpression(RxParser::ConditionBreakBitOrExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakClosedBitOrExpression(RxParser::ConditionBreakClosedBitOrExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakBitXorExpression(RxParser::ConditionBreakBitXorExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakClosedBitXorExpression(RxParser::ConditionBreakClosedBitXorExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakBitAndExpression(RxParser::ConditionBreakBitAndExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakClosedBitAndExpression(RxParser::ConditionBreakClosedBitAndExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakShiftExpression(RxParser::ConditionBreakShiftExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakClosedShiftExpression(RxParser::ConditionBreakClosedShiftExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakAdditiveExpression(RxParser::ConditionBreakAdditiveExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakClosedAdditiveExpression(RxParser::ConditionBreakClosedAdditiveExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakMultiplicativeExpression(RxParser::ConditionBreakMultiplicativeExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakClosedMultiplicativeExpression(RxParser::ConditionBreakClosedMultiplicativeExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakCastExpression(RxParser::ConditionBreakCastExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakClosedCastExpression(RxParser::ConditionBreakClosedCastExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakUnaryExpression(RxParser::ConditionBreakUnaryExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionBreakPostfixExpression(RxParser::ConditionBreakPostfixExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementExpression(RxParser::StatementExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementAssignmentExpression(RxParser::StatementAssignmentExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementLogicalOrExpression(RxParser::StatementLogicalOrExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementLogicalAndExpression(RxParser::StatementLogicalAndExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementComparisonExpression(RxParser::StatementComparisonExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementBitOrExpression(RxParser::StatementBitOrExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementClosedBitOrExpression(RxParser::StatementClosedBitOrExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementBitXorExpression(RxParser::StatementBitXorExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementClosedBitXorExpression(RxParser::StatementClosedBitXorExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementBitAndExpression(RxParser::StatementBitAndExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementClosedBitAndExpression(RxParser::StatementClosedBitAndExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementShiftExpression(RxParser::StatementShiftExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementClosedShiftExpression(RxParser::StatementClosedShiftExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementAdditiveExpression(RxParser::StatementAdditiveExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementClosedAdditiveExpression(RxParser::StatementClosedAdditiveExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementMultiplicativeExpression(RxParser::StatementMultiplicativeExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementClosedMultiplicativeExpression(RxParser::StatementClosedMultiplicativeExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementCastExpression(RxParser::StatementCastExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementClosedCastExpression(RxParser::StatementClosedCastExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementUnaryExpression(RxParser::StatementUnaryExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementPostfixExpression(RxParser::StatementPostfixExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrimaryExpression(RxParser::PrimaryExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNonBlockPrimary(RxParser::NonBlockPrimaryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionPrimary(RxParser::ConditionPrimaryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionPrimaryWithoutBareBlock(RxParser::ConditionPrimaryWithoutBareBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLiteralExpression(RxParser::LiteralExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStructExprFields(RxParser::StructExprFieldsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStructExprField(RxParser::StructExprFieldContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitArrayExpression(RxParser::ArrayExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPostfixSuffix(RxParser::PostfixSuffixContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDotSuffix(RxParser::DotSuffixContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCallArguments(RxParser::CallArgumentsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryOperator(RxParser::UnaryOperatorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMultiplicativeOperator(RxParser::MultiplicativeOperatorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAdditiveOperator(RxParser::AdditiveOperatorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShiftRight(RxParser::ShiftRightContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitComparisonExceptLt(RxParser::ComparisonExceptLtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAssignmentOperator(RxParser::AssignmentOperatorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEqualsSign(RxParser::EqualsSignContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifier(RxParser::IdentifierContext *ctx) override {
    return visitChildren(ctx);
  }


};

