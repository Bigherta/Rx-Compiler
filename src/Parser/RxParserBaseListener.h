
// Generated from RxParser.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "RxParserListener.h"


/**
 * This class provides an empty implementation of RxParserListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  RxParserBaseListener : public RxParserListener {
public:

  virtual void enterCrate(RxParser::CrateContext * /*ctx*/) override { }
  virtual void exitCrate(RxParser::CrateContext * /*ctx*/) override { }

  virtual void enterItem(RxParser::ItemContext * /*ctx*/) override { }
  virtual void exitItem(RxParser::ItemContext * /*ctx*/) override { }

  virtual void enterUseDeclaration(RxParser::UseDeclarationContext * /*ctx*/) override { }
  virtual void exitUseDeclaration(RxParser::UseDeclarationContext * /*ctx*/) override { }

  virtual void enterUseTree(RxParser::UseTreeContext * /*ctx*/) override { }
  virtual void exitUseTree(RxParser::UseTreeContext * /*ctx*/) override { }

  virtual void enterUsePath(RxParser::UsePathContext * /*ctx*/) override { }
  virtual void exitUsePath(RxParser::UsePathContext * /*ctx*/) override { }

  virtual void enterUsePathSegment(RxParser::UsePathSegmentContext * /*ctx*/) override { }
  virtual void exitUsePathSegment(RxParser::UsePathSegmentContext * /*ctx*/) override { }

  virtual void enterFunctionDefinition(RxParser::FunctionDefinitionContext * /*ctx*/) override { }
  virtual void exitFunctionDefinition(RxParser::FunctionDefinitionContext * /*ctx*/) override { }

  virtual void enterFunctionParameters(RxParser::FunctionParametersContext * /*ctx*/) override { }
  virtual void exitFunctionParameters(RxParser::FunctionParametersContext * /*ctx*/) override { }

  virtual void enterSelfParam(RxParser::SelfParamContext * /*ctx*/) override { }
  virtual void exitSelfParam(RxParser::SelfParamContext * /*ctx*/) override { }

  virtual void enterFunctionParam(RxParser::FunctionParamContext * /*ctx*/) override { }
  virtual void exitFunctionParam(RxParser::FunctionParamContext * /*ctx*/) override { }

  virtual void enterStructDefinition(RxParser::StructDefinitionContext * /*ctx*/) override { }
  virtual void exitStructDefinition(RxParser::StructDefinitionContext * /*ctx*/) override { }

  virtual void enterStructField(RxParser::StructFieldContext * /*ctx*/) override { }
  virtual void exitStructField(RxParser::StructFieldContext * /*ctx*/) override { }

  virtual void enterOuterAttribute(RxParser::OuterAttributeContext * /*ctx*/) override { }
  virtual void exitOuterAttribute(RxParser::OuterAttributeContext * /*ctx*/) override { }

  virtual void enterDeriveName(RxParser::DeriveNameContext * /*ctx*/) override { }
  virtual void exitDeriveName(RxParser::DeriveNameContext * /*ctx*/) override { }

  virtual void enterConstantItem(RxParser::ConstantItemContext * /*ctx*/) override { }
  virtual void exitConstantItem(RxParser::ConstantItemContext * /*ctx*/) override { }

  virtual void enterInherentImpl(RxParser::InherentImplContext * /*ctx*/) override { }
  virtual void exitInherentImpl(RxParser::InherentImplContext * /*ctx*/) override { }

  virtual void enterAssociatedItem(RxParser::AssociatedItemContext * /*ctx*/) override { }
  virtual void exitAssociatedItem(RxParser::AssociatedItemContext * /*ctx*/) override { }

  virtual void enterGenericParams(RxParser::GenericParamsContext * /*ctx*/) override { }
  virtual void exitGenericParams(RxParser::GenericParamsContext * /*ctx*/) override { }

  virtual void enterLifetimeParam(RxParser::LifetimeParamContext * /*ctx*/) override { }
  virtual void exitLifetimeParam(RxParser::LifetimeParamContext * /*ctx*/) override { }

  virtual void enterLifetime(RxParser::LifetimeContext * /*ctx*/) override { }
  virtual void exitLifetime(RxParser::LifetimeContext * /*ctx*/) override { }

  virtual void enterLifetimeBounds(RxParser::LifetimeBoundsContext * /*ctx*/) override { }
  virtual void exitLifetimeBounds(RxParser::LifetimeBoundsContext * /*ctx*/) override { }

  virtual void enterTypeParamBounds(RxParser::TypeParamBoundsContext * /*ctx*/) override { }
  virtual void exitTypeParamBounds(RxParser::TypeParamBoundsContext * /*ctx*/) override { }

  virtual void enterWhereClause(RxParser::WhereClauseContext * /*ctx*/) override { }
  virtual void exitWhereClause(RxParser::WhereClauseContext * /*ctx*/) override { }

  virtual void enterWhereClauseItem(RxParser::WhereClauseItemContext * /*ctx*/) override { }
  virtual void exitWhereClauseItem(RxParser::WhereClauseItemContext * /*ctx*/) override { }

  virtual void enterTypeRef(RxParser::TypeRefContext * /*ctx*/) override { }
  virtual void exitTypeRef(RxParser::TypeRefContext * /*ctx*/) override { }

  virtual void enterReferenceType(RxParser::ReferenceTypeContext * /*ctx*/) override { }
  virtual void exitReferenceType(RxParser::ReferenceTypeContext * /*ctx*/) override { }

  virtual void enterArrayType(RxParser::ArrayTypeContext * /*ctx*/) override { }
  virtual void exitArrayType(RxParser::ArrayTypeContext * /*ctx*/) override { }

  virtual void enterTypePath(RxParser::TypePathContext * /*ctx*/) override { }
  virtual void exitTypePath(RxParser::TypePathContext * /*ctx*/) override { }

  virtual void enterTypePathSegment(RxParser::TypePathSegmentContext * /*ctx*/) override { }
  virtual void exitTypePathSegment(RxParser::TypePathSegmentContext * /*ctx*/) override { }

  virtual void enterPathInExpression(RxParser::PathInExpressionContext * /*ctx*/) override { }
  virtual void exitPathInExpression(RxParser::PathInExpressionContext * /*ctx*/) override { }

  virtual void enterPathExprSegment(RxParser::PathExprSegmentContext * /*ctx*/) override { }
  virtual void exitPathExprSegment(RxParser::PathExprSegmentContext * /*ctx*/) override { }

  virtual void enterPathIdentSegment(RxParser::PathIdentSegmentContext * /*ctx*/) override { }
  virtual void exitPathIdentSegment(RxParser::PathIdentSegmentContext * /*ctx*/) override { }

  virtual void enterGenericArgs(RxParser::GenericArgsContext * /*ctx*/) override { }
  virtual void exitGenericArgs(RxParser::GenericArgsContext * /*ctx*/) override { }

  virtual void enterGenericArg(RxParser::GenericArgContext * /*ctx*/) override { }
  virtual void exitGenericArg(RxParser::GenericArgContext * /*ctx*/) override { }

  virtual void enterGenericClose(RxParser::GenericCloseContext * /*ctx*/) override { }
  virtual void exitGenericClose(RxParser::GenericCloseContext * /*ctx*/) override { }

  virtual void enterClosedCastType(RxParser::ClosedCastTypeContext * /*ctx*/) override { }
  virtual void exitClosedCastType(RxParser::ClosedCastTypeContext * /*ctx*/) override { }

  virtual void enterConstValue(RxParser::ConstValueContext * /*ctx*/) override { }
  virtual void exitConstValue(RxParser::ConstValueContext * /*ctx*/) override { }

  virtual void enterMagnitude(RxParser::MagnitudeContext * /*ctx*/) override { }
  virtual void exitMagnitude(RxParser::MagnitudeContext * /*ctx*/) override { }

  virtual void enterIdentifierBinding(RxParser::IdentifierBindingContext * /*ctx*/) override { }
  virtual void exitIdentifierBinding(RxParser::IdentifierBindingContext * /*ctx*/) override { }

  virtual void enterLetStatement(RxParser::LetStatementContext * /*ctx*/) override { }
  virtual void exitLetStatement(RxParser::LetStatementContext * /*ctx*/) override { }

  virtual void enterBlockExpression(RxParser::BlockExpressionContext * /*ctx*/) override { }
  virtual void exitBlockExpression(RxParser::BlockExpressionContext * /*ctx*/) override { }

  virtual void enterStatement(RxParser::StatementContext * /*ctx*/) override { }
  virtual void exitStatement(RxParser::StatementContext * /*ctx*/) override { }

  virtual void enterExpressionWithBlock(RxParser::ExpressionWithBlockContext * /*ctx*/) override { }
  virtual void exitExpressionWithBlock(RxParser::ExpressionWithBlockContext * /*ctx*/) override { }

  virtual void enterIfExpression(RxParser::IfExpressionContext * /*ctx*/) override { }
  virtual void exitIfExpression(RxParser::IfExpressionContext * /*ctx*/) override { }

  virtual void enterExpression(RxParser::ExpressionContext * /*ctx*/) override { }
  virtual void exitExpression(RxParser::ExpressionContext * /*ctx*/) override { }

  virtual void enterAssignmentExpression(RxParser::AssignmentExpressionContext * /*ctx*/) override { }
  virtual void exitAssignmentExpression(RxParser::AssignmentExpressionContext * /*ctx*/) override { }

  virtual void enterLogicalOrExpression(RxParser::LogicalOrExpressionContext * /*ctx*/) override { }
  virtual void exitLogicalOrExpression(RxParser::LogicalOrExpressionContext * /*ctx*/) override { }

  virtual void enterLogicalAndExpression(RxParser::LogicalAndExpressionContext * /*ctx*/) override { }
  virtual void exitLogicalAndExpression(RxParser::LogicalAndExpressionContext * /*ctx*/) override { }

  virtual void enterComparisonExpression(RxParser::ComparisonExpressionContext * /*ctx*/) override { }
  virtual void exitComparisonExpression(RxParser::ComparisonExpressionContext * /*ctx*/) override { }

  virtual void enterBitOrExpression(RxParser::BitOrExpressionContext * /*ctx*/) override { }
  virtual void exitBitOrExpression(RxParser::BitOrExpressionContext * /*ctx*/) override { }

  virtual void enterClosedBitOrExpression(RxParser::ClosedBitOrExpressionContext * /*ctx*/) override { }
  virtual void exitClosedBitOrExpression(RxParser::ClosedBitOrExpressionContext * /*ctx*/) override { }

  virtual void enterBitXorExpression(RxParser::BitXorExpressionContext * /*ctx*/) override { }
  virtual void exitBitXorExpression(RxParser::BitXorExpressionContext * /*ctx*/) override { }

  virtual void enterClosedBitXorExpression(RxParser::ClosedBitXorExpressionContext * /*ctx*/) override { }
  virtual void exitClosedBitXorExpression(RxParser::ClosedBitXorExpressionContext * /*ctx*/) override { }

  virtual void enterBitAndExpression(RxParser::BitAndExpressionContext * /*ctx*/) override { }
  virtual void exitBitAndExpression(RxParser::BitAndExpressionContext * /*ctx*/) override { }

  virtual void enterClosedBitAndExpression(RxParser::ClosedBitAndExpressionContext * /*ctx*/) override { }
  virtual void exitClosedBitAndExpression(RxParser::ClosedBitAndExpressionContext * /*ctx*/) override { }

  virtual void enterShiftExpression(RxParser::ShiftExpressionContext * /*ctx*/) override { }
  virtual void exitShiftExpression(RxParser::ShiftExpressionContext * /*ctx*/) override { }

  virtual void enterClosedShiftExpression(RxParser::ClosedShiftExpressionContext * /*ctx*/) override { }
  virtual void exitClosedShiftExpression(RxParser::ClosedShiftExpressionContext * /*ctx*/) override { }

  virtual void enterAdditiveExpression(RxParser::AdditiveExpressionContext * /*ctx*/) override { }
  virtual void exitAdditiveExpression(RxParser::AdditiveExpressionContext * /*ctx*/) override { }

  virtual void enterClosedAdditiveExpression(RxParser::ClosedAdditiveExpressionContext * /*ctx*/) override { }
  virtual void exitClosedAdditiveExpression(RxParser::ClosedAdditiveExpressionContext * /*ctx*/) override { }

  virtual void enterMultiplicativeExpression(RxParser::MultiplicativeExpressionContext * /*ctx*/) override { }
  virtual void exitMultiplicativeExpression(RxParser::MultiplicativeExpressionContext * /*ctx*/) override { }

  virtual void enterClosedMultiplicativeExpression(RxParser::ClosedMultiplicativeExpressionContext * /*ctx*/) override { }
  virtual void exitClosedMultiplicativeExpression(RxParser::ClosedMultiplicativeExpressionContext * /*ctx*/) override { }

  virtual void enterCastExpression(RxParser::CastExpressionContext * /*ctx*/) override { }
  virtual void exitCastExpression(RxParser::CastExpressionContext * /*ctx*/) override { }

  virtual void enterClosedCastExpression(RxParser::ClosedCastExpressionContext * /*ctx*/) override { }
  virtual void exitClosedCastExpression(RxParser::ClosedCastExpressionContext * /*ctx*/) override { }

  virtual void enterUnaryExpression(RxParser::UnaryExpressionContext * /*ctx*/) override { }
  virtual void exitUnaryExpression(RxParser::UnaryExpressionContext * /*ctx*/) override { }

  virtual void enterPostfixExpression(RxParser::PostfixExpressionContext * /*ctx*/) override { }
  virtual void exitPostfixExpression(RxParser::PostfixExpressionContext * /*ctx*/) override { }

  virtual void enterConditionExpression(RxParser::ConditionExpressionContext * /*ctx*/) override { }
  virtual void exitConditionExpression(RxParser::ConditionExpressionContext * /*ctx*/) override { }

  virtual void enterConditionAssignmentExpression(RxParser::ConditionAssignmentExpressionContext * /*ctx*/) override { }
  virtual void exitConditionAssignmentExpression(RxParser::ConditionAssignmentExpressionContext * /*ctx*/) override { }

  virtual void enterConditionLogicalOrExpression(RxParser::ConditionLogicalOrExpressionContext * /*ctx*/) override { }
  virtual void exitConditionLogicalOrExpression(RxParser::ConditionLogicalOrExpressionContext * /*ctx*/) override { }

  virtual void enterConditionLogicalAndExpression(RxParser::ConditionLogicalAndExpressionContext * /*ctx*/) override { }
  virtual void exitConditionLogicalAndExpression(RxParser::ConditionLogicalAndExpressionContext * /*ctx*/) override { }

  virtual void enterConditionComparisonExpression(RxParser::ConditionComparisonExpressionContext * /*ctx*/) override { }
  virtual void exitConditionComparisonExpression(RxParser::ConditionComparisonExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBitOrExpression(RxParser::ConditionBitOrExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBitOrExpression(RxParser::ConditionBitOrExpressionContext * /*ctx*/) override { }

  virtual void enterConditionClosedBitOrExpression(RxParser::ConditionClosedBitOrExpressionContext * /*ctx*/) override { }
  virtual void exitConditionClosedBitOrExpression(RxParser::ConditionClosedBitOrExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBitXorExpression(RxParser::ConditionBitXorExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBitXorExpression(RxParser::ConditionBitXorExpressionContext * /*ctx*/) override { }

  virtual void enterConditionClosedBitXorExpression(RxParser::ConditionClosedBitXorExpressionContext * /*ctx*/) override { }
  virtual void exitConditionClosedBitXorExpression(RxParser::ConditionClosedBitXorExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBitAndExpression(RxParser::ConditionBitAndExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBitAndExpression(RxParser::ConditionBitAndExpressionContext * /*ctx*/) override { }

  virtual void enterConditionClosedBitAndExpression(RxParser::ConditionClosedBitAndExpressionContext * /*ctx*/) override { }
  virtual void exitConditionClosedBitAndExpression(RxParser::ConditionClosedBitAndExpressionContext * /*ctx*/) override { }

  virtual void enterConditionShiftExpression(RxParser::ConditionShiftExpressionContext * /*ctx*/) override { }
  virtual void exitConditionShiftExpression(RxParser::ConditionShiftExpressionContext * /*ctx*/) override { }

  virtual void enterConditionClosedShiftExpression(RxParser::ConditionClosedShiftExpressionContext * /*ctx*/) override { }
  virtual void exitConditionClosedShiftExpression(RxParser::ConditionClosedShiftExpressionContext * /*ctx*/) override { }

  virtual void enterConditionAdditiveExpression(RxParser::ConditionAdditiveExpressionContext * /*ctx*/) override { }
  virtual void exitConditionAdditiveExpression(RxParser::ConditionAdditiveExpressionContext * /*ctx*/) override { }

  virtual void enterConditionClosedAdditiveExpression(RxParser::ConditionClosedAdditiveExpressionContext * /*ctx*/) override { }
  virtual void exitConditionClosedAdditiveExpression(RxParser::ConditionClosedAdditiveExpressionContext * /*ctx*/) override { }

  virtual void enterConditionMultiplicativeExpression(RxParser::ConditionMultiplicativeExpressionContext * /*ctx*/) override { }
  virtual void exitConditionMultiplicativeExpression(RxParser::ConditionMultiplicativeExpressionContext * /*ctx*/) override { }

  virtual void enterConditionClosedMultiplicativeExpression(RxParser::ConditionClosedMultiplicativeExpressionContext * /*ctx*/) override { }
  virtual void exitConditionClosedMultiplicativeExpression(RxParser::ConditionClosedMultiplicativeExpressionContext * /*ctx*/) override { }

  virtual void enterConditionCastExpression(RxParser::ConditionCastExpressionContext * /*ctx*/) override { }
  virtual void exitConditionCastExpression(RxParser::ConditionCastExpressionContext * /*ctx*/) override { }

  virtual void enterConditionClosedCastExpression(RxParser::ConditionClosedCastExpressionContext * /*ctx*/) override { }
  virtual void exitConditionClosedCastExpression(RxParser::ConditionClosedCastExpressionContext * /*ctx*/) override { }

  virtual void enterConditionUnaryExpression(RxParser::ConditionUnaryExpressionContext * /*ctx*/) override { }
  virtual void exitConditionUnaryExpression(RxParser::ConditionUnaryExpressionContext * /*ctx*/) override { }

  virtual void enterConditionPostfixExpression(RxParser::ConditionPostfixExpressionContext * /*ctx*/) override { }
  virtual void exitConditionPostfixExpression(RxParser::ConditionPostfixExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakExpression(RxParser::ConditionBreakExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakExpression(RxParser::ConditionBreakExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakAssignmentExpression(RxParser::ConditionBreakAssignmentExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakAssignmentExpression(RxParser::ConditionBreakAssignmentExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakLogicalOrExpression(RxParser::ConditionBreakLogicalOrExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakLogicalOrExpression(RxParser::ConditionBreakLogicalOrExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakLogicalAndExpression(RxParser::ConditionBreakLogicalAndExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakLogicalAndExpression(RxParser::ConditionBreakLogicalAndExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakComparisonExpression(RxParser::ConditionBreakComparisonExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakComparisonExpression(RxParser::ConditionBreakComparisonExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakBitOrExpression(RxParser::ConditionBreakBitOrExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakBitOrExpression(RxParser::ConditionBreakBitOrExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakClosedBitOrExpression(RxParser::ConditionBreakClosedBitOrExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakClosedBitOrExpression(RxParser::ConditionBreakClosedBitOrExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakBitXorExpression(RxParser::ConditionBreakBitXorExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakBitXorExpression(RxParser::ConditionBreakBitXorExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakClosedBitXorExpression(RxParser::ConditionBreakClosedBitXorExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakClosedBitXorExpression(RxParser::ConditionBreakClosedBitXorExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakBitAndExpression(RxParser::ConditionBreakBitAndExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakBitAndExpression(RxParser::ConditionBreakBitAndExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakClosedBitAndExpression(RxParser::ConditionBreakClosedBitAndExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakClosedBitAndExpression(RxParser::ConditionBreakClosedBitAndExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakShiftExpression(RxParser::ConditionBreakShiftExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakShiftExpression(RxParser::ConditionBreakShiftExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakClosedShiftExpression(RxParser::ConditionBreakClosedShiftExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakClosedShiftExpression(RxParser::ConditionBreakClosedShiftExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakAdditiveExpression(RxParser::ConditionBreakAdditiveExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakAdditiveExpression(RxParser::ConditionBreakAdditiveExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakClosedAdditiveExpression(RxParser::ConditionBreakClosedAdditiveExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakClosedAdditiveExpression(RxParser::ConditionBreakClosedAdditiveExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakMultiplicativeExpression(RxParser::ConditionBreakMultiplicativeExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakMultiplicativeExpression(RxParser::ConditionBreakMultiplicativeExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakClosedMultiplicativeExpression(RxParser::ConditionBreakClosedMultiplicativeExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakClosedMultiplicativeExpression(RxParser::ConditionBreakClosedMultiplicativeExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakCastExpression(RxParser::ConditionBreakCastExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakCastExpression(RxParser::ConditionBreakCastExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakClosedCastExpression(RxParser::ConditionBreakClosedCastExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakClosedCastExpression(RxParser::ConditionBreakClosedCastExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakUnaryExpression(RxParser::ConditionBreakUnaryExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakUnaryExpression(RxParser::ConditionBreakUnaryExpressionContext * /*ctx*/) override { }

  virtual void enterConditionBreakPostfixExpression(RxParser::ConditionBreakPostfixExpressionContext * /*ctx*/) override { }
  virtual void exitConditionBreakPostfixExpression(RxParser::ConditionBreakPostfixExpressionContext * /*ctx*/) override { }

  virtual void enterStatementExpression(RxParser::StatementExpressionContext * /*ctx*/) override { }
  virtual void exitStatementExpression(RxParser::StatementExpressionContext * /*ctx*/) override { }

  virtual void enterStatementAssignmentExpression(RxParser::StatementAssignmentExpressionContext * /*ctx*/) override { }
  virtual void exitStatementAssignmentExpression(RxParser::StatementAssignmentExpressionContext * /*ctx*/) override { }

  virtual void enterStatementLogicalOrExpression(RxParser::StatementLogicalOrExpressionContext * /*ctx*/) override { }
  virtual void exitStatementLogicalOrExpression(RxParser::StatementLogicalOrExpressionContext * /*ctx*/) override { }

  virtual void enterStatementLogicalAndExpression(RxParser::StatementLogicalAndExpressionContext * /*ctx*/) override { }
  virtual void exitStatementLogicalAndExpression(RxParser::StatementLogicalAndExpressionContext * /*ctx*/) override { }

  virtual void enterStatementComparisonExpression(RxParser::StatementComparisonExpressionContext * /*ctx*/) override { }
  virtual void exitStatementComparisonExpression(RxParser::StatementComparisonExpressionContext * /*ctx*/) override { }

  virtual void enterStatementBitOrExpression(RxParser::StatementBitOrExpressionContext * /*ctx*/) override { }
  virtual void exitStatementBitOrExpression(RxParser::StatementBitOrExpressionContext * /*ctx*/) override { }

  virtual void enterStatementClosedBitOrExpression(RxParser::StatementClosedBitOrExpressionContext * /*ctx*/) override { }
  virtual void exitStatementClosedBitOrExpression(RxParser::StatementClosedBitOrExpressionContext * /*ctx*/) override { }

  virtual void enterStatementBitXorExpression(RxParser::StatementBitXorExpressionContext * /*ctx*/) override { }
  virtual void exitStatementBitXorExpression(RxParser::StatementBitXorExpressionContext * /*ctx*/) override { }

  virtual void enterStatementClosedBitXorExpression(RxParser::StatementClosedBitXorExpressionContext * /*ctx*/) override { }
  virtual void exitStatementClosedBitXorExpression(RxParser::StatementClosedBitXorExpressionContext * /*ctx*/) override { }

  virtual void enterStatementBitAndExpression(RxParser::StatementBitAndExpressionContext * /*ctx*/) override { }
  virtual void exitStatementBitAndExpression(RxParser::StatementBitAndExpressionContext * /*ctx*/) override { }

  virtual void enterStatementClosedBitAndExpression(RxParser::StatementClosedBitAndExpressionContext * /*ctx*/) override { }
  virtual void exitStatementClosedBitAndExpression(RxParser::StatementClosedBitAndExpressionContext * /*ctx*/) override { }

  virtual void enterStatementShiftExpression(RxParser::StatementShiftExpressionContext * /*ctx*/) override { }
  virtual void exitStatementShiftExpression(RxParser::StatementShiftExpressionContext * /*ctx*/) override { }

  virtual void enterStatementClosedShiftExpression(RxParser::StatementClosedShiftExpressionContext * /*ctx*/) override { }
  virtual void exitStatementClosedShiftExpression(RxParser::StatementClosedShiftExpressionContext * /*ctx*/) override { }

  virtual void enterStatementAdditiveExpression(RxParser::StatementAdditiveExpressionContext * /*ctx*/) override { }
  virtual void exitStatementAdditiveExpression(RxParser::StatementAdditiveExpressionContext * /*ctx*/) override { }

  virtual void enterStatementClosedAdditiveExpression(RxParser::StatementClosedAdditiveExpressionContext * /*ctx*/) override { }
  virtual void exitStatementClosedAdditiveExpression(RxParser::StatementClosedAdditiveExpressionContext * /*ctx*/) override { }

  virtual void enterStatementMultiplicativeExpression(RxParser::StatementMultiplicativeExpressionContext * /*ctx*/) override { }
  virtual void exitStatementMultiplicativeExpression(RxParser::StatementMultiplicativeExpressionContext * /*ctx*/) override { }

  virtual void enterStatementClosedMultiplicativeExpression(RxParser::StatementClosedMultiplicativeExpressionContext * /*ctx*/) override { }
  virtual void exitStatementClosedMultiplicativeExpression(RxParser::StatementClosedMultiplicativeExpressionContext * /*ctx*/) override { }

  virtual void enterStatementCastExpression(RxParser::StatementCastExpressionContext * /*ctx*/) override { }
  virtual void exitStatementCastExpression(RxParser::StatementCastExpressionContext * /*ctx*/) override { }

  virtual void enterStatementClosedCastExpression(RxParser::StatementClosedCastExpressionContext * /*ctx*/) override { }
  virtual void exitStatementClosedCastExpression(RxParser::StatementClosedCastExpressionContext * /*ctx*/) override { }

  virtual void enterStatementUnaryExpression(RxParser::StatementUnaryExpressionContext * /*ctx*/) override { }
  virtual void exitStatementUnaryExpression(RxParser::StatementUnaryExpressionContext * /*ctx*/) override { }

  virtual void enterStatementPostfixExpression(RxParser::StatementPostfixExpressionContext * /*ctx*/) override { }
  virtual void exitStatementPostfixExpression(RxParser::StatementPostfixExpressionContext * /*ctx*/) override { }

  virtual void enterPrimaryExpression(RxParser::PrimaryExpressionContext * /*ctx*/) override { }
  virtual void exitPrimaryExpression(RxParser::PrimaryExpressionContext * /*ctx*/) override { }

  virtual void enterNonBlockPrimary(RxParser::NonBlockPrimaryContext * /*ctx*/) override { }
  virtual void exitNonBlockPrimary(RxParser::NonBlockPrimaryContext * /*ctx*/) override { }

  virtual void enterConditionPrimary(RxParser::ConditionPrimaryContext * /*ctx*/) override { }
  virtual void exitConditionPrimary(RxParser::ConditionPrimaryContext * /*ctx*/) override { }

  virtual void enterConditionPrimaryWithoutBareBlock(RxParser::ConditionPrimaryWithoutBareBlockContext * /*ctx*/) override { }
  virtual void exitConditionPrimaryWithoutBareBlock(RxParser::ConditionPrimaryWithoutBareBlockContext * /*ctx*/) override { }

  virtual void enterLiteralExpression(RxParser::LiteralExpressionContext * /*ctx*/) override { }
  virtual void exitLiteralExpression(RxParser::LiteralExpressionContext * /*ctx*/) override { }

  virtual void enterStructExprFields(RxParser::StructExprFieldsContext * /*ctx*/) override { }
  virtual void exitStructExprFields(RxParser::StructExprFieldsContext * /*ctx*/) override { }

  virtual void enterStructExprField(RxParser::StructExprFieldContext * /*ctx*/) override { }
  virtual void exitStructExprField(RxParser::StructExprFieldContext * /*ctx*/) override { }

  virtual void enterArrayExpression(RxParser::ArrayExpressionContext * /*ctx*/) override { }
  virtual void exitArrayExpression(RxParser::ArrayExpressionContext * /*ctx*/) override { }

  virtual void enterPostfixSuffix(RxParser::PostfixSuffixContext * /*ctx*/) override { }
  virtual void exitPostfixSuffix(RxParser::PostfixSuffixContext * /*ctx*/) override { }

  virtual void enterDotSuffix(RxParser::DotSuffixContext * /*ctx*/) override { }
  virtual void exitDotSuffix(RxParser::DotSuffixContext * /*ctx*/) override { }

  virtual void enterCallArguments(RxParser::CallArgumentsContext * /*ctx*/) override { }
  virtual void exitCallArguments(RxParser::CallArgumentsContext * /*ctx*/) override { }

  virtual void enterUnaryOperator(RxParser::UnaryOperatorContext * /*ctx*/) override { }
  virtual void exitUnaryOperator(RxParser::UnaryOperatorContext * /*ctx*/) override { }

  virtual void enterMultiplicativeOperator(RxParser::MultiplicativeOperatorContext * /*ctx*/) override { }
  virtual void exitMultiplicativeOperator(RxParser::MultiplicativeOperatorContext * /*ctx*/) override { }

  virtual void enterAdditiveOperator(RxParser::AdditiveOperatorContext * /*ctx*/) override { }
  virtual void exitAdditiveOperator(RxParser::AdditiveOperatorContext * /*ctx*/) override { }

  virtual void enterShiftRight(RxParser::ShiftRightContext * /*ctx*/) override { }
  virtual void exitShiftRight(RxParser::ShiftRightContext * /*ctx*/) override { }

  virtual void enterComparisonExceptLt(RxParser::ComparisonExceptLtContext * /*ctx*/) override { }
  virtual void exitComparisonExceptLt(RxParser::ComparisonExceptLtContext * /*ctx*/) override { }

  virtual void enterAssignmentOperator(RxParser::AssignmentOperatorContext * /*ctx*/) override { }
  virtual void exitAssignmentOperator(RxParser::AssignmentOperatorContext * /*ctx*/) override { }

  virtual void enterEqualsSign(RxParser::EqualsSignContext * /*ctx*/) override { }
  virtual void exitEqualsSign(RxParser::EqualsSignContext * /*ctx*/) override { }

  virtual void enterIdentifier(RxParser::IdentifierContext * /*ctx*/) override { }
  virtual void exitIdentifier(RxParser::IdentifierContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

