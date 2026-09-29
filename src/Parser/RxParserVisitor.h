
// Generated from RxParser.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "RxParser.h"



/**
 * This class defines an abstract visitor for a parse tree
 * produced by RxParser.
 */
class  RxParserVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by RxParser.
   */
    virtual std::any visitCrate(RxParser::CrateContext *context) = 0;

    virtual std::any visitItem(RxParser::ItemContext *context) = 0;

    virtual std::any visitUseDeclaration(RxParser::UseDeclarationContext *context) = 0;

    virtual std::any visitUseTree(RxParser::UseTreeContext *context) = 0;

    virtual std::any visitUsePath(RxParser::UsePathContext *context) = 0;

    virtual std::any visitUsePathSegment(RxParser::UsePathSegmentContext *context) = 0;

    virtual std::any visitFunctionDefinition(RxParser::FunctionDefinitionContext *context) = 0;

    virtual std::any visitFunctionParameters(RxParser::FunctionParametersContext *context) = 0;

    virtual std::any visitSelfParam(RxParser::SelfParamContext *context) = 0;

    virtual std::any visitFunctionParam(RxParser::FunctionParamContext *context) = 0;

    virtual std::any visitStructDefinition(RxParser::StructDefinitionContext *context) = 0;

    virtual std::any visitStructField(RxParser::StructFieldContext *context) = 0;

    virtual std::any visitOuterAttribute(RxParser::OuterAttributeContext *context) = 0;

    virtual std::any visitDeriveName(RxParser::DeriveNameContext *context) = 0;

    virtual std::any visitConstantItem(RxParser::ConstantItemContext *context) = 0;

    virtual std::any visitInherentImpl(RxParser::InherentImplContext *context) = 0;

    virtual std::any visitAssociatedItem(RxParser::AssociatedItemContext *context) = 0;

    virtual std::any visitGenericParams(RxParser::GenericParamsContext *context) = 0;

    virtual std::any visitLifetimeParam(RxParser::LifetimeParamContext *context) = 0;

    virtual std::any visitLifetime(RxParser::LifetimeContext *context) = 0;

    virtual std::any visitLifetimeBounds(RxParser::LifetimeBoundsContext *context) = 0;

    virtual std::any visitTypeParamBounds(RxParser::TypeParamBoundsContext *context) = 0;

    virtual std::any visitWhereClause(RxParser::WhereClauseContext *context) = 0;

    virtual std::any visitWhereClauseItem(RxParser::WhereClauseItemContext *context) = 0;

    virtual std::any visitTypeRef(RxParser::TypeRefContext *context) = 0;

    virtual std::any visitReferenceType(RxParser::ReferenceTypeContext *context) = 0;

    virtual std::any visitArrayType(RxParser::ArrayTypeContext *context) = 0;

    virtual std::any visitTypePath(RxParser::TypePathContext *context) = 0;

    virtual std::any visitTypePathSegment(RxParser::TypePathSegmentContext *context) = 0;

    virtual std::any visitPathInExpression(RxParser::PathInExpressionContext *context) = 0;

    virtual std::any visitPathExprSegment(RxParser::PathExprSegmentContext *context) = 0;

    virtual std::any visitPathIdentSegment(RxParser::PathIdentSegmentContext *context) = 0;

    virtual std::any visitGenericArgs(RxParser::GenericArgsContext *context) = 0;

    virtual std::any visitGenericArg(RxParser::GenericArgContext *context) = 0;

    virtual std::any visitGenericClose(RxParser::GenericCloseContext *context) = 0;

    virtual std::any visitClosedCastType(RxParser::ClosedCastTypeContext *context) = 0;

    virtual std::any visitConstValue(RxParser::ConstValueContext *context) = 0;

    virtual std::any visitMagnitude(RxParser::MagnitudeContext *context) = 0;

    virtual std::any visitIdentifierBinding(RxParser::IdentifierBindingContext *context) = 0;

    virtual std::any visitLetStatement(RxParser::LetStatementContext *context) = 0;

    virtual std::any visitBlockExpression(RxParser::BlockExpressionContext *context) = 0;

    virtual std::any visitStatement(RxParser::StatementContext *context) = 0;

    virtual std::any visitExpressionWithBlock(RxParser::ExpressionWithBlockContext *context) = 0;

    virtual std::any visitIfExpression(RxParser::IfExpressionContext *context) = 0;

    virtual std::any visitExpression(RxParser::ExpressionContext *context) = 0;

    virtual std::any visitAssignmentExpression(RxParser::AssignmentExpressionContext *context) = 0;

    virtual std::any visitLogicalOrExpression(RxParser::LogicalOrExpressionContext *context) = 0;

    virtual std::any visitLogicalAndExpression(RxParser::LogicalAndExpressionContext *context) = 0;

    virtual std::any visitComparisonExpression(RxParser::ComparisonExpressionContext *context) = 0;

    virtual std::any visitBitOrExpression(RxParser::BitOrExpressionContext *context) = 0;

    virtual std::any visitClosedBitOrExpression(RxParser::ClosedBitOrExpressionContext *context) = 0;

    virtual std::any visitBitXorExpression(RxParser::BitXorExpressionContext *context) = 0;

    virtual std::any visitClosedBitXorExpression(RxParser::ClosedBitXorExpressionContext *context) = 0;

    virtual std::any visitBitAndExpression(RxParser::BitAndExpressionContext *context) = 0;

    virtual std::any visitClosedBitAndExpression(RxParser::ClosedBitAndExpressionContext *context) = 0;

    virtual std::any visitShiftExpression(RxParser::ShiftExpressionContext *context) = 0;

    virtual std::any visitClosedShiftExpression(RxParser::ClosedShiftExpressionContext *context) = 0;

    virtual std::any visitAdditiveExpression(RxParser::AdditiveExpressionContext *context) = 0;

    virtual std::any visitClosedAdditiveExpression(RxParser::ClosedAdditiveExpressionContext *context) = 0;

    virtual std::any visitMultiplicativeExpression(RxParser::MultiplicativeExpressionContext *context) = 0;

    virtual std::any visitClosedMultiplicativeExpression(RxParser::ClosedMultiplicativeExpressionContext *context) = 0;

    virtual std::any visitCastExpression(RxParser::CastExpressionContext *context) = 0;

    virtual std::any visitClosedCastExpression(RxParser::ClosedCastExpressionContext *context) = 0;

    virtual std::any visitUnaryExpression(RxParser::UnaryExpressionContext *context) = 0;

    virtual std::any visitPostfixExpression(RxParser::PostfixExpressionContext *context) = 0;

    virtual std::any visitConditionExpression(RxParser::ConditionExpressionContext *context) = 0;

    virtual std::any visitConditionAssignmentExpression(RxParser::ConditionAssignmentExpressionContext *context) = 0;

    virtual std::any visitConditionLogicalOrExpression(RxParser::ConditionLogicalOrExpressionContext *context) = 0;

    virtual std::any visitConditionLogicalAndExpression(RxParser::ConditionLogicalAndExpressionContext *context) = 0;

    virtual std::any visitConditionComparisonExpression(RxParser::ConditionComparisonExpressionContext *context) = 0;

    virtual std::any visitConditionBitOrExpression(RxParser::ConditionBitOrExpressionContext *context) = 0;

    virtual std::any visitConditionClosedBitOrExpression(RxParser::ConditionClosedBitOrExpressionContext *context) = 0;

    virtual std::any visitConditionBitXorExpression(RxParser::ConditionBitXorExpressionContext *context) = 0;

    virtual std::any visitConditionClosedBitXorExpression(RxParser::ConditionClosedBitXorExpressionContext *context) = 0;

    virtual std::any visitConditionBitAndExpression(RxParser::ConditionBitAndExpressionContext *context) = 0;

    virtual std::any visitConditionClosedBitAndExpression(RxParser::ConditionClosedBitAndExpressionContext *context) = 0;

    virtual std::any visitConditionShiftExpression(RxParser::ConditionShiftExpressionContext *context) = 0;

    virtual std::any visitConditionClosedShiftExpression(RxParser::ConditionClosedShiftExpressionContext *context) = 0;

    virtual std::any visitConditionAdditiveExpression(RxParser::ConditionAdditiveExpressionContext *context) = 0;

    virtual std::any visitConditionClosedAdditiveExpression(RxParser::ConditionClosedAdditiveExpressionContext *context) = 0;

    virtual std::any visitConditionMultiplicativeExpression(RxParser::ConditionMultiplicativeExpressionContext *context) = 0;

    virtual std::any visitConditionClosedMultiplicativeExpression(RxParser::ConditionClosedMultiplicativeExpressionContext *context) = 0;

    virtual std::any visitConditionCastExpression(RxParser::ConditionCastExpressionContext *context) = 0;

    virtual std::any visitConditionClosedCastExpression(RxParser::ConditionClosedCastExpressionContext *context) = 0;

    virtual std::any visitConditionUnaryExpression(RxParser::ConditionUnaryExpressionContext *context) = 0;

    virtual std::any visitConditionPostfixExpression(RxParser::ConditionPostfixExpressionContext *context) = 0;

    virtual std::any visitConditionBreakExpression(RxParser::ConditionBreakExpressionContext *context) = 0;

    virtual std::any visitConditionBreakAssignmentExpression(RxParser::ConditionBreakAssignmentExpressionContext *context) = 0;

    virtual std::any visitConditionBreakLogicalOrExpression(RxParser::ConditionBreakLogicalOrExpressionContext *context) = 0;

    virtual std::any visitConditionBreakLogicalAndExpression(RxParser::ConditionBreakLogicalAndExpressionContext *context) = 0;

    virtual std::any visitConditionBreakComparisonExpression(RxParser::ConditionBreakComparisonExpressionContext *context) = 0;

    virtual std::any visitConditionBreakBitOrExpression(RxParser::ConditionBreakBitOrExpressionContext *context) = 0;

    virtual std::any visitConditionBreakClosedBitOrExpression(RxParser::ConditionBreakClosedBitOrExpressionContext *context) = 0;

    virtual std::any visitConditionBreakBitXorExpression(RxParser::ConditionBreakBitXorExpressionContext *context) = 0;

    virtual std::any visitConditionBreakClosedBitXorExpression(RxParser::ConditionBreakClosedBitXorExpressionContext *context) = 0;

    virtual std::any visitConditionBreakBitAndExpression(RxParser::ConditionBreakBitAndExpressionContext *context) = 0;

    virtual std::any visitConditionBreakClosedBitAndExpression(RxParser::ConditionBreakClosedBitAndExpressionContext *context) = 0;

    virtual std::any visitConditionBreakShiftExpression(RxParser::ConditionBreakShiftExpressionContext *context) = 0;

    virtual std::any visitConditionBreakClosedShiftExpression(RxParser::ConditionBreakClosedShiftExpressionContext *context) = 0;

    virtual std::any visitConditionBreakAdditiveExpression(RxParser::ConditionBreakAdditiveExpressionContext *context) = 0;

    virtual std::any visitConditionBreakClosedAdditiveExpression(RxParser::ConditionBreakClosedAdditiveExpressionContext *context) = 0;

    virtual std::any visitConditionBreakMultiplicativeExpression(RxParser::ConditionBreakMultiplicativeExpressionContext *context) = 0;

    virtual std::any visitConditionBreakClosedMultiplicativeExpression(RxParser::ConditionBreakClosedMultiplicativeExpressionContext *context) = 0;

    virtual std::any visitConditionBreakCastExpression(RxParser::ConditionBreakCastExpressionContext *context) = 0;

    virtual std::any visitConditionBreakClosedCastExpression(RxParser::ConditionBreakClosedCastExpressionContext *context) = 0;

    virtual std::any visitConditionBreakUnaryExpression(RxParser::ConditionBreakUnaryExpressionContext *context) = 0;

    virtual std::any visitConditionBreakPostfixExpression(RxParser::ConditionBreakPostfixExpressionContext *context) = 0;

    virtual std::any visitStatementExpression(RxParser::StatementExpressionContext *context) = 0;

    virtual std::any visitStatementAssignmentExpression(RxParser::StatementAssignmentExpressionContext *context) = 0;

    virtual std::any visitStatementLogicalOrExpression(RxParser::StatementLogicalOrExpressionContext *context) = 0;

    virtual std::any visitStatementLogicalAndExpression(RxParser::StatementLogicalAndExpressionContext *context) = 0;

    virtual std::any visitStatementComparisonExpression(RxParser::StatementComparisonExpressionContext *context) = 0;

    virtual std::any visitStatementBitOrExpression(RxParser::StatementBitOrExpressionContext *context) = 0;

    virtual std::any visitStatementClosedBitOrExpression(RxParser::StatementClosedBitOrExpressionContext *context) = 0;

    virtual std::any visitStatementBitXorExpression(RxParser::StatementBitXorExpressionContext *context) = 0;

    virtual std::any visitStatementClosedBitXorExpression(RxParser::StatementClosedBitXorExpressionContext *context) = 0;

    virtual std::any visitStatementBitAndExpression(RxParser::StatementBitAndExpressionContext *context) = 0;

    virtual std::any visitStatementClosedBitAndExpression(RxParser::StatementClosedBitAndExpressionContext *context) = 0;

    virtual std::any visitStatementShiftExpression(RxParser::StatementShiftExpressionContext *context) = 0;

    virtual std::any visitStatementClosedShiftExpression(RxParser::StatementClosedShiftExpressionContext *context) = 0;

    virtual std::any visitStatementAdditiveExpression(RxParser::StatementAdditiveExpressionContext *context) = 0;

    virtual std::any visitStatementClosedAdditiveExpression(RxParser::StatementClosedAdditiveExpressionContext *context) = 0;

    virtual std::any visitStatementMultiplicativeExpression(RxParser::StatementMultiplicativeExpressionContext *context) = 0;

    virtual std::any visitStatementClosedMultiplicativeExpression(RxParser::StatementClosedMultiplicativeExpressionContext *context) = 0;

    virtual std::any visitStatementCastExpression(RxParser::StatementCastExpressionContext *context) = 0;

    virtual std::any visitStatementClosedCastExpression(RxParser::StatementClosedCastExpressionContext *context) = 0;

    virtual std::any visitStatementUnaryExpression(RxParser::StatementUnaryExpressionContext *context) = 0;

    virtual std::any visitStatementPostfixExpression(RxParser::StatementPostfixExpressionContext *context) = 0;

    virtual std::any visitPrimaryExpression(RxParser::PrimaryExpressionContext *context) = 0;

    virtual std::any visitNonBlockPrimary(RxParser::NonBlockPrimaryContext *context) = 0;

    virtual std::any visitConditionPrimary(RxParser::ConditionPrimaryContext *context) = 0;

    virtual std::any visitConditionPrimaryWithoutBareBlock(RxParser::ConditionPrimaryWithoutBareBlockContext *context) = 0;

    virtual std::any visitLiteralExpression(RxParser::LiteralExpressionContext *context) = 0;

    virtual std::any visitStructExprFields(RxParser::StructExprFieldsContext *context) = 0;

    virtual std::any visitStructExprField(RxParser::StructExprFieldContext *context) = 0;

    virtual std::any visitArrayExpression(RxParser::ArrayExpressionContext *context) = 0;

    virtual std::any visitPostfixSuffix(RxParser::PostfixSuffixContext *context) = 0;

    virtual std::any visitDotSuffix(RxParser::DotSuffixContext *context) = 0;

    virtual std::any visitCallArguments(RxParser::CallArgumentsContext *context) = 0;

    virtual std::any visitUnaryOperator(RxParser::UnaryOperatorContext *context) = 0;

    virtual std::any visitMultiplicativeOperator(RxParser::MultiplicativeOperatorContext *context) = 0;

    virtual std::any visitAdditiveOperator(RxParser::AdditiveOperatorContext *context) = 0;

    virtual std::any visitShiftRight(RxParser::ShiftRightContext *context) = 0;

    virtual std::any visitComparisonExceptLt(RxParser::ComparisonExceptLtContext *context) = 0;

    virtual std::any visitAssignmentOperator(RxParser::AssignmentOperatorContext *context) = 0;

    virtual std::any visitEqualsSign(RxParser::EqualsSignContext *context) = 0;

    virtual std::any visitIdentifier(RxParser::IdentifierContext *context) = 0;


};

