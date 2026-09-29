
// Generated from RxParser.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "RxParser.h"


/**
 * This interface defines an abstract listener for a parse tree produced by RxParser.
 */
class  RxParserListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterCrate(RxParser::CrateContext *ctx) = 0;
  virtual void exitCrate(RxParser::CrateContext *ctx) = 0;

  virtual void enterItem(RxParser::ItemContext *ctx) = 0;
  virtual void exitItem(RxParser::ItemContext *ctx) = 0;

  virtual void enterUseDeclaration(RxParser::UseDeclarationContext *ctx) = 0;
  virtual void exitUseDeclaration(RxParser::UseDeclarationContext *ctx) = 0;

  virtual void enterUseTree(RxParser::UseTreeContext *ctx) = 0;
  virtual void exitUseTree(RxParser::UseTreeContext *ctx) = 0;

  virtual void enterUsePath(RxParser::UsePathContext *ctx) = 0;
  virtual void exitUsePath(RxParser::UsePathContext *ctx) = 0;

  virtual void enterUsePathSegment(RxParser::UsePathSegmentContext *ctx) = 0;
  virtual void exitUsePathSegment(RxParser::UsePathSegmentContext *ctx) = 0;

  virtual void enterFunctionDefinition(RxParser::FunctionDefinitionContext *ctx) = 0;
  virtual void exitFunctionDefinition(RxParser::FunctionDefinitionContext *ctx) = 0;

  virtual void enterFunctionParameters(RxParser::FunctionParametersContext *ctx) = 0;
  virtual void exitFunctionParameters(RxParser::FunctionParametersContext *ctx) = 0;

  virtual void enterSelfParam(RxParser::SelfParamContext *ctx) = 0;
  virtual void exitSelfParam(RxParser::SelfParamContext *ctx) = 0;

  virtual void enterFunctionParam(RxParser::FunctionParamContext *ctx) = 0;
  virtual void exitFunctionParam(RxParser::FunctionParamContext *ctx) = 0;

  virtual void enterStructDefinition(RxParser::StructDefinitionContext *ctx) = 0;
  virtual void exitStructDefinition(RxParser::StructDefinitionContext *ctx) = 0;

  virtual void enterStructField(RxParser::StructFieldContext *ctx) = 0;
  virtual void exitStructField(RxParser::StructFieldContext *ctx) = 0;

  virtual void enterOuterAttribute(RxParser::OuterAttributeContext *ctx) = 0;
  virtual void exitOuterAttribute(RxParser::OuterAttributeContext *ctx) = 0;

  virtual void enterDeriveName(RxParser::DeriveNameContext *ctx) = 0;
  virtual void exitDeriveName(RxParser::DeriveNameContext *ctx) = 0;

  virtual void enterConstantItem(RxParser::ConstantItemContext *ctx) = 0;
  virtual void exitConstantItem(RxParser::ConstantItemContext *ctx) = 0;

  virtual void enterInherentImpl(RxParser::InherentImplContext *ctx) = 0;
  virtual void exitInherentImpl(RxParser::InherentImplContext *ctx) = 0;

  virtual void enterAssociatedItem(RxParser::AssociatedItemContext *ctx) = 0;
  virtual void exitAssociatedItem(RxParser::AssociatedItemContext *ctx) = 0;

  virtual void enterGenericParams(RxParser::GenericParamsContext *ctx) = 0;
  virtual void exitGenericParams(RxParser::GenericParamsContext *ctx) = 0;

  virtual void enterLifetimeParam(RxParser::LifetimeParamContext *ctx) = 0;
  virtual void exitLifetimeParam(RxParser::LifetimeParamContext *ctx) = 0;

  virtual void enterLifetime(RxParser::LifetimeContext *ctx) = 0;
  virtual void exitLifetime(RxParser::LifetimeContext *ctx) = 0;

  virtual void enterLifetimeBounds(RxParser::LifetimeBoundsContext *ctx) = 0;
  virtual void exitLifetimeBounds(RxParser::LifetimeBoundsContext *ctx) = 0;

  virtual void enterTypeParamBounds(RxParser::TypeParamBoundsContext *ctx) = 0;
  virtual void exitTypeParamBounds(RxParser::TypeParamBoundsContext *ctx) = 0;

  virtual void enterWhereClause(RxParser::WhereClauseContext *ctx) = 0;
  virtual void exitWhereClause(RxParser::WhereClauseContext *ctx) = 0;

  virtual void enterWhereClauseItem(RxParser::WhereClauseItemContext *ctx) = 0;
  virtual void exitWhereClauseItem(RxParser::WhereClauseItemContext *ctx) = 0;

  virtual void enterTypeRef(RxParser::TypeRefContext *ctx) = 0;
  virtual void exitTypeRef(RxParser::TypeRefContext *ctx) = 0;

  virtual void enterReferenceType(RxParser::ReferenceTypeContext *ctx) = 0;
  virtual void exitReferenceType(RxParser::ReferenceTypeContext *ctx) = 0;

  virtual void enterArrayType(RxParser::ArrayTypeContext *ctx) = 0;
  virtual void exitArrayType(RxParser::ArrayTypeContext *ctx) = 0;

  virtual void enterTypePath(RxParser::TypePathContext *ctx) = 0;
  virtual void exitTypePath(RxParser::TypePathContext *ctx) = 0;

  virtual void enterTypePathSegment(RxParser::TypePathSegmentContext *ctx) = 0;
  virtual void exitTypePathSegment(RxParser::TypePathSegmentContext *ctx) = 0;

  virtual void enterPathInExpression(RxParser::PathInExpressionContext *ctx) = 0;
  virtual void exitPathInExpression(RxParser::PathInExpressionContext *ctx) = 0;

  virtual void enterPathExprSegment(RxParser::PathExprSegmentContext *ctx) = 0;
  virtual void exitPathExprSegment(RxParser::PathExprSegmentContext *ctx) = 0;

  virtual void enterPathIdentSegment(RxParser::PathIdentSegmentContext *ctx) = 0;
  virtual void exitPathIdentSegment(RxParser::PathIdentSegmentContext *ctx) = 0;

  virtual void enterGenericArgs(RxParser::GenericArgsContext *ctx) = 0;
  virtual void exitGenericArgs(RxParser::GenericArgsContext *ctx) = 0;

  virtual void enterGenericArg(RxParser::GenericArgContext *ctx) = 0;
  virtual void exitGenericArg(RxParser::GenericArgContext *ctx) = 0;

  virtual void enterGenericClose(RxParser::GenericCloseContext *ctx) = 0;
  virtual void exitGenericClose(RxParser::GenericCloseContext *ctx) = 0;

  virtual void enterClosedCastType(RxParser::ClosedCastTypeContext *ctx) = 0;
  virtual void exitClosedCastType(RxParser::ClosedCastTypeContext *ctx) = 0;

  virtual void enterConstValue(RxParser::ConstValueContext *ctx) = 0;
  virtual void exitConstValue(RxParser::ConstValueContext *ctx) = 0;

  virtual void enterMagnitude(RxParser::MagnitudeContext *ctx) = 0;
  virtual void exitMagnitude(RxParser::MagnitudeContext *ctx) = 0;

  virtual void enterIdentifierBinding(RxParser::IdentifierBindingContext *ctx) = 0;
  virtual void exitIdentifierBinding(RxParser::IdentifierBindingContext *ctx) = 0;

  virtual void enterLetStatement(RxParser::LetStatementContext *ctx) = 0;
  virtual void exitLetStatement(RxParser::LetStatementContext *ctx) = 0;

  virtual void enterBlockExpression(RxParser::BlockExpressionContext *ctx) = 0;
  virtual void exitBlockExpression(RxParser::BlockExpressionContext *ctx) = 0;

  virtual void enterStatement(RxParser::StatementContext *ctx) = 0;
  virtual void exitStatement(RxParser::StatementContext *ctx) = 0;

  virtual void enterExpressionWithBlock(RxParser::ExpressionWithBlockContext *ctx) = 0;
  virtual void exitExpressionWithBlock(RxParser::ExpressionWithBlockContext *ctx) = 0;

  virtual void enterIfExpression(RxParser::IfExpressionContext *ctx) = 0;
  virtual void exitIfExpression(RxParser::IfExpressionContext *ctx) = 0;

  virtual void enterExpression(RxParser::ExpressionContext *ctx) = 0;
  virtual void exitExpression(RxParser::ExpressionContext *ctx) = 0;

  virtual void enterAssignmentExpression(RxParser::AssignmentExpressionContext *ctx) = 0;
  virtual void exitAssignmentExpression(RxParser::AssignmentExpressionContext *ctx) = 0;

  virtual void enterLogicalOrExpression(RxParser::LogicalOrExpressionContext *ctx) = 0;
  virtual void exitLogicalOrExpression(RxParser::LogicalOrExpressionContext *ctx) = 0;

  virtual void enterLogicalAndExpression(RxParser::LogicalAndExpressionContext *ctx) = 0;
  virtual void exitLogicalAndExpression(RxParser::LogicalAndExpressionContext *ctx) = 0;

  virtual void enterComparisonExpression(RxParser::ComparisonExpressionContext *ctx) = 0;
  virtual void exitComparisonExpression(RxParser::ComparisonExpressionContext *ctx) = 0;

  virtual void enterBitOrExpression(RxParser::BitOrExpressionContext *ctx) = 0;
  virtual void exitBitOrExpression(RxParser::BitOrExpressionContext *ctx) = 0;

  virtual void enterClosedBitOrExpression(RxParser::ClosedBitOrExpressionContext *ctx) = 0;
  virtual void exitClosedBitOrExpression(RxParser::ClosedBitOrExpressionContext *ctx) = 0;

  virtual void enterBitXorExpression(RxParser::BitXorExpressionContext *ctx) = 0;
  virtual void exitBitXorExpression(RxParser::BitXorExpressionContext *ctx) = 0;

  virtual void enterClosedBitXorExpression(RxParser::ClosedBitXorExpressionContext *ctx) = 0;
  virtual void exitClosedBitXorExpression(RxParser::ClosedBitXorExpressionContext *ctx) = 0;

  virtual void enterBitAndExpression(RxParser::BitAndExpressionContext *ctx) = 0;
  virtual void exitBitAndExpression(RxParser::BitAndExpressionContext *ctx) = 0;

  virtual void enterClosedBitAndExpression(RxParser::ClosedBitAndExpressionContext *ctx) = 0;
  virtual void exitClosedBitAndExpression(RxParser::ClosedBitAndExpressionContext *ctx) = 0;

  virtual void enterShiftExpression(RxParser::ShiftExpressionContext *ctx) = 0;
  virtual void exitShiftExpression(RxParser::ShiftExpressionContext *ctx) = 0;

  virtual void enterClosedShiftExpression(RxParser::ClosedShiftExpressionContext *ctx) = 0;
  virtual void exitClosedShiftExpression(RxParser::ClosedShiftExpressionContext *ctx) = 0;

  virtual void enterAdditiveExpression(RxParser::AdditiveExpressionContext *ctx) = 0;
  virtual void exitAdditiveExpression(RxParser::AdditiveExpressionContext *ctx) = 0;

  virtual void enterClosedAdditiveExpression(RxParser::ClosedAdditiveExpressionContext *ctx) = 0;
  virtual void exitClosedAdditiveExpression(RxParser::ClosedAdditiveExpressionContext *ctx) = 0;

  virtual void enterMultiplicativeExpression(RxParser::MultiplicativeExpressionContext *ctx) = 0;
  virtual void exitMultiplicativeExpression(RxParser::MultiplicativeExpressionContext *ctx) = 0;

  virtual void enterClosedMultiplicativeExpression(RxParser::ClosedMultiplicativeExpressionContext *ctx) = 0;
  virtual void exitClosedMultiplicativeExpression(RxParser::ClosedMultiplicativeExpressionContext *ctx) = 0;

  virtual void enterCastExpression(RxParser::CastExpressionContext *ctx) = 0;
  virtual void exitCastExpression(RxParser::CastExpressionContext *ctx) = 0;

  virtual void enterClosedCastExpression(RxParser::ClosedCastExpressionContext *ctx) = 0;
  virtual void exitClosedCastExpression(RxParser::ClosedCastExpressionContext *ctx) = 0;

  virtual void enterUnaryExpression(RxParser::UnaryExpressionContext *ctx) = 0;
  virtual void exitUnaryExpression(RxParser::UnaryExpressionContext *ctx) = 0;

  virtual void enterPostfixExpression(RxParser::PostfixExpressionContext *ctx) = 0;
  virtual void exitPostfixExpression(RxParser::PostfixExpressionContext *ctx) = 0;

  virtual void enterConditionExpression(RxParser::ConditionExpressionContext *ctx) = 0;
  virtual void exitConditionExpression(RxParser::ConditionExpressionContext *ctx) = 0;

  virtual void enterConditionAssignmentExpression(RxParser::ConditionAssignmentExpressionContext *ctx) = 0;
  virtual void exitConditionAssignmentExpression(RxParser::ConditionAssignmentExpressionContext *ctx) = 0;

  virtual void enterConditionLogicalOrExpression(RxParser::ConditionLogicalOrExpressionContext *ctx) = 0;
  virtual void exitConditionLogicalOrExpression(RxParser::ConditionLogicalOrExpressionContext *ctx) = 0;

  virtual void enterConditionLogicalAndExpression(RxParser::ConditionLogicalAndExpressionContext *ctx) = 0;
  virtual void exitConditionLogicalAndExpression(RxParser::ConditionLogicalAndExpressionContext *ctx) = 0;

  virtual void enterConditionComparisonExpression(RxParser::ConditionComparisonExpressionContext *ctx) = 0;
  virtual void exitConditionComparisonExpression(RxParser::ConditionComparisonExpressionContext *ctx) = 0;

  virtual void enterConditionBitOrExpression(RxParser::ConditionBitOrExpressionContext *ctx) = 0;
  virtual void exitConditionBitOrExpression(RxParser::ConditionBitOrExpressionContext *ctx) = 0;

  virtual void enterConditionClosedBitOrExpression(RxParser::ConditionClosedBitOrExpressionContext *ctx) = 0;
  virtual void exitConditionClosedBitOrExpression(RxParser::ConditionClosedBitOrExpressionContext *ctx) = 0;

  virtual void enterConditionBitXorExpression(RxParser::ConditionBitXorExpressionContext *ctx) = 0;
  virtual void exitConditionBitXorExpression(RxParser::ConditionBitXorExpressionContext *ctx) = 0;

  virtual void enterConditionClosedBitXorExpression(RxParser::ConditionClosedBitXorExpressionContext *ctx) = 0;
  virtual void exitConditionClosedBitXorExpression(RxParser::ConditionClosedBitXorExpressionContext *ctx) = 0;

  virtual void enterConditionBitAndExpression(RxParser::ConditionBitAndExpressionContext *ctx) = 0;
  virtual void exitConditionBitAndExpression(RxParser::ConditionBitAndExpressionContext *ctx) = 0;

  virtual void enterConditionClosedBitAndExpression(RxParser::ConditionClosedBitAndExpressionContext *ctx) = 0;
  virtual void exitConditionClosedBitAndExpression(RxParser::ConditionClosedBitAndExpressionContext *ctx) = 0;

  virtual void enterConditionShiftExpression(RxParser::ConditionShiftExpressionContext *ctx) = 0;
  virtual void exitConditionShiftExpression(RxParser::ConditionShiftExpressionContext *ctx) = 0;

  virtual void enterConditionClosedShiftExpression(RxParser::ConditionClosedShiftExpressionContext *ctx) = 0;
  virtual void exitConditionClosedShiftExpression(RxParser::ConditionClosedShiftExpressionContext *ctx) = 0;

  virtual void enterConditionAdditiveExpression(RxParser::ConditionAdditiveExpressionContext *ctx) = 0;
  virtual void exitConditionAdditiveExpression(RxParser::ConditionAdditiveExpressionContext *ctx) = 0;

  virtual void enterConditionClosedAdditiveExpression(RxParser::ConditionClosedAdditiveExpressionContext *ctx) = 0;
  virtual void exitConditionClosedAdditiveExpression(RxParser::ConditionClosedAdditiveExpressionContext *ctx) = 0;

  virtual void enterConditionMultiplicativeExpression(RxParser::ConditionMultiplicativeExpressionContext *ctx) = 0;
  virtual void exitConditionMultiplicativeExpression(RxParser::ConditionMultiplicativeExpressionContext *ctx) = 0;

  virtual void enterConditionClosedMultiplicativeExpression(RxParser::ConditionClosedMultiplicativeExpressionContext *ctx) = 0;
  virtual void exitConditionClosedMultiplicativeExpression(RxParser::ConditionClosedMultiplicativeExpressionContext *ctx) = 0;

  virtual void enterConditionCastExpression(RxParser::ConditionCastExpressionContext *ctx) = 0;
  virtual void exitConditionCastExpression(RxParser::ConditionCastExpressionContext *ctx) = 0;

  virtual void enterConditionClosedCastExpression(RxParser::ConditionClosedCastExpressionContext *ctx) = 0;
  virtual void exitConditionClosedCastExpression(RxParser::ConditionClosedCastExpressionContext *ctx) = 0;

  virtual void enterConditionUnaryExpression(RxParser::ConditionUnaryExpressionContext *ctx) = 0;
  virtual void exitConditionUnaryExpression(RxParser::ConditionUnaryExpressionContext *ctx) = 0;

  virtual void enterConditionPostfixExpression(RxParser::ConditionPostfixExpressionContext *ctx) = 0;
  virtual void exitConditionPostfixExpression(RxParser::ConditionPostfixExpressionContext *ctx) = 0;

  virtual void enterConditionBreakExpression(RxParser::ConditionBreakExpressionContext *ctx) = 0;
  virtual void exitConditionBreakExpression(RxParser::ConditionBreakExpressionContext *ctx) = 0;

  virtual void enterConditionBreakAssignmentExpression(RxParser::ConditionBreakAssignmentExpressionContext *ctx) = 0;
  virtual void exitConditionBreakAssignmentExpression(RxParser::ConditionBreakAssignmentExpressionContext *ctx) = 0;

  virtual void enterConditionBreakLogicalOrExpression(RxParser::ConditionBreakLogicalOrExpressionContext *ctx) = 0;
  virtual void exitConditionBreakLogicalOrExpression(RxParser::ConditionBreakLogicalOrExpressionContext *ctx) = 0;

  virtual void enterConditionBreakLogicalAndExpression(RxParser::ConditionBreakLogicalAndExpressionContext *ctx) = 0;
  virtual void exitConditionBreakLogicalAndExpression(RxParser::ConditionBreakLogicalAndExpressionContext *ctx) = 0;

  virtual void enterConditionBreakComparisonExpression(RxParser::ConditionBreakComparisonExpressionContext *ctx) = 0;
  virtual void exitConditionBreakComparisonExpression(RxParser::ConditionBreakComparisonExpressionContext *ctx) = 0;

  virtual void enterConditionBreakBitOrExpression(RxParser::ConditionBreakBitOrExpressionContext *ctx) = 0;
  virtual void exitConditionBreakBitOrExpression(RxParser::ConditionBreakBitOrExpressionContext *ctx) = 0;

  virtual void enterConditionBreakClosedBitOrExpression(RxParser::ConditionBreakClosedBitOrExpressionContext *ctx) = 0;
  virtual void exitConditionBreakClosedBitOrExpression(RxParser::ConditionBreakClosedBitOrExpressionContext *ctx) = 0;

  virtual void enterConditionBreakBitXorExpression(RxParser::ConditionBreakBitXorExpressionContext *ctx) = 0;
  virtual void exitConditionBreakBitXorExpression(RxParser::ConditionBreakBitXorExpressionContext *ctx) = 0;

  virtual void enterConditionBreakClosedBitXorExpression(RxParser::ConditionBreakClosedBitXorExpressionContext *ctx) = 0;
  virtual void exitConditionBreakClosedBitXorExpression(RxParser::ConditionBreakClosedBitXorExpressionContext *ctx) = 0;

  virtual void enterConditionBreakBitAndExpression(RxParser::ConditionBreakBitAndExpressionContext *ctx) = 0;
  virtual void exitConditionBreakBitAndExpression(RxParser::ConditionBreakBitAndExpressionContext *ctx) = 0;

  virtual void enterConditionBreakClosedBitAndExpression(RxParser::ConditionBreakClosedBitAndExpressionContext *ctx) = 0;
  virtual void exitConditionBreakClosedBitAndExpression(RxParser::ConditionBreakClosedBitAndExpressionContext *ctx) = 0;

  virtual void enterConditionBreakShiftExpression(RxParser::ConditionBreakShiftExpressionContext *ctx) = 0;
  virtual void exitConditionBreakShiftExpression(RxParser::ConditionBreakShiftExpressionContext *ctx) = 0;

  virtual void enterConditionBreakClosedShiftExpression(RxParser::ConditionBreakClosedShiftExpressionContext *ctx) = 0;
  virtual void exitConditionBreakClosedShiftExpression(RxParser::ConditionBreakClosedShiftExpressionContext *ctx) = 0;

  virtual void enterConditionBreakAdditiveExpression(RxParser::ConditionBreakAdditiveExpressionContext *ctx) = 0;
  virtual void exitConditionBreakAdditiveExpression(RxParser::ConditionBreakAdditiveExpressionContext *ctx) = 0;

  virtual void enterConditionBreakClosedAdditiveExpression(RxParser::ConditionBreakClosedAdditiveExpressionContext *ctx) = 0;
  virtual void exitConditionBreakClosedAdditiveExpression(RxParser::ConditionBreakClosedAdditiveExpressionContext *ctx) = 0;

  virtual void enterConditionBreakMultiplicativeExpression(RxParser::ConditionBreakMultiplicativeExpressionContext *ctx) = 0;
  virtual void exitConditionBreakMultiplicativeExpression(RxParser::ConditionBreakMultiplicativeExpressionContext *ctx) = 0;

  virtual void enterConditionBreakClosedMultiplicativeExpression(RxParser::ConditionBreakClosedMultiplicativeExpressionContext *ctx) = 0;
  virtual void exitConditionBreakClosedMultiplicativeExpression(RxParser::ConditionBreakClosedMultiplicativeExpressionContext *ctx) = 0;

  virtual void enterConditionBreakCastExpression(RxParser::ConditionBreakCastExpressionContext *ctx) = 0;
  virtual void exitConditionBreakCastExpression(RxParser::ConditionBreakCastExpressionContext *ctx) = 0;

  virtual void enterConditionBreakClosedCastExpression(RxParser::ConditionBreakClosedCastExpressionContext *ctx) = 0;
  virtual void exitConditionBreakClosedCastExpression(RxParser::ConditionBreakClosedCastExpressionContext *ctx) = 0;

  virtual void enterConditionBreakUnaryExpression(RxParser::ConditionBreakUnaryExpressionContext *ctx) = 0;
  virtual void exitConditionBreakUnaryExpression(RxParser::ConditionBreakUnaryExpressionContext *ctx) = 0;

  virtual void enterConditionBreakPostfixExpression(RxParser::ConditionBreakPostfixExpressionContext *ctx) = 0;
  virtual void exitConditionBreakPostfixExpression(RxParser::ConditionBreakPostfixExpressionContext *ctx) = 0;

  virtual void enterStatementExpression(RxParser::StatementExpressionContext *ctx) = 0;
  virtual void exitStatementExpression(RxParser::StatementExpressionContext *ctx) = 0;

  virtual void enterStatementAssignmentExpression(RxParser::StatementAssignmentExpressionContext *ctx) = 0;
  virtual void exitStatementAssignmentExpression(RxParser::StatementAssignmentExpressionContext *ctx) = 0;

  virtual void enterStatementLogicalOrExpression(RxParser::StatementLogicalOrExpressionContext *ctx) = 0;
  virtual void exitStatementLogicalOrExpression(RxParser::StatementLogicalOrExpressionContext *ctx) = 0;

  virtual void enterStatementLogicalAndExpression(RxParser::StatementLogicalAndExpressionContext *ctx) = 0;
  virtual void exitStatementLogicalAndExpression(RxParser::StatementLogicalAndExpressionContext *ctx) = 0;

  virtual void enterStatementComparisonExpression(RxParser::StatementComparisonExpressionContext *ctx) = 0;
  virtual void exitStatementComparisonExpression(RxParser::StatementComparisonExpressionContext *ctx) = 0;

  virtual void enterStatementBitOrExpression(RxParser::StatementBitOrExpressionContext *ctx) = 0;
  virtual void exitStatementBitOrExpression(RxParser::StatementBitOrExpressionContext *ctx) = 0;

  virtual void enterStatementClosedBitOrExpression(RxParser::StatementClosedBitOrExpressionContext *ctx) = 0;
  virtual void exitStatementClosedBitOrExpression(RxParser::StatementClosedBitOrExpressionContext *ctx) = 0;

  virtual void enterStatementBitXorExpression(RxParser::StatementBitXorExpressionContext *ctx) = 0;
  virtual void exitStatementBitXorExpression(RxParser::StatementBitXorExpressionContext *ctx) = 0;

  virtual void enterStatementClosedBitXorExpression(RxParser::StatementClosedBitXorExpressionContext *ctx) = 0;
  virtual void exitStatementClosedBitXorExpression(RxParser::StatementClosedBitXorExpressionContext *ctx) = 0;

  virtual void enterStatementBitAndExpression(RxParser::StatementBitAndExpressionContext *ctx) = 0;
  virtual void exitStatementBitAndExpression(RxParser::StatementBitAndExpressionContext *ctx) = 0;

  virtual void enterStatementClosedBitAndExpression(RxParser::StatementClosedBitAndExpressionContext *ctx) = 0;
  virtual void exitStatementClosedBitAndExpression(RxParser::StatementClosedBitAndExpressionContext *ctx) = 0;

  virtual void enterStatementShiftExpression(RxParser::StatementShiftExpressionContext *ctx) = 0;
  virtual void exitStatementShiftExpression(RxParser::StatementShiftExpressionContext *ctx) = 0;

  virtual void enterStatementClosedShiftExpression(RxParser::StatementClosedShiftExpressionContext *ctx) = 0;
  virtual void exitStatementClosedShiftExpression(RxParser::StatementClosedShiftExpressionContext *ctx) = 0;

  virtual void enterStatementAdditiveExpression(RxParser::StatementAdditiveExpressionContext *ctx) = 0;
  virtual void exitStatementAdditiveExpression(RxParser::StatementAdditiveExpressionContext *ctx) = 0;

  virtual void enterStatementClosedAdditiveExpression(RxParser::StatementClosedAdditiveExpressionContext *ctx) = 0;
  virtual void exitStatementClosedAdditiveExpression(RxParser::StatementClosedAdditiveExpressionContext *ctx) = 0;

  virtual void enterStatementMultiplicativeExpression(RxParser::StatementMultiplicativeExpressionContext *ctx) = 0;
  virtual void exitStatementMultiplicativeExpression(RxParser::StatementMultiplicativeExpressionContext *ctx) = 0;

  virtual void enterStatementClosedMultiplicativeExpression(RxParser::StatementClosedMultiplicativeExpressionContext *ctx) = 0;
  virtual void exitStatementClosedMultiplicativeExpression(RxParser::StatementClosedMultiplicativeExpressionContext *ctx) = 0;

  virtual void enterStatementCastExpression(RxParser::StatementCastExpressionContext *ctx) = 0;
  virtual void exitStatementCastExpression(RxParser::StatementCastExpressionContext *ctx) = 0;

  virtual void enterStatementClosedCastExpression(RxParser::StatementClosedCastExpressionContext *ctx) = 0;
  virtual void exitStatementClosedCastExpression(RxParser::StatementClosedCastExpressionContext *ctx) = 0;

  virtual void enterStatementUnaryExpression(RxParser::StatementUnaryExpressionContext *ctx) = 0;
  virtual void exitStatementUnaryExpression(RxParser::StatementUnaryExpressionContext *ctx) = 0;

  virtual void enterStatementPostfixExpression(RxParser::StatementPostfixExpressionContext *ctx) = 0;
  virtual void exitStatementPostfixExpression(RxParser::StatementPostfixExpressionContext *ctx) = 0;

  virtual void enterPrimaryExpression(RxParser::PrimaryExpressionContext *ctx) = 0;
  virtual void exitPrimaryExpression(RxParser::PrimaryExpressionContext *ctx) = 0;

  virtual void enterNonBlockPrimary(RxParser::NonBlockPrimaryContext *ctx) = 0;
  virtual void exitNonBlockPrimary(RxParser::NonBlockPrimaryContext *ctx) = 0;

  virtual void enterConditionPrimary(RxParser::ConditionPrimaryContext *ctx) = 0;
  virtual void exitConditionPrimary(RxParser::ConditionPrimaryContext *ctx) = 0;

  virtual void enterConditionPrimaryWithoutBareBlock(RxParser::ConditionPrimaryWithoutBareBlockContext *ctx) = 0;
  virtual void exitConditionPrimaryWithoutBareBlock(RxParser::ConditionPrimaryWithoutBareBlockContext *ctx) = 0;

  virtual void enterLiteralExpression(RxParser::LiteralExpressionContext *ctx) = 0;
  virtual void exitLiteralExpression(RxParser::LiteralExpressionContext *ctx) = 0;

  virtual void enterStructExprFields(RxParser::StructExprFieldsContext *ctx) = 0;
  virtual void exitStructExprFields(RxParser::StructExprFieldsContext *ctx) = 0;

  virtual void enterStructExprField(RxParser::StructExprFieldContext *ctx) = 0;
  virtual void exitStructExprField(RxParser::StructExprFieldContext *ctx) = 0;

  virtual void enterArrayExpression(RxParser::ArrayExpressionContext *ctx) = 0;
  virtual void exitArrayExpression(RxParser::ArrayExpressionContext *ctx) = 0;

  virtual void enterPostfixSuffix(RxParser::PostfixSuffixContext *ctx) = 0;
  virtual void exitPostfixSuffix(RxParser::PostfixSuffixContext *ctx) = 0;

  virtual void enterDotSuffix(RxParser::DotSuffixContext *ctx) = 0;
  virtual void exitDotSuffix(RxParser::DotSuffixContext *ctx) = 0;

  virtual void enterCallArguments(RxParser::CallArgumentsContext *ctx) = 0;
  virtual void exitCallArguments(RxParser::CallArgumentsContext *ctx) = 0;

  virtual void enterUnaryOperator(RxParser::UnaryOperatorContext *ctx) = 0;
  virtual void exitUnaryOperator(RxParser::UnaryOperatorContext *ctx) = 0;

  virtual void enterMultiplicativeOperator(RxParser::MultiplicativeOperatorContext *ctx) = 0;
  virtual void exitMultiplicativeOperator(RxParser::MultiplicativeOperatorContext *ctx) = 0;

  virtual void enterAdditiveOperator(RxParser::AdditiveOperatorContext *ctx) = 0;
  virtual void exitAdditiveOperator(RxParser::AdditiveOperatorContext *ctx) = 0;

  virtual void enterShiftRight(RxParser::ShiftRightContext *ctx) = 0;
  virtual void exitShiftRight(RxParser::ShiftRightContext *ctx) = 0;

  virtual void enterComparisonExceptLt(RxParser::ComparisonExceptLtContext *ctx) = 0;
  virtual void exitComparisonExceptLt(RxParser::ComparisonExceptLtContext *ctx) = 0;

  virtual void enterAssignmentOperator(RxParser::AssignmentOperatorContext *ctx) = 0;
  virtual void exitAssignmentOperator(RxParser::AssignmentOperatorContext *ctx) = 0;

  virtual void enterEqualsSign(RxParser::EqualsSignContext *ctx) = 0;
  virtual void exitEqualsSign(RxParser::EqualsSignContext *ctx) = 0;

  virtual void enterIdentifier(RxParser::IdentifierContext *ctx) = 0;
  virtual void exitIdentifier(RxParser::IdentifierContext *ctx) = 0;


};

