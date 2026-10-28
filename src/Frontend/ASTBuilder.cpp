#include "ASTBuilder.hpp"
#include "AST.hpp"
#include "RxParser.h"
#include "TerminalNode.h"
#include "Token.h"
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace helper {
position getPosition(antlr4::Token *tok) {
  std::size_t startLine = tok->getLine();
  std::size_t startColumn = tok->getCharPositionInLine();
  std::size_t endLine = startLine;
  std::size_t endColumn = tok->getCharPositionInLine();
  if (tok->getType() != antlr4::Token::EOF) {
    endColumn += tok->getStopIndex() - tok->getStartIndex() + 1;
  }
  return {startLine, startColumn, endLine, endColumn};
}
position getPosition(antlr4::Token *first, antlr4::Token *last) {
  auto firstTokenPos = getPosition(first);
  auto lastTokenPos = getPosition(last);
  return {firstTokenPos.startLine, firstTokenPos.startColumn,
          lastTokenPos.endLine, lastTokenPos.endColumn};
}
position getPosition(antlr4::tree::TerminalNode *terminal) {
  return getPosition(terminal->getSymbol());
}
position getPosition(antlr4::ParserRuleContext *ctx) {
  return getPosition(ctx->getStart(), ctx->getStop());
}
} // namespace helper

AssignmentOperator
ASTBuilder::getAssignmentOperator(RxParser::AssignmentOperatorContext *ctx) {
  if (ctx->equalsSign()) {
    return AssignmentOperator::Assignment;
  } else if (ctx->PLUS_ASSIGN()) {
    return AssignmentOperator::additionAssignment;
  } else if (ctx->MINUS_ASSIGN()) {
    return AssignmentOperator::subtractionAssignment;
  } else if (ctx->STAR_ASSIGN()) {
    return AssignmentOperator::multiplicationAssignment;
  } else if (ctx->SLASH_ASSIGN()) {
    return AssignmentOperator::divisionAssignment;
  } else if (ctx->PERCENT_ASSIGN()) {
    return AssignmentOperator::moduloAssignment;
  } else if (ctx->AMP_ASSIGN()) {
    return AssignmentOperator::ANDAssignment;
  } else if (ctx->PIPE_ASSIGN()) {
    return AssignmentOperator::ORAssignment;
  } else if (ctx->CARET_ASSIGN()) {
    return AssignmentOperator::XORAssignment;
  } else if (ctx->SHL_ASSIGN()) {
    return AssignmentOperator::leftShiftAssignment;
  } else if (ctx->GT() && ctx->GT_SECOND() && ctx->SHR_EQ()) {
    // >>= is represented by three pieces of one punctuation token.
    return AssignmentOperator::rightShiftAssignment;
  }
  throw std::logic_error("Internal AST error: unsupported assignment operator `" +
                         ctx->getText() + "`");
}

ComparisonOperator
ASTBuilder::getComparisonOperator(RxParser::ComparisonExceptLtContext *ctx) {
  if (ctx->EQEQ()) {
    return ComparisonOperator::equal;
  } else if ((ctx->GT() && ctx->GE_EQ()) ||
             (ctx->GT_SECOND() && ctx->SHR_EQ())) {
    // >= can also be the remaining pieces of >>= after a generic close.
    return ComparisonOperator::greaterThanOrEqual;
  } else if (ctx->genericClose()) {
    return ComparisonOperator::greaterThan;
  } else if (ctx->LE()) {
    return ComparisonOperator::lessThanOrEqual;
  } else if (ctx->NE()) {
    return ComparisonOperator::notEqual;
  }
  throw std::logic_error("Internal AST error: unsupported comparison operator `" +
                         ctx->getText() + "`");
}

std::unique_ptr<CrateNode> ASTBuilder::visitCrate(RxParser::CrateContext *ctx) {
  auto crate = std::make_unique<CrateNode>(helper::getPosition(ctx));
  for (const auto &item : ctx->item()) {
    auto node = visitItem(item);
    if (node) {
      crate->items.push_back(std::move(node));
    }
  }
  return crate;
}

std::unique_ptr<ItemNode> ASTBuilder::visitItem(RxParser::ItemContext *ctx) {
  if (auto *fn = ctx->functionDefinition()) {
    return visitFunctionDeclItem(fn);
  } else if (auto *structure = ctx->structDefinition()) {
    return visitStructDeclItem(structure);
  } else if (auto *constant = ctx->constantItem()) {
    return visitConstantDeclItem(constant);
  } else if (auto *impl = ctx->inherentImpl()) {
    return visitImplDeclItem(impl);
  } else
    return nullptr;
}

std::unique_ptr<FunctionDeclItemNode>
ASTBuilder::visitFunctionDeclItem(RxParser::FunctionDefinitionContext *ctx) {
  const std::string &funcName = ctx->identifier()->getText();
  position funcPos = helper::getPosition(ctx);
  auto funcNode = std::make_unique<FunctionDeclItemNode>(
      funcPos, funcName, visitBlockExprNode(ctx->blockExpression()));
  if (auto *params = ctx->genericParams()) {
    funcNode->args.hasLifeTime = !params->lifetimeParam().empty();
  }
  if (ctx->functionParameters() != nullptr) {
    auto selfParam = ctx->functionParameters()->selfParam();
    if (selfParam != nullptr) {
      funcNode->selfParam =
          SelfParam{selfParam->AMP() != nullptr, selfParam->MUT() != nullptr};
    }
    funcNode->functionParam.resize(
        ctx->functionParameters()->functionParam().size());
    std::size_t cursor = 0;
    for (const auto &param : ctx->functionParameters()->functionParam()) {
      funcNode->functionParam[cursor].isMut =
          param->identifierBinding()->MUT() != nullptr;
      funcNode->functionParam[cursor].identifier =
          param->identifierBinding()->identifier()->getText();
      funcNode->functionParam[cursor].paramType =
          std::move(visitTypeNode(param->typeRef()));
      ++cursor;
    }
  }
  auto type = ctx->typeRef();
  if (type != nullptr) {
    funcNode->returnType = std::move(visitTypeNode(type));
  }
  return funcNode;
}

std::unique_ptr<OuterAttributeNode>
ASTBuilder::visitOuterAttributeNode(RxParser::OuterAttributeContext *ctx) {
  std::vector<Attribute> attributes;
  for (const auto &attribute : ctx->deriveName()) {
    if (attribute->EQ()) {
      attributes.push_back(Attribute::Eq);
    } else if (attribute->PARTIAL_EQ()) {
      attributes.push_back(Attribute::PartialEq);
    } else if (attribute->CLONE()) {
      attributes.push_back(Attribute::Clone);
    } else {
      attributes.push_back(Attribute::Copy);
    }
  }
  position outerAttributePos = helper::getPosition(ctx);
  return std::make_unique<OuterAttributeNode>(outerAttributePos,
                                              std::move(attributes));
}

std::unique_ptr<ItemNode>
ASTBuilder::visitStructDeclItem(RxParser::StructDefinitionContext *ctx) {
  const std::string &structName = ctx->identifier()->getText();
  position structPos = helper::getPosition(ctx);
  auto structNode = std::make_unique<StructDeclItemNode>(structPos, structName);
  for (const auto &outAttribute : ctx->outerAttribute()) {
    structNode->outerAttribute.push_back(visitOuterAttributeNode(outAttribute));
  }
  structNode->structFields.resize(ctx->structField().size());
  std::size_t cursor = 0;
  for (const auto &structField : ctx->structField()) {
    structNode->structFields[cursor].identifier =
        structField->identifier()->getText();
    structNode->structFields[cursor].fieldType =
        visitTypeNode(structField->typeRef());
    ++cursor;
  }
  return structNode;
}

std::unique_ptr<ConstantDeclItemNode>
ASTBuilder::visitConstantDeclItem(RxParser::ConstantItemContext *ctx) {
  return std::make_unique<ConstantDeclItemNode>(
      helper::getPosition(ctx), ctx->identifier()->getText(),
      visitTypeNode(ctx->typeRef()), visitConstExprNode(ctx->constValue()));
}

std::unique_ptr<ItemNode>
ASTBuilder::visitImplDeclItem(RxParser::InherentImplContext *ctx) {
  auto implNode = std::make_unique<ImplDeclItemNode>(helper::getPosition(ctx));
  implNode->implType = visitTypeNode(ctx->typeRef());
  for (const auto &associateItem : ctx->associatedItem()) {
    if (associateItem->constantItem()) {
      implNode->associatedItems.push_back(
          visitConstantDeclItem(associateItem->constantItem()));
    } else {
      implNode->associatedItems.push_back(
          visitFunctionDeclItem(associateItem->functionDefinition()));
    }
  }
  return implNode;
}

std::unique_ptr<TypeNode>
ASTBuilder::visitTypeNode(RxParser::TypeRefContext *ctx) {
  if (ctx->typeRef()) {
    return visitTypeNode(ctx->typeRef());
  } else if (ctx->arrayType()) {
    return visitArrayTypeNode(ctx->arrayType());
  } else if (ctx->referenceType()) {
    return visitReferenceTypeNode(ctx->referenceType());
  } else if (ctx->typePath()) {
    return visitPathInTypeNode(ctx->typePath());
  } else if (ctx->LPAREN() && ctx->RPAREN()) {
    return std::make_unique<UnitTypeNode>(helper::getPosition(ctx));
  }
  return nullptr;
}

std::unique_ptr<TypeNode>
ASTBuilder::visitArrayTypeNode(RxParser::ArrayTypeContext *ctx) {
  return std::make_unique<ArrayTypeNode>(helper::getPosition(ctx),
                                         visitTypeNode(ctx->typeRef()),
                                         visitConstExprNode(ctx->constValue()));
}

std::unique_ptr<TypeNode>
ASTBuilder::visitReferenceTypeNode(RxParser::ReferenceTypeContext *ctx) {
  if (ctx->AMP()) {
    return std::make_unique<ReferenceTypeNode>(helper::getPosition(ctx),
                                               ctx->MUT() != nullptr,
                                               visitTypeNode(ctx->typeRef()));
  } else if (ctx->ANDAND()) {
    const auto outerPos = helper::getPosition(ctx);
    const auto ampPos = helper::getPosition(ctx->ANDAND());
    auto innerPos = outerPos;
    innerPos.startLine = ampPos.startLine;
    innerPos.startColumn = ampPos.startColumn + 1;
    return std::make_unique<ReferenceTypeNode>(
        outerPos, false,
        std::make_unique<ReferenceTypeNode>(innerPos, ctx->MUT() != nullptr,
                                          visitTypeNode(ctx->typeRef())));
  }
  return nullptr;
}

GenericArgs
ASTBuilder::visitGenericArgsNode(RxParser::GenericArgsContext *ctx) {
  GenericArgs args;
  if (ctx) {
    for (auto *arg : ctx->genericArg()) {
      if (arg->lifetime()) {
        args.hasLifeTime = true;
      } else if (arg->typeRef()) {
        args.genericArgs.emplace_back(visitTypeNode(arg->typeRef()));
      }
    }
  }
  return args;
}

std::unique_ptr<TypeNode>
ASTBuilder::visitPathInTypeNode(RxParser::TypePathContext *ctx) {
  auto typePath = std::make_unique<PathInTypeNode>(helper::getPosition(ctx));
  for (const auto &segment : ctx->typePathSegment()) {
    PathSegment seg;
    if (segment->pathIdentSegment()->identifier()) {
      seg.pathIdentSegment =
          segment->pathIdentSegment()->identifier()->getText();
    } else if (segment->pathIdentSegment()->SELF_TYPE()) {
      seg.pathIdentSegment = "Self";
    } else if (segment->pathIdentSegment()->SELF_VALUE()) {
      seg.pathIdentSegment = "self";
    }
    seg.args = visitGenericArgsNode(segment->genericArgs());
    typePath->path.segments.push_back(std::move(seg));
  }
  return typePath;
}

std::unique_ptr<PathInExprNode>
ASTBuilder::visitPathInExprNode(RxParser::PathInExpressionContext *ctx) {
  auto pathInExpr = std::make_unique<PathInExprNode>(helper::getPosition(ctx));
  for (auto *segment : ctx->pathExprSegment()) {
    PathSegment seg;
    if (segment->pathIdentSegment()->identifier()) {
      seg.pathIdentSegment =
          segment->pathIdentSegment()->identifier()->getText();
    } else if (segment->pathIdentSegment()->SELF_TYPE()) {
      seg.pathIdentSegment = "Self";
    } else if (segment->pathIdentSegment()->SELF_VALUE()) {
      seg.pathIdentSegment = "self";
    }
    seg.args = visitGenericArgsNode(segment->genericArgs());
    pathInExpr->path.segments.push_back(std::move(seg));
  }
  return pathInExpr;
};

std::unique_ptr<ExprNode>
ASTBuilder::visitExprNode(RxParser::ExpressionContext *ctx) {
  if (ctx->assignmentExpression() != nullptr) {
    return visitAssignmentExprNode(ctx->assignmentExpression());
  }
  return nullptr;
}

std::unique_ptr<ExprNode>
ASTBuilder::visitExprNode(RxParser::StatementExpressionContext *ctx) {
  if (ctx->statementAssignmentExpression() != nullptr) {
    return visitAssignmentExprNode(ctx->statementAssignmentExpression());
  }
  return nullptr;
}

std::unique_ptr<ExprNode>
ASTBuilder::visitExprNode(RxParser::ConditionExpressionContext *ctx) {
  if (ctx->conditionAssignmentExpression() != nullptr) {
    return visitAssignmentExprNode(ctx->conditionAssignmentExpression());
  }
  return nullptr;
}

std::unique_ptr<ExprNode>
ASTBuilder::visitExprNode(RxParser::ConditionBreakExpressionContext *ctx) {
  if (ctx->conditionBreakAssignmentExpression() != nullptr) {
    return visitAssignmentExprNode(ctx->conditionBreakAssignmentExpression());
  }
  return nullptr;
}

std::unique_ptr<ExprNode> ASTBuilder::visitAssignmentExprNode(
    RxParser::AssignmentExpressionContext *ctx) {
  if (ctx->assignmentOperator() == nullptr) {
    return visitLogicalOrExprNode(ctx->logicalOrExpression());
  }

  return std::make_unique<AssignmentExprNode>(
      helper::getPosition(ctx),
      getAssignmentOperator(ctx->assignmentOperator()),
      visitLogicalOrExprNode(ctx->logicalOrExpression()),
      visitExprNode(ctx->expression()));
}

std::unique_ptr<ExprNode> ASTBuilder::visitAssignmentExprNode(
    RxParser::StatementAssignmentExpressionContext *ctx) {
  if (ctx->assignmentOperator() == nullptr) {
    return visitLogicalOrExprNode(ctx->statementLogicalOrExpression());
  }

  return std::make_unique<AssignmentExprNode>(
      helper::getPosition(ctx),
      getAssignmentOperator(ctx->assignmentOperator()),
      visitLogicalOrExprNode(ctx->statementLogicalOrExpression()),
      visitExprNode(ctx->expression()));
}

std::unique_ptr<ExprNode> ASTBuilder::visitAssignmentExprNode(
    RxParser::ConditionAssignmentExpressionContext *ctx) {
  if (ctx->assignmentOperator() == nullptr) {
    return visitLogicalOrExprNode(ctx->conditionLogicalOrExpression());
  }

  return std::make_unique<AssignmentExprNode>(
      helper::getPosition(ctx),
      getAssignmentOperator(ctx->assignmentOperator()),
      visitLogicalOrExprNode(ctx->conditionLogicalOrExpression()),
      visitExprNode(ctx->conditionExpression()));
}

std::unique_ptr<ExprNode> ASTBuilder::visitAssignmentExprNode(
    RxParser::ConditionBreakAssignmentExpressionContext *ctx) {
  if (ctx->assignmentOperator() == nullptr) {
    return visitLogicalOrExprNode(ctx->conditionBreakLogicalOrExpression());
  }

  return std::make_unique<AssignmentExprNode>(
      helper::getPosition(ctx),
      getAssignmentOperator(ctx->assignmentOperator()),
      visitLogicalOrExprNode(ctx->conditionBreakLogicalOrExpression()),
      visitExprNode(ctx->conditionExpression()));
}

std::unique_ptr<ExprNode>
ASTBuilder::visitLogicalOrExprNode(RxParser::LogicalOrExpressionContext *ctx) {
  if (ctx->OROR().size() == 0) {
    return visitLogicalAndExprNode(ctx->logicalAndExpression(0));
  }
  std::size_t numOfOperators = ctx->logicalAndExpression().size();
  auto terminalNode = std::make_unique<LogicExprNode>(
      helper::getPosition(ctx->getStart(),
                          ctx->logicalAndExpression(1)->getStop()),
      LogicOperator::LogicOr,
      visitLogicalAndExprNode(ctx->logicalAndExpression(0)),
      visitLogicalAndExprNode(ctx->logicalAndExpression(1)));
  std::size_t cursor = 2;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<LogicExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->logicalAndExpression(cursor)->getStop()),
        LogicOperator::LogicOr, std::move(terminalNode),
        visitLogicalAndExprNode(ctx->logicalAndExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitLogicalOrExprNode(
    RxParser::StatementLogicalOrExpressionContext *ctx) {
  if (ctx->OROR().size() == 0) {
    return visitLogicalAndExprNode(ctx->statementLogicalAndExpression());
  }
  std::size_t numOfOperators = ctx->logicalAndExpression().size();
  auto terminalNode = std::make_unique<LogicExprNode>(
      helper::getPosition(ctx->getStart(),
                          ctx->logicalAndExpression(0)->getStop()),
      LogicOperator::LogicOr,
      visitLogicalAndExprNode(ctx->statementLogicalAndExpression()),
      visitLogicalAndExprNode(ctx->logicalAndExpression(0)));
  std::size_t cursor = 1;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<LogicExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->logicalAndExpression(cursor)->getStop()),
        LogicOperator::LogicOr, std::move(terminalNode),
        visitLogicalAndExprNode(ctx->logicalAndExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitLogicalOrExprNode(
    RxParser::ConditionLogicalOrExpressionContext *ctx) {
  if (ctx->OROR().size() == 0) {
    return visitLogicalAndExprNode(ctx->conditionLogicalAndExpression(0));
  }
  std::size_t numOfOperators = ctx->conditionLogicalAndExpression().size();
  auto terminalNode = std::make_unique<LogicExprNode>(
      helper::getPosition(ctx->getStart(),
                          ctx->conditionLogicalAndExpression(1)->getStop()),
      LogicOperator::LogicOr,
      visitLogicalAndExprNode(ctx->conditionLogicalAndExpression(0)),
      visitLogicalAndExprNode(ctx->conditionLogicalAndExpression(1)));
  std::size_t cursor = 2;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<LogicExprNode>(
        helper::getPosition(
            ctx->getStart(),
            ctx->conditionLogicalAndExpression(cursor)->getStop()),
        LogicOperator::LogicOr, std::move(terminalNode),
        visitLogicalAndExprNode(ctx->conditionLogicalAndExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitLogicalOrExprNode(
    RxParser::ConditionBreakLogicalOrExpressionContext *ctx) {
  if (ctx->OROR().size() == 0) {
    return visitLogicalAndExprNode(ctx->conditionBreakLogicalAndExpression());
  }
  std::size_t numOfOperators = ctx->conditionLogicalAndExpression().size();
  auto terminalNode = std::make_unique<LogicExprNode>(
      helper::getPosition(ctx->getStart(),
                          ctx->conditionLogicalAndExpression(0)->getStop()),
      LogicOperator::LogicOr,
      visitLogicalAndExprNode(ctx->conditionBreakLogicalAndExpression()),
      visitLogicalAndExprNode(ctx->conditionLogicalAndExpression(0)));
  std::size_t cursor = 1;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<LogicExprNode>(
        helper::getPosition(
            ctx->getStart(),
            ctx->conditionLogicalAndExpression(cursor)->getStop()),
        LogicOperator::LogicOr, std::move(terminalNode),
        visitLogicalAndExprNode(ctx->conditionLogicalAndExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitLogicalAndExprNode(
    RxParser::LogicalAndExpressionContext *ctx) {
  if (ctx->ANDAND().size() == 0) {
    return visitComparisonExprNode(ctx->comparisonExpression(0));
  }
  std::size_t numOfOperators = ctx->comparisonExpression().size();
  auto terminalNode = std::make_unique<LogicExprNode>(
      helper::getPosition(ctx->getStart(),
                          ctx->comparisonExpression(1)->getStop()),
      LogicOperator::LogicAnd,
      visitComparisonExprNode(ctx->comparisonExpression(0)),
      visitComparisonExprNode(ctx->comparisonExpression(1)));
  std::size_t cursor = 2;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<LogicExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->comparisonExpression(cursor)->getStop()),
        LogicOperator::LogicAnd, std::move(terminalNode),
        visitComparisonExprNode(ctx->comparisonExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitLogicalAndExprNode(
    RxParser::StatementLogicalAndExpressionContext *ctx) {
  if (ctx->ANDAND().size() == 0) {
    return visitComparisonExprNode(ctx->statementComparisonExpression());
  }
  std::size_t numOfOperators = ctx->comparisonExpression().size();
  auto terminalNode = std::make_unique<LogicExprNode>(
      helper::getPosition(ctx->getStart(),
                          ctx->comparisonExpression(0)->getStop()),
      LogicOperator::LogicAnd,
      visitComparisonExprNode(ctx->statementComparisonExpression()),
      visitComparisonExprNode(ctx->comparisonExpression(0)));
  std::size_t cursor = 1;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<LogicExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->comparisonExpression(cursor)->getStop()),
        LogicOperator::LogicAnd, std::move(terminalNode),
        visitComparisonExprNode(ctx->comparisonExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitLogicalAndExprNode(
    RxParser::ConditionLogicalAndExpressionContext *ctx) {
  if (ctx->ANDAND().size() == 0) {
    return visitComparisonExprNode(ctx->conditionComparisonExpression(0));
  }
  std::size_t numOfOperators = ctx->conditionComparisonExpression().size();
  auto terminalNode = std::make_unique<LogicExprNode>(
      helper::getPosition(ctx->getStart(),
                          ctx->conditionComparisonExpression(1)->getStop()),
      LogicOperator::LogicAnd,
      visitComparisonExprNode(ctx->conditionComparisonExpression(0)),
      visitComparisonExprNode(ctx->conditionComparisonExpression(1)));
  std::size_t cursor = 2;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<LogicExprNode>(
        helper::getPosition(
            ctx->getStart(),
            ctx->conditionComparisonExpression(cursor)->getStop()),
        LogicOperator::LogicAnd, std::move(terminalNode),
        visitComparisonExprNode(ctx->conditionComparisonExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitLogicalAndExprNode(
    RxParser::ConditionBreakLogicalAndExpressionContext *ctx) {
  if (ctx->ANDAND().size() == 0) {
    return visitComparisonExprNode(ctx->conditionBreakComparisonExpression());
  }
  std::size_t numOfOperators = ctx->conditionComparisonExpression().size();
  auto terminalNode = std::make_unique<LogicExprNode>(
      helper::getPosition(ctx->getStart(),
                          ctx->conditionComparisonExpression(0)->getStop()),
      LogicOperator::LogicAnd,
      visitComparisonExprNode(ctx->conditionBreakComparisonExpression()),
      visitComparisonExprNode(ctx->conditionComparisonExpression(0)));
  std::size_t cursor = 1;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<LogicExprNode>(
        helper::getPosition(
            ctx->getStart(),
            ctx->conditionComparisonExpression(cursor)->getStop()),
        LogicOperator::LogicAnd, std::move(terminalNode),
        visitComparisonExprNode(ctx->conditionComparisonExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitComparisonExprNode(
    RxParser::ComparisonExpressionContext *ctx) {
  if (ctx->LT() == nullptr && ctx->comparisonExceptLt() == nullptr) {
    return visitBitOrExprNode(ctx->bitOrExpression(0));
  } else if (ctx->LT()) {
    return std::make_unique<ComparisonExprNode>(
        helper::getPosition(ctx), ComparisonOperator::lessThan,
        visitBitOrExprNode(ctx->closedBitOrExpression()),
        visitBitOrExprNode(ctx->bitOrExpression(0)));
  } else if (ctx->comparisonExceptLt()) {
    return std::make_unique<ComparisonExprNode>(
        helper::getPosition(ctx),
        getComparisonOperator(ctx->comparisonExceptLt()),
        visitBitOrExprNode(ctx->bitOrExpression(0)),
        visitBitOrExprNode(ctx->bitOrExpression(1)));
  }
  return nullptr;
}

std::unique_ptr<ExprNode> ASTBuilder::visitComparisonExprNode(
    RxParser::StatementComparisonExpressionContext *ctx) {
  if (ctx->LT() == nullptr && ctx->comparisonExceptLt() == nullptr) {
    return visitBitOrExprNode(ctx->statementBitOrExpression());
  } else if (ctx->LT()) {
    return std::make_unique<ComparisonExprNode>(
        helper::getPosition(ctx), ComparisonOperator::lessThan,
        visitBitOrExprNode(ctx->statementClosedBitOrExpression()),
        visitBitOrExprNode(ctx->bitOrExpression()));
  } else if (ctx->comparisonExceptLt()) {
    return std::make_unique<ComparisonExprNode>(
        helper::getPosition(ctx),
        getComparisonOperator(ctx->comparisonExceptLt()),
        visitBitOrExprNode(ctx->statementBitOrExpression()),
        visitBitOrExprNode(ctx->bitOrExpression()));
  }
  return nullptr;
}

std::unique_ptr<ExprNode> ASTBuilder::visitComparisonExprNode(
    RxParser::ConditionComparisonExpressionContext *ctx) {
  if (ctx->LT() == nullptr && ctx->comparisonExceptLt() == nullptr) {
    return visitBitOrExprNode(ctx->conditionBitOrExpression(0));
  } else if (ctx->LT()) {
    return std::make_unique<ComparisonExprNode>(
        helper::getPosition(ctx), ComparisonOperator::lessThan,
        visitBitOrExprNode(ctx->conditionClosedBitOrExpression()),
        visitBitOrExprNode(ctx->conditionBitOrExpression(0)));
  } else if (ctx->comparisonExceptLt()) {
    return std::make_unique<ComparisonExprNode>(
        helper::getPosition(ctx),
        getComparisonOperator(ctx->comparisonExceptLt()),
        visitBitOrExprNode(ctx->conditionBitOrExpression(0)),
        visitBitOrExprNode(ctx->conditionBitOrExpression(1)));
  }
  return nullptr;
}

std::unique_ptr<ExprNode> ASTBuilder::visitComparisonExprNode(
    RxParser::ConditionBreakComparisonExpressionContext *ctx) {
  if (ctx->LT() == nullptr && ctx->comparisonExceptLt() == nullptr) {
    return visitBitOrExprNode(ctx->conditionBreakBitOrExpression());
  } else if (ctx->LT()) {
    return std::make_unique<ComparisonExprNode>(
        helper::getPosition(ctx), ComparisonOperator::lessThan,
        visitBitOrExprNode(ctx->conditionBreakClosedBitOrExpression()),
        visitBitOrExprNode(ctx->conditionBitOrExpression()));
  } else if (ctx->comparisonExceptLt()) {
    return std::make_unique<ComparisonExprNode>(
        helper::getPosition(ctx),
        getComparisonOperator(ctx->comparisonExceptLt()),
        visitBitOrExprNode(ctx->conditionBreakBitOrExpression()),
        visitBitOrExprNode(ctx->conditionBitOrExpression()));
  }
  return nullptr;
}

std::unique_ptr<ExprNode>
ASTBuilder::visitBitOrExprNode(RxParser::BitOrExpressionContext *ctx) {
  if (ctx->PIPE().size() == 0) {
    return visitBitXorExprNode(ctx->bitXorExpression(0));
  }
  std::size_t numOfOperators = ctx->bitXorExpression().size();
  auto terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx->getStart(), ctx->bitXorExpression(1)->getStop()),
      BinaryOperator::BitOr, visitBitXorExprNode(ctx->bitXorExpression(0)),
      visitBitXorExprNode(ctx->bitXorExpression(1)));
  std::size_t cursor = 2;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->bitXorExpression(cursor)->getStop()),
        BinaryOperator::BitOr, std::move(terminalNode),
        visitBitXorExprNode(ctx->bitXorExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode>
ASTBuilder::visitBitOrExprNode(RxParser::ClosedBitOrExpressionContext *ctx) {
  if (ctx->PIPE().size() == 0) {
    return visitBitXorExprNode(ctx->closedBitXorExpression());
  }
  std::size_t numOfOperators = ctx->bitXorExpression().size() + 1;
  auto terminalNode =
      numOfOperators == 2
          ? std::make_unique<BinaryExprNode>(
                helper::getPosition(ctx), BinaryOperator::BitOr,
                visitBitXorExprNode(ctx->bitXorExpression(0)),
                visitBitXorExprNode(ctx->closedBitXorExpression()))
          : std::make_unique<BinaryExprNode>(
                helper::getPosition(ctx->getStart(),
                                    ctx->bitXorExpression(1)->getStop()),
                BinaryOperator::BitOr,
                visitBitXorExprNode(ctx->bitXorExpression(0)),
                visitBitXorExprNode(ctx->bitXorExpression(1)));
  if (numOfOperators == 2) {
    return terminalNode;
  }
  std::size_t cursor = 2;
  while (cursor < numOfOperators - 1) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->bitXorExpression(cursor)->getStop()),
        BinaryOperator::BitOr, std::move(terminalNode),
        visitBitXorExprNode(ctx->bitXorExpression(cursor)));
    ++cursor;
  }
  terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx), BinaryOperator::BitOr, std::move(terminalNode),
      visitBitXorExprNode(ctx->closedBitXorExpression()));
  return terminalNode;
}

std::unique_ptr<ExprNode>
ASTBuilder::visitBitOrExprNode(RxParser::StatementBitOrExpressionContext *ctx) {
  if (ctx->PIPE().size() == 0) {
    return visitBitXorExprNode(ctx->statementBitXorExpression());
  }
  std::size_t numOfOperators = ctx->bitXorExpression().size();
  auto terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx->getStart(), ctx->bitXorExpression(0)->getStop()),
      BinaryOperator::BitOr,
      visitBitXorExprNode(ctx->statementBitXorExpression()),
      visitBitXorExprNode(ctx->bitXorExpression(0)));
  std::size_t cursor = 1;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->bitXorExpression(cursor)->getStop()),
        BinaryOperator::BitOr, std::move(terminalNode),
        visitBitXorExprNode(ctx->bitXorExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitBitOrExprNode(
    RxParser::StatementClosedBitOrExpressionContext *ctx) {
  if (ctx->PIPE().size() == 0) {
    return visitBitXorExprNode(ctx->statementClosedBitXorExpression());
  }
  std::size_t numOfOperators = ctx->bitXorExpression().size();
  auto terminalNode =
      numOfOperators == 0
          ? std::make_unique<BinaryExprNode>(
                helper::getPosition(ctx), BinaryOperator::BitOr,
                visitBitXorExprNode(ctx->statementBitXorExpression()),
                visitBitXorExprNode(ctx->closedBitXorExpression()))
          : std::make_unique<BinaryExprNode>(
                helper::getPosition(ctx->getStart(),
                                    ctx->bitXorExpression(0)->getStop()),
                BinaryOperator::BitOr,
                visitBitXorExprNode(ctx->statementBitXorExpression()),
                visitBitXorExprNode(ctx->bitXorExpression(0)));
  if (numOfOperators == 0) {
    return terminalNode;
  }
  std::size_t cursor = 1;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->bitXorExpression(cursor)->getStop()),
        BinaryOperator::BitOr, std::move(terminalNode),
        visitBitXorExprNode(ctx->bitXorExpression(cursor)));
    ++cursor;
  }
  terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx), BinaryOperator::BitOr, std::move(terminalNode),
      visitBitXorExprNode(ctx->closedBitXorExpression()));
  return terminalNode;
}

std::unique_ptr<ExprNode>
ASTBuilder::visitBitOrExprNode(RxParser::ConditionBitOrExpressionContext *ctx) {
  if (ctx->PIPE().size() == 0) {
    return visitBitXorExprNode(ctx->conditionBitXorExpression(0));
  }
  std::size_t numOfOperators = ctx->conditionBitXorExpression().size();
  auto terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx->getStart(),
                          ctx->conditionBitXorExpression(1)->getStop()),
      BinaryOperator::BitOr,
      visitBitXorExprNode(ctx->conditionBitXorExpression(0)),
      visitBitXorExprNode(ctx->conditionBitXorExpression(1)));
  std::size_t cursor = 2;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->conditionBitXorExpression(cursor)->getStop()),
        BinaryOperator::BitOr, std::move(terminalNode),
        visitBitXorExprNode(ctx->conditionBitXorExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitBitOrExprNode(
    RxParser::ConditionClosedBitOrExpressionContext *ctx) {
  if (ctx->PIPE().size() == 0) {
    return visitBitXorExprNode(ctx->conditionClosedBitXorExpression());
  }
  std::size_t numOfOperators = ctx->conditionBitXorExpression().size() + 1;
  auto terminalNode =
      numOfOperators == 2
          ? std::make_unique<BinaryExprNode>(
                helper::getPosition(ctx), BinaryOperator::BitOr,
                visitBitXorExprNode(ctx->conditionBitXorExpression(0)),
                visitBitXorExprNode(ctx->conditionClosedBitXorExpression()))
          : std::make_unique<BinaryExprNode>(
                helper::getPosition(
                    ctx->getStart(),
                    ctx->conditionBitXorExpression(1)->getStop()),
                BinaryOperator::BitOr,
                visitBitXorExprNode(ctx->conditionBitXorExpression(0)),
                visitBitXorExprNode(ctx->conditionBitXorExpression(1)));
  if (numOfOperators == 2) {
    return terminalNode;
  }
  std::size_t cursor = 2;
  while (cursor < numOfOperators - 1) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->conditionBitXorExpression(cursor)->getStop()),
        BinaryOperator::BitOr, std::move(terminalNode),
        visitBitXorExprNode(ctx->conditionBitXorExpression(cursor)));
    ++cursor;
  }
  terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx), BinaryOperator::BitOr, std::move(terminalNode),
      visitBitXorExprNode(ctx->conditionClosedBitXorExpression()));
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitBitOrExprNode(
    RxParser::ConditionBreakBitOrExpressionContext *ctx) {
  if (ctx->PIPE().size() == 0) {
    return visitBitXorExprNode(ctx->conditionBreakBitXorExpression());
  }
  std::size_t numOfOperators = ctx->conditionBitXorExpression().size();
  auto terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx->getStart(),
                          ctx->conditionBitXorExpression(0)->getStop()),
      BinaryOperator::BitOr,
      visitBitXorExprNode(ctx->conditionBreakBitXorExpression()),
      visitBitXorExprNode(ctx->conditionBitXorExpression(0)));
  std::size_t cursor = 1;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->conditionBitXorExpression(cursor)->getStop()),
        BinaryOperator::BitOr, std::move(terminalNode),
        visitBitXorExprNode(ctx->conditionBitXorExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitBitOrExprNode(
    RxParser::ConditionBreakClosedBitOrExpressionContext *ctx) {
  if (ctx->PIPE().size() == 0) {
    return visitBitXorExprNode(ctx->conditionBreakClosedBitXorExpression());
  }
  auto terminalNode =
      visitBitXorExprNode(ctx->conditionBreakBitXorExpression());
  for (auto *expr : ctx->conditionBitXorExpression()) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(), expr->getStop()),
        BinaryOperator::BitOr, std::move(terminalNode),
        visitBitXorExprNode(expr));
  }
  return std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx), BinaryOperator::BitOr, std::move(terminalNode),
      visitBitXorExprNode(ctx->conditionClosedBitXorExpression()));
}

std::unique_ptr<ExprNode>
ASTBuilder::visitBitXorExprNode(RxParser::BitXorExpressionContext *ctx) {
  if (ctx->CARET().size() == 0) {
    return visitBitAndExprNode(ctx->bitAndExpression(0));
  }
  std::size_t numOfOperators = ctx->bitAndExpression().size();
  auto terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx->getStart(), ctx->bitAndExpression(1)->getStop()),
      BinaryOperator::BitXor, visitBitAndExprNode(ctx->bitAndExpression(0)),
      visitBitAndExprNode(ctx->bitAndExpression(1)));
  std::size_t cursor = 2;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->bitAndExpression(cursor)->getStop()),
        BinaryOperator::BitXor, std::move(terminalNode),
        visitBitAndExprNode(ctx->bitAndExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode>
ASTBuilder::visitBitXorExprNode(RxParser::ClosedBitXorExpressionContext *ctx) {
  if (ctx->CARET().size() == 0) {
    return visitBitAndExprNode(ctx->closedBitAndExpression());
  }
  std::size_t numOfOperators = ctx->bitAndExpression().size() + 1;
  auto terminalNode =
      numOfOperators == 2
          ? std::make_unique<BinaryExprNode>(
                helper::getPosition(ctx), BinaryOperator::BitXor,
                visitBitAndExprNode(ctx->bitAndExpression(0)),
                visitBitAndExprNode(ctx->closedBitAndExpression()))
          : std::make_unique<BinaryExprNode>(
                helper::getPosition(ctx->getStart(),
                                    ctx->bitAndExpression(1)->getStop()),
                BinaryOperator::BitXor,
                visitBitAndExprNode(ctx->bitAndExpression(0)),
                visitBitAndExprNode(ctx->bitAndExpression(1)));
  if (numOfOperators == 2) {
    return terminalNode;
  }
  std::size_t cursor = 2;
  while (cursor < numOfOperators - 1) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->bitAndExpression(cursor)->getStop()),
        BinaryOperator::BitXor, std::move(terminalNode),
        visitBitAndExprNode(ctx->bitAndExpression(cursor)));
    ++cursor;
  }
  terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx), BinaryOperator::BitXor, std::move(terminalNode),
      visitBitAndExprNode(ctx->closedBitAndExpression()));
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitBitXorExprNode(
    RxParser::StatementBitXorExpressionContext *ctx) {
  if (ctx->CARET().size() == 0) {
    return visitBitAndExprNode(ctx->statementBitAndExpression());
  }
  std::size_t numOfOperators = ctx->bitAndExpression().size();
  auto terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx->getStart(), ctx->bitAndExpression(0)->getStop()),
      BinaryOperator::BitXor,
      visitBitAndExprNode(ctx->statementBitAndExpression()),
      visitBitAndExprNode(ctx->bitAndExpression(0)));
  std::size_t cursor = 1;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->bitAndExpression(cursor)->getStop()),
        BinaryOperator::BitXor, std::move(terminalNode),
        visitBitAndExprNode(ctx->bitAndExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitBitXorExprNode(
    RxParser::StatementClosedBitXorExpressionContext *ctx) {
  if (ctx->CARET().size() == 0) {
    return visitBitAndExprNode(ctx->statementClosedBitAndExpression());
  }
  std::size_t numOfOperators = ctx->bitAndExpression().size();
  auto terminalNode =
      numOfOperators == 0
          ? std::make_unique<BinaryExprNode>(
                helper::getPosition(ctx), BinaryOperator::BitXor,
                visitBitAndExprNode(ctx->statementBitAndExpression()),
                visitBitAndExprNode(ctx->closedBitAndExpression()))
          : std::make_unique<BinaryExprNode>(
                helper::getPosition(ctx->getStart(),
                                    ctx->bitAndExpression(0)->getStop()),
                BinaryOperator::BitXor,
                visitBitAndExprNode(ctx->statementBitAndExpression()),
                visitBitAndExprNode(ctx->bitAndExpression(0)));
  if (numOfOperators == 0) {
    return terminalNode;
  }
  std::size_t cursor = 1;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->bitAndExpression(cursor)->getStop()),
        BinaryOperator::BitXor, std::move(terminalNode),
        visitBitAndExprNode(ctx->bitAndExpression(cursor)));
    ++cursor;
  }
  terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx), BinaryOperator::BitXor, std::move(terminalNode),
      visitBitAndExprNode(ctx->closedBitAndExpression()));
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitBitXorExprNode(
    RxParser::ConditionBitXorExpressionContext *ctx) {
  if (ctx->CARET().size() == 0) {
    return visitBitAndExprNode(ctx->conditionBitAndExpression(0));
  }
  std::size_t numOfOperators = ctx->conditionBitAndExpression().size();
  auto terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx->getStart(),
                          ctx->conditionBitAndExpression(1)->getStop()),
      BinaryOperator::BitXor,
      visitBitAndExprNode(ctx->conditionBitAndExpression(0)),
      visitBitAndExprNode(ctx->conditionBitAndExpression(1)));
  std::size_t cursor = 2;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->conditionBitAndExpression(cursor)->getStop()),
        BinaryOperator::BitXor, std::move(terminalNode),
        visitBitAndExprNode(ctx->conditionBitAndExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitBitXorExprNode(
    RxParser::ConditionClosedBitXorExpressionContext *ctx) {
  if (ctx->CARET().size() == 0) {
    return visitBitAndExprNode(ctx->conditionClosedBitAndExpression());
  }
  std::size_t numOfOperators = ctx->conditionBitAndExpression().size() + 1;
  auto terminalNode =
      numOfOperators == 2
          ? std::make_unique<BinaryExprNode>(
                helper::getPosition(ctx), BinaryOperator::BitXor,
                visitBitAndExprNode(ctx->conditionBitAndExpression(0)),
                visitBitAndExprNode(ctx->conditionClosedBitAndExpression()))
          : std::make_unique<BinaryExprNode>(
                helper::getPosition(
                    ctx->getStart(),
                    ctx->conditionBitAndExpression(1)->getStop()),
                BinaryOperator::BitXor,
                visitBitAndExprNode(ctx->conditionBitAndExpression(0)),
                visitBitAndExprNode(ctx->conditionBitAndExpression(1)));
  if (numOfOperators == 2) {
    return terminalNode;
  }
  std::size_t cursor = 2;
  while (cursor < numOfOperators - 1) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->conditionBitAndExpression(cursor)->getStop()),
        BinaryOperator::BitXor, std::move(terminalNode),
        visitBitAndExprNode(ctx->conditionBitAndExpression(cursor)));
    ++cursor;
  }
  terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx), BinaryOperator::BitXor, std::move(terminalNode),
      visitBitAndExprNode(ctx->conditionClosedBitAndExpression()));
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitBitXorExprNode(
    RxParser::ConditionBreakBitXorExpressionContext *ctx) {
  if (ctx->CARET().size() == 0) {
    return visitBitAndExprNode(ctx->conditionBreakBitAndExpression());
  }
  std::size_t numOfOperators = ctx->conditionBitAndExpression().size();
  auto terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx->getStart(),
                          ctx->conditionBitAndExpression(0)->getStop()),
      BinaryOperator::BitXor,
      visitBitAndExprNode(ctx->conditionBreakBitAndExpression()),
      visitBitAndExprNode(ctx->conditionBitAndExpression(0)));
  std::size_t cursor = 1;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->conditionBitAndExpression(cursor)->getStop()),
        BinaryOperator::BitXor, std::move(terminalNode),
        visitBitAndExprNode(ctx->conditionBitAndExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitBitXorExprNode(
    RxParser::ConditionBreakClosedBitXorExpressionContext *ctx) {
  if (ctx->CARET().size() == 0) {
    return visitBitAndExprNode(ctx->conditionBreakClosedBitAndExpression());
  }
  auto terminalNode =
      visitBitAndExprNode(ctx->conditionBreakBitAndExpression());
  for (auto *expr : ctx->conditionBitAndExpression()) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(), expr->getStop()),
        BinaryOperator::BitXor, std::move(terminalNode),
        visitBitAndExprNode(expr));
  }
  return std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx), BinaryOperator::BitXor, std::move(terminalNode),
      visitBitAndExprNode(ctx->conditionClosedBitAndExpression()));
}

std::unique_ptr<ExprNode>
ASTBuilder::visitBitAndExprNode(RxParser::BitAndExpressionContext *ctx) {
  if (ctx->AMP().size() == 0) {
    return visitShiftExprNode(ctx->shiftExpression(0));
  }
  std::size_t numOfOperators = ctx->shiftExpression().size();
  auto terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx->getStart(), ctx->shiftExpression(1)->getStop()),
      BinaryOperator::BitAnd, visitShiftExprNode(ctx->shiftExpression(0)),
      visitShiftExprNode(ctx->shiftExpression(1)));
  std::size_t cursor = 2;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->shiftExpression(cursor)->getStop()),
        BinaryOperator::BitAnd, std::move(terminalNode),
        visitShiftExprNode(ctx->shiftExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode>
ASTBuilder::visitBitAndExprNode(RxParser::ClosedBitAndExpressionContext *ctx) {
  if (ctx->AMP().size() == 0) {
    return visitShiftExprNode(ctx->closedShiftExpression());
  }
  std::size_t numOfOperators = ctx->shiftExpression().size() + 1;
  auto terminalNode =
      numOfOperators == 2
          ? std::make_unique<BinaryExprNode>(
                helper::getPosition(ctx), BinaryOperator::BitAnd,
                visitShiftExprNode(ctx->shiftExpression(0)),
                visitShiftExprNode(ctx->closedShiftExpression()))
          : std::make_unique<BinaryExprNode>(
                helper::getPosition(ctx->getStart(),
                                    ctx->shiftExpression(1)->getStop()),
                BinaryOperator::BitAnd,
                visitShiftExprNode(ctx->shiftExpression(0)),
                visitShiftExprNode(ctx->shiftExpression(1)));
  if (numOfOperators == 2) {
    return terminalNode;
  }
  std::size_t cursor = 2;
  while (cursor < numOfOperators - 1) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->shiftExpression(cursor)->getStop()),
        BinaryOperator::BitAnd, std::move(terminalNode),
        visitShiftExprNode(ctx->shiftExpression(cursor)));
    ++cursor;
  }
  terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx), BinaryOperator::BitAnd, std::move(terminalNode),
      visitShiftExprNode(ctx->closedShiftExpression()));
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitBitAndExprNode(
    RxParser::StatementBitAndExpressionContext *ctx) {
  if (ctx->AMP().size() == 0) {
    return visitShiftExprNode(ctx->statementShiftExpression());
  }
  std::size_t numOfOperators = ctx->shiftExpression().size();
  auto terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx->getStart(), ctx->shiftExpression(0)->getStop()),
      BinaryOperator::BitAnd,
      visitShiftExprNode(ctx->statementShiftExpression()),
      visitShiftExprNode(ctx->shiftExpression(0)));
  std::size_t cursor = 1;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->shiftExpression(cursor)->getStop()),
        BinaryOperator::BitAnd, std::move(terminalNode),
        visitShiftExprNode(ctx->shiftExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitBitAndExprNode(
    RxParser::StatementClosedBitAndExpressionContext *ctx) {
  if (ctx->AMP().size() == 0) {
    return visitShiftExprNode(ctx->statementClosedShiftExpression());
  }
  std::size_t numOfOperators = ctx->shiftExpression().size();
  auto terminalNode =
      numOfOperators == 0
          ? std::make_unique<BinaryExprNode>(
                helper::getPosition(ctx), BinaryOperator::BitAnd,
                visitShiftExprNode(ctx->statementShiftExpression()),
                visitShiftExprNode(ctx->closedShiftExpression()))
          : std::make_unique<BinaryExprNode>(
                helper::getPosition(ctx->getStart(),
                                    ctx->shiftExpression(0)->getStop()),
                BinaryOperator::BitAnd,
                visitShiftExprNode(ctx->statementShiftExpression()),
                visitShiftExprNode(ctx->shiftExpression(0)));
  if (numOfOperators == 0) {
    return terminalNode;
  }
  std::size_t cursor = 1;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->shiftExpression(cursor)->getStop()),
        BinaryOperator::BitAnd, std::move(terminalNode),
        visitShiftExprNode(ctx->shiftExpression(cursor)));
    ++cursor;
  }
  terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx), BinaryOperator::BitAnd, std::move(terminalNode),
      visitShiftExprNode(ctx->closedShiftExpression()));
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitBitAndExprNode(
    RxParser::ConditionBitAndExpressionContext *ctx) {
  if (ctx->AMP().size() == 0) {
    return visitShiftExprNode(ctx->conditionShiftExpression(0));
  }
  std::size_t numOfOperators = ctx->conditionShiftExpression().size();
  auto terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx->getStart(),
                          ctx->conditionShiftExpression(1)->getStop()),
      BinaryOperator::BitAnd,
      visitShiftExprNode(ctx->conditionShiftExpression(0)),
      visitShiftExprNode(ctx->conditionShiftExpression(1)));
  std::size_t cursor = 2;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->conditionShiftExpression(cursor)->getStop()),
        BinaryOperator::BitAnd, std::move(terminalNode),
        visitShiftExprNode(ctx->conditionShiftExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitBitAndExprNode(
    RxParser::ConditionClosedBitAndExpressionContext *ctx) {
  if (ctx->AMP().size() == 0) {
    return visitShiftExprNode(ctx->conditionClosedShiftExpression());
  }
  std::size_t numOfOperators = ctx->conditionShiftExpression().size() + 1;
  auto terminalNode =
      numOfOperators == 2
          ? std::make_unique<BinaryExprNode>(
                helper::getPosition(ctx), BinaryOperator::BitAnd,
                visitShiftExprNode(ctx->conditionShiftExpression(0)),
                visitShiftExprNode(ctx->conditionClosedShiftExpression()))
          : std::make_unique<BinaryExprNode>(
                helper::getPosition(
                    ctx->getStart(),
                    ctx->conditionShiftExpression(1)->getStop()),
                BinaryOperator::BitAnd,
                visitShiftExprNode(ctx->conditionShiftExpression(0)),
                visitShiftExprNode(ctx->conditionShiftExpression(1)));
  if (numOfOperators == 2) {
    return terminalNode;
  }
  std::size_t cursor = 2;
  while (cursor < numOfOperators - 1) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->conditionShiftExpression(cursor)->getStop()),
        BinaryOperator::BitAnd, std::move(terminalNode),
        visitShiftExprNode(ctx->conditionShiftExpression(cursor)));
    ++cursor;
  }
  terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx), BinaryOperator::BitAnd, std::move(terminalNode),
      visitShiftExprNode(ctx->conditionClosedShiftExpression()));
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitBitAndExprNode(
    RxParser::ConditionBreakBitAndExpressionContext *ctx) {
  if (ctx->AMP().size() == 0) {
    return visitShiftExprNode(ctx->conditionBreakShiftExpression());
  }
  std::size_t numOfOperators = ctx->conditionShiftExpression().size();
  auto terminalNode = std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx->getStart(),
                          ctx->conditionShiftExpression(0)->getStop()),
      BinaryOperator::BitAnd,
      visitShiftExprNode(ctx->conditionBreakShiftExpression()),
      visitShiftExprNode(ctx->conditionShiftExpression(0)));
  std::size_t cursor = 1;
  while (cursor < numOfOperators) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->conditionShiftExpression(cursor)->getStop()),
        BinaryOperator::BitAnd, std::move(terminalNode),
        visitShiftExprNode(ctx->conditionShiftExpression(cursor)));
    ++cursor;
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitBitAndExprNode(
    RxParser::ConditionBreakClosedBitAndExpressionContext *ctx) {
  if (ctx->AMP().size() == 0) {
    return visitShiftExprNode(ctx->conditionBreakClosedShiftExpression());
  }
  auto terminalNode = visitShiftExprNode(ctx->conditionBreakShiftExpression());
  for (auto *expr : ctx->conditionShiftExpression()) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(), expr->getStop()),
        BinaryOperator::BitAnd, std::move(terminalNode),
        visitShiftExprNode(expr));
  }
  return std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx), BinaryOperator::BitAnd, std::move(terminalNode),
      visitShiftExprNode(ctx->conditionClosedShiftExpression()));
}

std::unique_ptr<ExprNode>
ASTBuilder::visitShiftExprNode(RxParser::ShiftExpressionContext *ctx) {
  std::size_t numOfOperators = ctx->shiftRight().size() + ctx->SHL().size();
  if (numOfOperators == 0) {
    return visitAdditiveExprNode(ctx->additiveExpression(0));
  }
  std::vector<BinaryOperator> shiftKinds;
  shiftKinds.reserve(numOfOperators);
  for (auto *child : ctx->children) {
    if (auto childNode = dynamic_cast<antlr4::tree::TerminalNode *>(child);
        childNode && childNode->getSymbol()->getType() == RxParser::SHL) {
      shiftKinds.emplace_back(BinaryOperator::ShiftLeft);
    } else if (auto childNode =
                   dynamic_cast<RxParser::ShiftRightContext *>(child);
               childNode) {
      shiftKinds.emplace_back(BinaryOperator::ShiftRight);
    }
  }
  std::size_t shiftLeftCursor = 0;
  std::size_t shiftRightCursor = 0;
  if (shiftKinds.front() == BinaryOperator::ShiftLeft) {
    if (numOfOperators == 1) {
      return std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), BinaryOperator::ShiftLeft,
          visitAdditiveExprNode(ctx->closedAdditiveExpression().front()),
          visitAdditiveExprNode(ctx->additiveExpression().front()));
    } else {
      BinaryOperator lastOp = BinaryOperator::ShiftLeft;
      auto terminalNode =
          visitAdditiveExprNode(ctx->closedAdditiveExpression().front());
      ++shiftLeftCursor;
      for (std::size_t cursor = 1; cursor < shiftKinds.size(); ++cursor) {
        if (shiftKinds[cursor] == BinaryOperator::ShiftLeft) {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->closedAdditiveExpression(shiftLeftCursor)->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(
                  ctx->closedAdditiveExpression(shiftLeftCursor)));
          ++shiftLeftCursor;
          lastOp = BinaryOperator::ShiftLeft;
        } else {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->additiveExpression(shiftRightCursor)->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(ctx->additiveExpression(shiftRightCursor)));
          ++shiftRightCursor;
          lastOp = BinaryOperator::ShiftRight;
        }
      }
      terminalNode = std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), lastOp, std::move(terminalNode),
          visitAdditiveExprNode(ctx->additiveExpression().back()));
      return terminalNode;
    }
  } else {
    if (numOfOperators == 1) {
      return std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), BinaryOperator::ShiftRight,
          visitAdditiveExprNode(ctx->additiveExpression(0)),
          visitAdditiveExprNode(ctx->additiveExpression(1)));
    } else {
      BinaryOperator lastOp = BinaryOperator::ShiftRight;
      auto terminalNode =
          visitAdditiveExprNode(ctx->additiveExpression().front());
      ++shiftRightCursor;
      for (std::size_t cursor = 1; cursor < shiftKinds.size(); ++cursor) {
        if (shiftKinds[cursor] == BinaryOperator::ShiftLeft) {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->closedAdditiveExpression(shiftLeftCursor)->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(
                  ctx->closedAdditiveExpression(shiftLeftCursor)));
          ++shiftLeftCursor;
          lastOp = BinaryOperator::ShiftLeft;
        } else {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->additiveExpression(shiftRightCursor)->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(ctx->additiveExpression(shiftRightCursor)));
          ++shiftRightCursor;
          lastOp = BinaryOperator::ShiftRight;
        }
      }
      terminalNode = std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), lastOp, std::move(terminalNode),
          visitAdditiveExprNode(ctx->additiveExpression().back()));
      return terminalNode;
    }
  }
}

std::unique_ptr<ExprNode>
ASTBuilder::visitShiftExprNode(RxParser::ClosedShiftExpressionContext *ctx) {
  std::size_t numOfOperators = ctx->shiftRight().size() + ctx->SHL().size();
  if (numOfOperators == 0) {
    return visitAdditiveExprNode(ctx->closedAdditiveExpression(0));
  }
  std::vector<BinaryOperator> shiftKinds;
  shiftKinds.reserve(numOfOperators);
  for (auto *child : ctx->children) {
    if (auto childNode = dynamic_cast<antlr4::tree::TerminalNode *>(child);
        childNode && childNode->getSymbol()->getType() == RxParser::SHL) {
      shiftKinds.emplace_back(BinaryOperator::ShiftLeft);
    } else if (auto childNode =
                   dynamic_cast<RxParser::ShiftRightContext *>(child);
               childNode) {
      shiftKinds.emplace_back(BinaryOperator::ShiftRight);
    }
  }
  std::size_t shiftLeftCursor = 0;
  std::size_t shiftRightCursor = 0;
  if (shiftKinds.front() == BinaryOperator::ShiftLeft) {
    if (numOfOperators == 1) {
      return std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), BinaryOperator::ShiftLeft,
          visitAdditiveExprNode(ctx->closedAdditiveExpression(0)),
          visitAdditiveExprNode(ctx->closedAdditiveExpression(1)));
    } else {
      BinaryOperator lastOp = BinaryOperator::ShiftLeft;
      auto terminalNode =
          visitAdditiveExprNode(ctx->closedAdditiveExpression().front());
      ++shiftLeftCursor;
      for (std::size_t cursor = 1; cursor < shiftKinds.size(); ++cursor) {
        if (shiftKinds[cursor] == BinaryOperator::ShiftLeft) {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->closedAdditiveExpression(shiftLeftCursor)->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(
                  ctx->closedAdditiveExpression(shiftLeftCursor)));
          ++shiftLeftCursor;
          lastOp = BinaryOperator::ShiftLeft;
        } else {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->additiveExpression(shiftRightCursor)->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(ctx->additiveExpression(shiftRightCursor)));
          ++shiftRightCursor;
          lastOp = BinaryOperator::ShiftRight;
        }
      }
      terminalNode = std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), lastOp, std::move(terminalNode),
          visitAdditiveExprNode(ctx->closedAdditiveExpression().back()));
      return terminalNode;
    }
  } else {
    if (numOfOperators == 1) {
      return std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), BinaryOperator::ShiftRight,
          visitAdditiveExprNode(ctx->additiveExpression().front()),
          visitAdditiveExprNode(ctx->closedAdditiveExpression().front()));
    } else {
      BinaryOperator lastOp = BinaryOperator::ShiftRight;
      auto terminalNode =
          visitAdditiveExprNode(ctx->additiveExpression().front());
      ++shiftRightCursor;
      for (std::size_t cursor = 1; cursor < shiftKinds.size(); ++cursor) {
        if (shiftKinds[cursor] == BinaryOperator::ShiftLeft) {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->closedAdditiveExpression(shiftLeftCursor)->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(
                  ctx->closedAdditiveExpression(shiftLeftCursor)));
          ++shiftLeftCursor;
          lastOp = BinaryOperator::ShiftLeft;
        } else {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->additiveExpression(shiftRightCursor)->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(ctx->additiveExpression(shiftRightCursor)));
          ++shiftRightCursor;
          lastOp = BinaryOperator::ShiftRight;
        }
      }
      terminalNode = std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), lastOp, std::move(terminalNode),
          visitAdditiveExprNode(ctx->closedAdditiveExpression().back()));
      return terminalNode;
    }
  }
}

std::unique_ptr<ExprNode>
ASTBuilder::visitShiftExprNode(RxParser::StatementShiftExpressionContext *ctx) {
  std::size_t numOfOperators = ctx->shiftRight().size() + ctx->SHL().size();
  if (numOfOperators == 0) {
    return visitAdditiveExprNode(ctx->statementAdditiveExpression());
  }
  std::vector<BinaryOperator> shiftKinds;
  shiftKinds.reserve(numOfOperators);
  for (auto *child : ctx->children) {
    if (auto childNode = dynamic_cast<antlr4::tree::TerminalNode *>(child);
        childNode && childNode->getSymbol()->getType() == RxParser::SHL) {
      shiftKinds.emplace_back(BinaryOperator::ShiftLeft);
    } else if (auto childNode =
                   dynamic_cast<RxParser::ShiftRightContext *>(child);
               childNode) {
      shiftKinds.emplace_back(BinaryOperator::ShiftRight);
    }
  }
  std::size_t shiftLeftCursor = 0;
  std::size_t shiftRightCursor = 0;
  if (shiftKinds.front() == BinaryOperator::ShiftLeft) {
    if (numOfOperators == 1) {
      return std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), BinaryOperator::ShiftLeft,
          visitAdditiveExprNode(ctx->statementClosedAdditiveExpression()),
          visitAdditiveExprNode(ctx->additiveExpression().front()));
    } else {
      BinaryOperator lastOp = BinaryOperator::ShiftLeft;
      auto terminalNode =
          visitAdditiveExprNode(ctx->statementClosedAdditiveExpression());
      for (std::size_t cursor = 1; cursor < shiftKinds.size(); ++cursor) {
        if (shiftKinds[cursor] == BinaryOperator::ShiftLeft) {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->closedAdditiveExpression(shiftLeftCursor)->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(
                  ctx->closedAdditiveExpression(shiftLeftCursor)));
          ++shiftLeftCursor;
          lastOp = BinaryOperator::ShiftLeft;
        } else {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->additiveExpression(shiftRightCursor)->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(ctx->additiveExpression(shiftRightCursor)));
          ++shiftRightCursor;
          lastOp = BinaryOperator::ShiftRight;
        }
      }
      terminalNode = std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), lastOp, std::move(terminalNode),
          visitAdditiveExprNode(ctx->additiveExpression().back()));
      return terminalNode;
    }
  } else {
    if (numOfOperators == 1) {
      return std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), BinaryOperator::ShiftRight,
          visitAdditiveExprNode(ctx->statementAdditiveExpression()),
          visitAdditiveExprNode(ctx->additiveExpression().front()));
    } else {
      BinaryOperator lastOp = BinaryOperator::ShiftRight;
      auto terminalNode =
          visitAdditiveExprNode(ctx->statementAdditiveExpression());
      for (std::size_t cursor = 1; cursor < shiftKinds.size(); ++cursor) {
        if (shiftKinds[cursor] == BinaryOperator::ShiftLeft) {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->closedAdditiveExpression(shiftLeftCursor)->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(
                  ctx->closedAdditiveExpression(shiftLeftCursor)));
          ++shiftLeftCursor;
          lastOp = BinaryOperator::ShiftLeft;
        } else {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->additiveExpression(shiftRightCursor)->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(ctx->additiveExpression(shiftRightCursor)));
          ++shiftRightCursor;
          lastOp = BinaryOperator::ShiftRight;
        }
      }
      terminalNode = std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), lastOp, std::move(terminalNode),
          visitAdditiveExprNode(ctx->additiveExpression().back()));
      return terminalNode;
    }
  }
}

std::unique_ptr<ExprNode> ASTBuilder::visitShiftExprNode(
    RxParser::StatementClosedShiftExpressionContext *ctx) {
  std::size_t numOfOperators = ctx->shiftRight().size() + ctx->SHL().size();
  if (numOfOperators == 0) {
    return visitAdditiveExprNode(ctx->statementClosedAdditiveExpression());
  }
  std::vector<BinaryOperator> shiftKinds;
  shiftKinds.reserve(numOfOperators);
  for (auto *child : ctx->children) {
    if (auto childNode = dynamic_cast<antlr4::tree::TerminalNode *>(child);
        childNode && childNode->getSymbol()->getType() == RxParser::SHL) {
      shiftKinds.emplace_back(BinaryOperator::ShiftLeft);
    } else if (auto childNode =
                   dynamic_cast<RxParser::ShiftRightContext *>(child);
               childNode) {
      shiftKinds.emplace_back(BinaryOperator::ShiftRight);
    }
  }
  std::size_t shiftLeftCursor = 0;
  std::size_t shiftRightCursor = 0;
  if (shiftKinds.front() == BinaryOperator::ShiftLeft) {
    if (numOfOperators == 1) {
      return std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), BinaryOperator::ShiftLeft,
          visitAdditiveExprNode(ctx->statementClosedAdditiveExpression()),
          visitAdditiveExprNode(ctx->closedAdditiveExpression().front()));
    } else {
      BinaryOperator lastOp = BinaryOperator::ShiftLeft;
      auto terminalNode =
          visitAdditiveExprNode(ctx->statementClosedAdditiveExpression());
      for (std::size_t cursor = 1; cursor < shiftKinds.size(); ++cursor) {
        if (shiftKinds[cursor] == BinaryOperator::ShiftLeft) {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->closedAdditiveExpression(shiftLeftCursor)->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(
                  ctx->closedAdditiveExpression(shiftLeftCursor)));
          ++shiftLeftCursor;
          lastOp = BinaryOperator::ShiftLeft;
        } else {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->additiveExpression(shiftRightCursor)->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(ctx->additiveExpression(shiftRightCursor)));
          ++shiftRightCursor;
          lastOp = BinaryOperator::ShiftRight;
        }
      }
      terminalNode = std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), lastOp, std::move(terminalNode),
          visitAdditiveExprNode(ctx->closedAdditiveExpression().back()));
      return terminalNode;
    }
  } else {
    if (numOfOperators == 1) {
      return std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), BinaryOperator::ShiftRight,
          visitAdditiveExprNode(ctx->statementAdditiveExpression()),
          visitAdditiveExprNode(ctx->closedAdditiveExpression().front()));
    } else {
      BinaryOperator lastOp = BinaryOperator::ShiftRight;
      auto terminalNode =
          visitAdditiveExprNode(ctx->statementAdditiveExpression());
      for (std::size_t cursor = 1; cursor < shiftKinds.size(); ++cursor) {
        if (shiftKinds[cursor] == BinaryOperator::ShiftLeft) {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->closedAdditiveExpression(shiftLeftCursor)->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(
                  ctx->closedAdditiveExpression(shiftLeftCursor)));
          ++shiftLeftCursor;
          lastOp = BinaryOperator::ShiftLeft;
        } else {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->additiveExpression(shiftRightCursor)->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(ctx->additiveExpression(shiftRightCursor)));
          ++shiftRightCursor;
          lastOp = BinaryOperator::ShiftRight;
        }
      }
      terminalNode = std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), lastOp, std::move(terminalNode),
          visitAdditiveExprNode(ctx->closedAdditiveExpression().back()));
      return terminalNode;
    }
  }
}

std::unique_ptr<ExprNode>
ASTBuilder::visitShiftExprNode(RxParser::ConditionShiftExpressionContext *ctx) {
  std::size_t numOfOperators = ctx->shiftRight().size() + ctx->SHL().size();
  if (numOfOperators == 0) {
    return visitAdditiveExprNode(ctx->conditionAdditiveExpression(0));
  }
  std::vector<BinaryOperator> shiftKinds;
  shiftKinds.reserve(numOfOperators);
  for (auto *child : ctx->children) {
    if (auto childNode = dynamic_cast<antlr4::tree::TerminalNode *>(child);
        childNode && childNode->getSymbol()->getType() == RxParser::SHL) {
      shiftKinds.emplace_back(BinaryOperator::ShiftLeft);
    } else if (auto childNode =
                   dynamic_cast<RxParser::ShiftRightContext *>(child);
               childNode) {
      shiftKinds.emplace_back(BinaryOperator::ShiftRight);
    }
  }
  std::size_t shiftLeftCursor = 0;
  std::size_t shiftRightCursor = 0;
  if (shiftKinds.front() == BinaryOperator::ShiftLeft) {
    if (numOfOperators == 1) {
      return std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), BinaryOperator::ShiftLeft,
          visitAdditiveExprNode(
              ctx->conditionClosedAdditiveExpression().front()),
          visitAdditiveExprNode(ctx->conditionAdditiveExpression().front()));
    } else {
      BinaryOperator lastOp = BinaryOperator::ShiftLeft;
      auto terminalNode = visitAdditiveExprNode(
          ctx->conditionClosedAdditiveExpression().front());
      ++shiftLeftCursor;
      for (std::size_t cursor = 1; cursor < shiftKinds.size(); ++cursor) {
        if (shiftKinds[cursor] == BinaryOperator::ShiftLeft) {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->conditionClosedAdditiveExpression(shiftLeftCursor)
                      ->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(
                  ctx->conditionClosedAdditiveExpression(shiftLeftCursor)));
          ++shiftLeftCursor;
          lastOp = BinaryOperator::ShiftLeft;
        } else {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->conditionAdditiveExpression(shiftRightCursor)
                      ->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(
                  ctx->conditionAdditiveExpression(shiftRightCursor)));
          ++shiftRightCursor;
          lastOp = BinaryOperator::ShiftRight;
        }
      }
      terminalNode = std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), lastOp, std::move(terminalNode),
          visitAdditiveExprNode(ctx->conditionAdditiveExpression().back()));
      return terminalNode;
    }
  } else {
    if (numOfOperators == 1) {
      return std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), BinaryOperator::ShiftRight,
          visitAdditiveExprNode(ctx->conditionAdditiveExpression(0)),
          visitAdditiveExprNode(ctx->conditionAdditiveExpression(1)));
    } else {
      BinaryOperator lastOp = BinaryOperator::ShiftRight;
      auto terminalNode =
          visitAdditiveExprNode(ctx->conditionAdditiveExpression().front());
      ++shiftRightCursor;
      for (std::size_t cursor = 1; cursor < shiftKinds.size(); ++cursor) {
        if (shiftKinds[cursor] == BinaryOperator::ShiftLeft) {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->conditionClosedAdditiveExpression(shiftLeftCursor)
                      ->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(
                  ctx->conditionClosedAdditiveExpression(shiftLeftCursor)));
          ++shiftLeftCursor;
          lastOp = BinaryOperator::ShiftLeft;
        } else {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->conditionAdditiveExpression(shiftRightCursor)
                      ->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(
                  ctx->conditionAdditiveExpression(shiftRightCursor)));
          ++shiftRightCursor;
          lastOp = BinaryOperator::ShiftRight;
        }
      }
      terminalNode = std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), lastOp, std::move(terminalNode),
          visitAdditiveExprNode(ctx->conditionAdditiveExpression().back()));
      return terminalNode;
    }
  }
}

std::unique_ptr<ExprNode> ASTBuilder::visitShiftExprNode(
    RxParser::ConditionClosedShiftExpressionContext *ctx) {
  std::size_t numOfOperators = ctx->shiftRight().size() + ctx->SHL().size();
  if (numOfOperators == 0) {
    return visitAdditiveExprNode(ctx->conditionClosedAdditiveExpression(0));
  }
  std::vector<BinaryOperator> shiftKinds;
  shiftKinds.reserve(numOfOperators);
  for (auto *child : ctx->children) {
    if (auto childNode = dynamic_cast<antlr4::tree::TerminalNode *>(child);
        childNode && childNode->getSymbol()->getType() == RxParser::SHL) {
      shiftKinds.emplace_back(BinaryOperator::ShiftLeft);
    } else if (auto childNode =
                   dynamic_cast<RxParser::ShiftRightContext *>(child);
               childNode) {
      shiftKinds.emplace_back(BinaryOperator::ShiftRight);
    }
  }
  std::size_t shiftLeftCursor = 0;
  std::size_t shiftRightCursor = 0;
  if (shiftKinds.front() == BinaryOperator::ShiftLeft) {
    if (numOfOperators == 1) {
      return std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), BinaryOperator::ShiftLeft,
          visitAdditiveExprNode(ctx->conditionClosedAdditiveExpression(0)),
          visitAdditiveExprNode(ctx->conditionClosedAdditiveExpression(1)));
    } else {
      BinaryOperator lastOp = BinaryOperator::ShiftLeft;
      auto terminalNode = visitAdditiveExprNode(
          ctx->conditionClosedAdditiveExpression().front());
      ++shiftLeftCursor;
      for (std::size_t cursor = 1; cursor < shiftKinds.size(); ++cursor) {
        if (shiftKinds[cursor] == BinaryOperator::ShiftLeft) {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->conditionClosedAdditiveExpression(shiftLeftCursor)
                      ->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(
                  ctx->conditionClosedAdditiveExpression(shiftLeftCursor)));
          ++shiftLeftCursor;
          lastOp = BinaryOperator::ShiftLeft;
        } else {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->conditionAdditiveExpression(shiftRightCursor)
                      ->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(
                  ctx->conditionAdditiveExpression(shiftRightCursor)));
          ++shiftRightCursor;
          lastOp = BinaryOperator::ShiftRight;
        }
      }
      terminalNode = std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), lastOp, std::move(terminalNode),
          visitAdditiveExprNode(
              ctx->conditionClosedAdditiveExpression().back()));
      return terminalNode;
    }
  } else {
    if (numOfOperators == 1) {
      return std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), BinaryOperator::ShiftRight,
          visitAdditiveExprNode(ctx->conditionAdditiveExpression().front()),
          visitAdditiveExprNode(
              ctx->conditionClosedAdditiveExpression().front()));
    } else {
      BinaryOperator lastOp = BinaryOperator::ShiftRight;
      auto terminalNode =
          visitAdditiveExprNode(ctx->conditionAdditiveExpression().front());
      ++shiftRightCursor;
      for (std::size_t cursor = 1; cursor < shiftKinds.size(); ++cursor) {
        if (shiftKinds[cursor] == BinaryOperator::ShiftLeft) {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->conditionClosedAdditiveExpression(shiftLeftCursor)
                      ->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(
                  ctx->conditionClosedAdditiveExpression(shiftLeftCursor)));
          ++shiftLeftCursor;
          lastOp = BinaryOperator::ShiftLeft;
        } else {
          terminalNode = std::make_unique<BinaryExprNode>(
              helper::getPosition(
                  ctx->getStart(),
                  ctx->conditionAdditiveExpression(shiftRightCursor)
                      ->getStop()),
              lastOp, std::move(terminalNode),
              visitAdditiveExprNode(
                  ctx->conditionAdditiveExpression(shiftRightCursor)));
          ++shiftRightCursor;
          lastOp = BinaryOperator::ShiftRight;
        }
      }
      terminalNode = std::make_unique<BinaryExprNode>(
          helper::getPosition(ctx), lastOp, std::move(terminalNode),
          visitAdditiveExprNode(
              ctx->conditionClosedAdditiveExpression().back()));
      return terminalNode;
    }
  }
}

std::unique_ptr<ExprNode> ASTBuilder::visitShiftExprNode(
    RxParser::ConditionBreakShiftExpressionContext *ctx) {
  std::size_t numOfOperators = ctx->shiftRight().size() + ctx->SHL().size();
  if (numOfOperators == 0) {
    return visitAdditiveExprNode(ctx->conditionBreakAdditiveExpression());
  }
  std::vector<BinaryOperator> shiftKinds;
  shiftKinds.reserve(numOfOperators);
  for (auto *child : ctx->children) {
    if (auto childNode = dynamic_cast<antlr4::tree::TerminalNode *>(child);
        childNode && childNode->getSymbol()->getType() == RxParser::SHL) {
      shiftKinds.emplace_back(BinaryOperator::ShiftLeft);
    } else if (auto childNode =
                   dynamic_cast<RxParser::ShiftRightContext *>(child);
               childNode) {
      shiftKinds.emplace_back(BinaryOperator::ShiftRight);
    }
  }
  std::size_t shiftLeftCursor = 0;
  std::size_t shiftRightCursor = 0;
  auto terminalNode =
      shiftKinds.front() == BinaryOperator::ShiftLeft
          ? visitAdditiveExprNode(ctx->conditionBreakClosedAdditiveExpression())
          : visitAdditiveExprNode(ctx->conditionBreakAdditiveExpression());
  BinaryOperator lastOp = shiftKinds.front();
  for (std::size_t cursor = 1; cursor < shiftKinds.size(); ++cursor) {
    if (shiftKinds[cursor] == BinaryOperator::ShiftLeft) {
      terminalNode = std::make_unique<BinaryExprNode>(
          helper::getPosition(
              ctx->getStart(),
              ctx->conditionClosedAdditiveExpression(shiftLeftCursor)
                  ->getStop()),
          lastOp, std::move(terminalNode),
          visitAdditiveExprNode(
              ctx->conditionClosedAdditiveExpression(shiftLeftCursor)));
      ++shiftLeftCursor;
    } else {
      terminalNode = std::make_unique<BinaryExprNode>(
          helper::getPosition(
              ctx->getStart(),
              ctx->conditionAdditiveExpression(shiftRightCursor)->getStop()),
          lastOp, std::move(terminalNode),
          visitAdditiveExprNode(
              ctx->conditionAdditiveExpression(shiftRightCursor)));
      ++shiftRightCursor;
    }
    lastOp = shiftKinds[cursor];
  }
  return std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx), lastOp, std::move(terminalNode),
      visitAdditiveExprNode(ctx->conditionAdditiveExpression().back()));
}

std::unique_ptr<ExprNode> ASTBuilder::visitShiftExprNode(
    RxParser::ConditionBreakClosedShiftExpressionContext *ctx) {
  std::size_t numOfOperators = ctx->shiftRight().size() + ctx->SHL().size();
  if (numOfOperators == 0) {
    return visitAdditiveExprNode(ctx->conditionBreakClosedAdditiveExpression());
  }
  std::vector<BinaryOperator> shiftKinds;
  shiftKinds.reserve(numOfOperators);
  for (auto *child : ctx->children) {
    if (auto childNode = dynamic_cast<antlr4::tree::TerminalNode *>(child);
        childNode && childNode->getSymbol()->getType() == RxParser::SHL) {
      shiftKinds.emplace_back(BinaryOperator::ShiftLeft);
    } else if (auto childNode =
                   dynamic_cast<RxParser::ShiftRightContext *>(child);
               childNode) {
      shiftKinds.emplace_back(BinaryOperator::ShiftRight);
    }
  }
  std::size_t shiftLeftCursor = 0;
  std::size_t shiftRightCursor = 0;
  auto terminalNode =
      shiftKinds.front() == BinaryOperator::ShiftLeft
          ? visitAdditiveExprNode(ctx->conditionBreakClosedAdditiveExpression())
          : visitAdditiveExprNode(ctx->conditionBreakAdditiveExpression());
  BinaryOperator lastOp = shiftKinds.front();
  for (std::size_t cursor = 1; cursor < shiftKinds.size(); ++cursor) {
    if (shiftKinds[cursor] == BinaryOperator::ShiftLeft) {
      terminalNode = std::make_unique<BinaryExprNode>(
          helper::getPosition(
              ctx->getStart(),
              ctx->conditionClosedAdditiveExpression(shiftLeftCursor)
                  ->getStop()),
          lastOp, std::move(terminalNode),
          visitAdditiveExprNode(
              ctx->conditionClosedAdditiveExpression(shiftLeftCursor)));
      ++shiftLeftCursor;
    } else {
      terminalNode = std::make_unique<BinaryExprNode>(
          helper::getPosition(
              ctx->getStart(),
              ctx->conditionAdditiveExpression(shiftRightCursor)->getStop()),
          lastOp, std::move(terminalNode),
          visitAdditiveExprNode(
              ctx->conditionAdditiveExpression(shiftRightCursor)));
      ++shiftRightCursor;
    }
    lastOp = shiftKinds[cursor];
  }
  return std::make_unique<BinaryExprNode>(
      helper::getPosition(ctx), lastOp, std::move(terminalNode),
      visitAdditiveExprNode(ctx->conditionClosedAdditiveExpression().back()));
}

std::unique_ptr<ExprNode>
ASTBuilder::visitAdditiveExprNode(RxParser::AdditiveExpressionContext *ctx) {
  if (ctx->additiveOperator().size() == 0) {
    return visitMultiplicativeExprNode(ctx->multiplicativeExpression(0));
  }
  std::size_t numOfOperators = ctx->additiveOperator().size();
  auto terminalNode =
      visitMultiplicativeExprNode(ctx->multiplicativeExpression().front());
  for (std::size_t cursor = 0; cursor < numOfOperators; ++cursor) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(
            ctx->getStart(),
            ctx->multiplicativeExpression(cursor + 1)->getStop()),
        ctx->additiveOperator(cursor)->PLUS() ? BinaryOperator::Add
                                              : BinaryOperator::Sub,
        std::move(terminalNode),
        visitMultiplicativeExprNode(ctx->multiplicativeExpression(cursor + 1)));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitAdditiveExprNode(
    RxParser::ClosedAdditiveExpressionContext *ctx) {
  if (ctx->additiveOperator().size() == 0) {
    return visitMultiplicativeExprNode(ctx->closedMultiplicativeExpression());
  }
  std::size_t numOfOperators = ctx->additiveOperator().size();
  auto terminalNode =
      visitMultiplicativeExprNode(ctx->multiplicativeExpression().front());
  for (std::size_t cursor = 0; cursor < numOfOperators; ++cursor) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(
            ctx->getStart(),
            cursor == numOfOperators - 1
                ? ctx->closedMultiplicativeExpression()->getStop()
                : ctx->multiplicativeExpression(cursor + 1)->getStop()),
        ctx->additiveOperator(cursor)->PLUS() ? BinaryOperator::Add
                                              : BinaryOperator::Sub,
        std::move(terminalNode),
        cursor == numOfOperators - 1
            ? visitMultiplicativeExprNode(ctx->closedMultiplicativeExpression())
            : visitMultiplicativeExprNode(
                  ctx->multiplicativeExpression(cursor + 1)));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitAdditiveExprNode(
    RxParser::StatementAdditiveExpressionContext *ctx) {
  if (ctx->multiplicativeExpression().size() == 0) {
    return visitMultiplicativeExprNode(
        ctx->statementMultiplicativeExpression());
  }
  std::size_t numOfOperators = ctx->additiveOperator().size();
  auto terminalNode =
      visitMultiplicativeExprNode(ctx->statementMultiplicativeExpression());
  for (std::size_t cursor = 0; cursor < numOfOperators; ++cursor) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->multiplicativeExpression(cursor)->getStop()),
        ctx->additiveOperator(cursor)->PLUS() ? BinaryOperator::Add
                                              : BinaryOperator::Sub,
        std::move(terminalNode),
        visitMultiplicativeExprNode(ctx->multiplicativeExpression(cursor)));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitAdditiveExprNode(
    RxParser::StatementClosedAdditiveExpressionContext *ctx) {
  if (ctx->additiveOperator().size() == 0) {
    return visitMultiplicativeExprNode(
        ctx->statementClosedMultiplicativeExpression());
  }
  std::size_t numOfMultiplicativeExpressions =
      ctx->multiplicativeExpression().size();
  auto terminalNode =
      visitMultiplicativeExprNode(ctx->statementMultiplicativeExpression());
  auto appendNode =
      [&](RxParser::AdditiveOperatorContext *op,
          RxParser::ClosedMultiplicativeExpressionContext *closedExpr,
          RxParser::MultiplicativeExpressionContext *expr) {
        BinaryOperator mulOp;
        if (op->PLUS()) {
          mulOp = BinaryOperator::Add;
        }
        if (op->MINUS()) {
          mulOp = BinaryOperator::Sub;
        }
        terminalNode = std::make_unique<BinaryExprNode>(
            helper::getPosition(ctx->getStart(), closedExpr != nullptr
                                                     ? closedExpr->getStop()
                                                     : expr->getStop()),
            mulOp, std::move(terminalNode),
            closedExpr != nullptr ? visitMultiplicativeExprNode(closedExpr)
                                  : visitMultiplicativeExprNode(expr));
      };
  for (std::size_t cursor = 0; cursor < numOfMultiplicativeExpressions;
       ++cursor) {
    appendNode(ctx->additiveOperator(cursor), nullptr,
               ctx->multiplicativeExpression(cursor));
  }
  appendNode(ctx->additiveOperator(numOfMultiplicativeExpressions),
             ctx->closedMultiplicativeExpression(), nullptr);
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitAdditiveExprNode(
    RxParser::ConditionAdditiveExpressionContext *ctx) {
  if (ctx->additiveOperator().size() == 0) {
    return visitMultiplicativeExprNode(
        ctx->conditionMultiplicativeExpression(0));
  }
  std::size_t numOfOperators = ctx->additiveOperator().size();
  auto terminalNode = visitMultiplicativeExprNode(
      ctx->conditionMultiplicativeExpression().front());
  for (std::size_t cursor = 0; cursor < numOfOperators; ++cursor) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(
            ctx->getStart(),
            ctx->conditionMultiplicativeExpression(cursor + 1)->getStop()),
        ctx->additiveOperator(cursor)->PLUS() ? BinaryOperator::Add
                                              : BinaryOperator::Sub,
        std::move(terminalNode),
        visitMultiplicativeExprNode(
            ctx->conditionMultiplicativeExpression(cursor + 1)));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitAdditiveExprNode(
    RxParser::ConditionClosedAdditiveExpressionContext *ctx) {
  if (ctx->additiveOperator().size() == 0) {
    return visitMultiplicativeExprNode(
        ctx->conditionClosedMultiplicativeExpression());
  }
  std::size_t numOfOperators = ctx->additiveOperator().size();
  auto terminalNode = visitMultiplicativeExprNode(
      ctx->conditionMultiplicativeExpression().front());
  for (std::size_t cursor = 0; cursor < numOfOperators; ++cursor) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(
            ctx->getStart(),
            cursor == numOfOperators - 1
                ? ctx->conditionClosedMultiplicativeExpression()->getStop()
                : ctx->conditionMultiplicativeExpression(cursor + 1)
                      ->getStop()),
        ctx->additiveOperator(cursor)->PLUS() ? BinaryOperator::Add
                                              : BinaryOperator::Sub,
        std::move(terminalNode),
        cursor == numOfOperators - 1
            ? visitMultiplicativeExprNode(
                  ctx->conditionClosedMultiplicativeExpression())
            : visitMultiplicativeExprNode(
                  ctx->conditionMultiplicativeExpression(cursor + 1)));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitAdditiveExprNode(
    RxParser::ConditionBreakAdditiveExpressionContext *ctx) {
  auto terminalNode = visitMultiplicativeExprNode(
      ctx->conditionBreakMultiplicativeExpression());
  std::size_t numOfOperators = ctx->additiveOperator().size();
  for (std::size_t cursor = 0; cursor < numOfOperators; ++cursor) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(
            ctx->getStart(),
            ctx->conditionMultiplicativeExpression(cursor)->getStop()),
        ctx->additiveOperator(cursor)->PLUS() ? BinaryOperator::Add
                                              : BinaryOperator::Sub,
        std::move(terminalNode),
        visitMultiplicativeExprNode(
            ctx->conditionMultiplicativeExpression(cursor)));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitAdditiveExprNode(
    RxParser::ConditionBreakClosedAdditiveExpressionContext *ctx) {
  if (ctx->additiveOperator().size() == 0) {
    return visitMultiplicativeExprNode(
        ctx->conditionBreakClosedMultiplicativeExpression());
  }
  auto terminalNode = visitMultiplicativeExprNode(
      ctx->conditionBreakMultiplicativeExpression());
  std::size_t numOfOperators = ctx->additiveOperator().size();
  for (std::size_t cursor = 0; cursor < numOfOperators; ++cursor) {
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(
            ctx->getStart(),
            cursor == numOfOperators - 1
                ? ctx->conditionClosedMultiplicativeExpression()->getStop()
                : ctx->conditionMultiplicativeExpression(cursor)->getStop()),
        ctx->additiveOperator(cursor)->PLUS() ? BinaryOperator::Add
                                              : BinaryOperator::Sub,
        std::move(terminalNode),
        cursor == numOfOperators - 1
            ? visitMultiplicativeExprNode(
                  ctx->conditionClosedMultiplicativeExpression())
            : visitMultiplicativeExprNode(
                  ctx->conditionMultiplicativeExpression(cursor)));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitMultiplicativeExprNode(
    RxParser::MultiplicativeExpressionContext *ctx) {
  if (ctx->multiplicativeOperator().size() == 0) {
    return visitCastExprNode(ctx->castExpression(0));
  }
  std::size_t numOfOperators = ctx->multiplicativeOperator().size();
  auto terminalNode = visitCastExprNode(ctx->castExpression().front());
  for (std::size_t cursor = 0; cursor < numOfOperators; ++cursor) {
    BinaryOperator op;
    if (ctx->multiplicativeOperator(cursor)->STAR()) {
      op = BinaryOperator::Mul;
    }
    if (ctx->multiplicativeOperator(cursor)->SLASH()) {
      op = BinaryOperator::Div;
    }
    if (ctx->multiplicativeOperator(cursor)->PERCENT()) {
      op = BinaryOperator::Mod;
    }
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->castExpression(cursor + 1)->getStop()),
        op, std::move(terminalNode),
        visitCastExprNode(ctx->castExpression(cursor + 1)));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitMultiplicativeExprNode(
    RxParser::ClosedMultiplicativeExpressionContext *ctx) {
  if (ctx->castExpression().size() == 0) {
    return visitCastExprNode(ctx->closedCastExpression());
  }
  std::size_t numOfOperators = ctx->multiplicativeOperator().size();
  auto terminalNode = visitCastExprNode(ctx->castExpression().front());
  for (std::size_t cursor = 0; cursor < numOfOperators; ++cursor) {
    BinaryOperator op;
    if (ctx->multiplicativeOperator(cursor)->STAR()) {
      op = BinaryOperator::Mul;
    }
    if (ctx->multiplicativeOperator(cursor)->SLASH()) {
      op = BinaryOperator::Div;
    }
    if (ctx->multiplicativeOperator(cursor)->PERCENT()) {
      op = BinaryOperator::Mod;
    }
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            cursor == numOfOperators - 1
                                ? ctx->closedCastExpression()->getStop()
                                : ctx->castExpression(cursor + 1)->getStop()),
        op, std::move(terminalNode),
        cursor == numOfOperators - 1
            ? visitCastExprNode(ctx->closedCastExpression())
            : visitCastExprNode(ctx->castExpression(cursor + 1)));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitMultiplicativeExprNode(
    RxParser::StatementMultiplicativeExpressionContext *ctx) {
  if (ctx->castExpression().size() == 0) {
    return visitCastExprNode(ctx->statementCastExpression());
  }
  std::size_t numOfOperators = ctx->multiplicativeOperator().size();
  auto terminalNode = visitCastExprNode(ctx->statementCastExpression());
  for (std::size_t cursor = 0; cursor < numOfOperators; ++cursor) {
    BinaryOperator op;
    if (ctx->multiplicativeOperator(cursor)->STAR()) {
      op = BinaryOperator::Mul;
    }
    if (ctx->multiplicativeOperator(cursor)->SLASH()) {
      op = BinaryOperator::Div;
    }
    if (ctx->multiplicativeOperator(cursor)->PERCENT()) {
      op = BinaryOperator::Mod;
    }
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->castExpression(cursor)->getStop()),
        op, std::move(terminalNode),
        visitCastExprNode(ctx->castExpression(cursor)));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitMultiplicativeExprNode(
    RxParser::StatementClosedMultiplicativeExpressionContext *ctx) {
  if (ctx->multiplicativeOperator().size() == 0) {
    return visitCastExprNode(ctx->statementClosedCastExpression());
  }
  std::size_t numOfCastExpressions = ctx->castExpression().size();
  auto terminalNode = visitCastExprNode(ctx->statementCastExpression());
  auto appendNode = [&](RxParser::MultiplicativeOperatorContext *op,
                        RxParser::ClosedCastExpressionContext *closedExpr,
                        RxParser::CastExpressionContext *expr) {
    BinaryOperator mulOp;
    if (op->STAR()) {
      mulOp = BinaryOperator::Mul;
    }
    if (op->SLASH()) {
      mulOp = BinaryOperator::Div;
    }
    if (op->PERCENT()) {
      mulOp = BinaryOperator::Mod;
    }
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(), closedExpr != nullptr
                                                 ? closedExpr->getStop()
                                                 : expr->getStop()),
        mulOp, std::move(terminalNode),
        closedExpr != nullptr ? visitCastExprNode(closedExpr)
                              : visitCastExprNode(expr));
  };
  for (std::size_t cursor = 0; cursor < numOfCastExpressions; ++cursor) {
    appendNode(ctx->multiplicativeOperator(cursor), nullptr,
               ctx->castExpression(cursor));
  }
  appendNode(ctx->multiplicativeOperator(numOfCastExpressions),
             ctx->closedCastExpression(), nullptr);
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitMultiplicativeExprNode(
    RxParser::ConditionMultiplicativeExpressionContext *ctx) {
  if (ctx->multiplicativeOperator().size() == 0) {
    return visitCastExprNode(ctx->conditionCastExpression(0));
  }
  std::size_t numOfOperators = ctx->multiplicativeOperator().size();
  auto terminalNode = visitCastExprNode(ctx->conditionCastExpression().front());
  for (std::size_t cursor = 0; cursor < numOfOperators; ++cursor) {
    BinaryOperator op;
    if (ctx->multiplicativeOperator(cursor)->STAR()) {
      op = BinaryOperator::Mul;
    }
    if (ctx->multiplicativeOperator(cursor)->SLASH()) {
      op = BinaryOperator::Div;
    }
    if (ctx->multiplicativeOperator(cursor)->PERCENT()) {
      op = BinaryOperator::Mod;
    }
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(
            ctx->getStart(),
            ctx->conditionCastExpression(cursor + 1)->getStop()),
        op, std::move(terminalNode),
        visitCastExprNode(ctx->conditionCastExpression(cursor + 1)));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitMultiplicativeExprNode(
    RxParser::ConditionClosedMultiplicativeExpressionContext *ctx) {
  if (ctx->multiplicativeOperator().size() == 0) {
    return visitCastExprNode(ctx->conditionClosedCastExpression());
  }
  std::size_t numOfOperators = ctx->multiplicativeOperator().size();
  auto terminalNode = visitCastExprNode(ctx->conditionCastExpression().front());
  for (std::size_t cursor = 0; cursor < numOfOperators; ++cursor) {
    BinaryOperator op;
    if (ctx->multiplicativeOperator(cursor)->STAR()) {
      op = BinaryOperator::Mul;
    }
    if (ctx->multiplicativeOperator(cursor)->SLASH()) {
      op = BinaryOperator::Div;
    }
    if (ctx->multiplicativeOperator(cursor)->PERCENT()) {
      op = BinaryOperator::Mod;
    }
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(
            ctx->getStart(),
            cursor == numOfOperators - 1
                ? ctx->conditionClosedCastExpression()->getStop()
                : ctx->conditionCastExpression(cursor + 1)->getStop()),
        op, std::move(terminalNode),
        cursor == numOfOperators - 1
            ? visitCastExprNode(ctx->conditionClosedCastExpression())
            : visitCastExprNode(ctx->conditionCastExpression(cursor + 1)));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitMultiplicativeExprNode(
    RxParser::ConditionBreakMultiplicativeExpressionContext *ctx) {
  auto terminalNode = visitCastExprNode(ctx->conditionBreakCastExpression());
  std::size_t numOfOperators = ctx->multiplicativeOperator().size();
  for (std::size_t cursor = 0; cursor < numOfOperators; ++cursor) {
    BinaryOperator op;
    if (ctx->multiplicativeOperator(cursor)->STAR()) {
      op = BinaryOperator::Mul;
    }
    if (ctx->multiplicativeOperator(cursor)->SLASH()) {
      op = BinaryOperator::Div;
    }
    if (ctx->multiplicativeOperator(cursor)->PERCENT()) {
      op = BinaryOperator::Mod;
    }
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(ctx->getStart(),
                            ctx->conditionCastExpression(cursor)->getStop()),
        op, std::move(terminalNode),
        visitCastExprNode(ctx->conditionCastExpression(cursor)));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitMultiplicativeExprNode(
    RxParser::ConditionBreakClosedMultiplicativeExpressionContext *ctx) {
  if (ctx->multiplicativeOperator().size() == 0) {
    return visitCastExprNode(ctx->conditionBreakClosedCastExpression());
  }
  auto terminalNode = visitCastExprNode(ctx->conditionBreakCastExpression());
  std::size_t numOfOperators = ctx->multiplicativeOperator().size();
  for (std::size_t cursor = 0; cursor < numOfOperators; ++cursor) {
    BinaryOperator op;
    if (ctx->multiplicativeOperator(cursor)->STAR()) {
      op = BinaryOperator::Mul;
    }
    if (ctx->multiplicativeOperator(cursor)->SLASH()) {
      op = BinaryOperator::Div;
    }
    if (ctx->multiplicativeOperator(cursor)->PERCENT()) {
      op = BinaryOperator::Mod;
    }
    terminalNode = std::make_unique<BinaryExprNode>(
        helper::getPosition(
            ctx->getStart(),
            cursor == numOfOperators - 1
                ? ctx->conditionClosedCastExpression()->getStop()
                : ctx->conditionCastExpression(cursor)->getStop()),
        op, std::move(terminalNode),
        cursor == numOfOperators - 1
            ? visitCastExprNode(ctx->conditionClosedCastExpression())
            : visitCastExprNode(ctx->conditionCastExpression(cursor)));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode>
ASTBuilder::visitCastExprNode(RxParser::CastExpressionContext *ctx) {
  auto terminalNode = visitUnaryExprNode(ctx->unaryExpression());
  for (auto *type : ctx->typeRef()) {
    terminalNode = std::make_unique<CastExprNode>(
        helper::getPosition(ctx->getStart(), type->getStop()),
        visitTypeNode(type), std::move(terminalNode));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode>
ASTBuilder::visitCastExprNode(RxParser::ClosedCastExpressionContext *ctx) {
  if (auto *unary = ctx->unaryExpression()) {
    return visitUnaryExprNode(unary);
  }
  return std::make_unique<CastExprNode>(
      helper::getPosition(ctx), visitClosedCastTypeNode(ctx->closedCastType()),
      visitCastExprNode(ctx->castExpression()));
}

std::unique_ptr<ExprNode>
ASTBuilder::visitCastExprNode(RxParser::StatementCastExpressionContext *ctx) {
  auto terminalNode = visitUnaryExprNode(ctx->statementUnaryExpression());
  for (auto *type : ctx->typeRef()) {
    terminalNode = std::make_unique<CastExprNode>(
        helper::getPosition(ctx->getStart(), type->getStop()),
        visitTypeNode(type), std::move(terminalNode));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitCastExprNode(
    RxParser::StatementClosedCastExpressionContext *ctx) {
  if (auto *unary = ctx->statementUnaryExpression()) {
    return visitUnaryExprNode(unary);
  }
  return std::make_unique<CastExprNode>(
      helper::getPosition(ctx), visitClosedCastTypeNode(ctx->closedCastType()),
      visitCastExprNode(ctx->statementCastExpression()));
}

std::unique_ptr<ExprNode>
ASTBuilder::visitCastExprNode(RxParser::ConditionCastExpressionContext *ctx) {
  auto terminalNode = visitUnaryExprNode(ctx->conditionUnaryExpression());
  for (auto *type : ctx->typeRef()) {
    terminalNode = std::make_unique<CastExprNode>(
        helper::getPosition(ctx->getStart(), type->getStop()),
        visitTypeNode(type), std::move(terminalNode));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitCastExprNode(
    RxParser::ConditionClosedCastExpressionContext *ctx) {
  if (auto *unary = ctx->conditionUnaryExpression()) {
    return visitUnaryExprNode(unary);
  }
  return std::make_unique<CastExprNode>(
      helper::getPosition(ctx), visitClosedCastTypeNode(ctx->closedCastType()),
      visitCastExprNode(ctx->conditionCastExpression()));
}

std::unique_ptr<ExprNode> ASTBuilder::visitCastExprNode(
    RxParser::ConditionBreakCastExpressionContext *ctx) {
  auto terminalNode = visitUnaryExprNode(ctx->conditionBreakUnaryExpression());
  for (auto *type : ctx->typeRef()) {
    terminalNode = std::make_unique<CastExprNode>(
        helper::getPosition(ctx->getStart(), type->getStop()),
        visitTypeNode(type), std::move(terminalNode));
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitCastExprNode(
    RxParser::ConditionBreakClosedCastExpressionContext *ctx) {
  if (auto *unary = ctx->conditionBreakUnaryExpression()) {
    return visitUnaryExprNode(unary);
  }
  return std::make_unique<CastExprNode>(
      helper::getPosition(ctx), visitClosedCastTypeNode(ctx->closedCastType()),
      visitCastExprNode(ctx->conditionBreakCastExpression()));
}

std::unique_ptr<TypeNode>
ASTBuilder::visitClosedCastTypeNode(RxParser::ClosedCastTypeContext *ctx) {
  const auto pos = helper::getPosition(ctx);
  if (ctx->LPAREN()) {
    if (ctx->typeRef()) {
      return visitTypeNode(ctx->typeRef());
    }
    return std::make_unique<UnitTypeNode>(pos);
  }
  if (ctx->arrayType()) {
    return visitArrayTypeNode(ctx->arrayType());
  }
  if (ctx->AMP()) {
    return std::make_unique<ReferenceTypeNode>(
        helper::getPosition(ctx), ctx->MUT() != nullptr,
        visitClosedCastTypeNode(ctx->closedCastType()));
  } else if (ctx->ANDAND()) {
    const auto ampPos = helper::getPosition(ctx->ANDAND());
    auto innerPos = pos;
    innerPos.startLine = ampPos.startLine;
    innerPos.startColumn = ampPos.startColumn + 1;
    return std::make_unique<ReferenceTypeNode>(
        pos, false,
        std::make_unique<ReferenceTypeNode>(
            innerPos, ctx->MUT() != nullptr,
            visitClosedCastTypeNode(ctx->closedCastType())));
  }

  auto path = std::make_unique<PathInTypeNode>(pos);
  auto appendSegment = [&](RxParser::PathIdentSegmentContext *ident,
                           RxParser::GenericArgsContext *args) {
    PathSegment segment;
    segment.pathIdentSegment = ident->getText();
    segment.args = visitGenericArgsNode(args);
    path->path.segments.push_back(std::move(segment));
  };
  for (auto *segment : ctx->typePathSegment()) {
    appendSegment(segment->pathIdentSegment(), segment->genericArgs());
  }
  appendSegment(ctx->pathIdentSegment(), ctx->genericArgs());
  return path;
}

std::unique_ptr<ExprNode>
ASTBuilder::visitUnaryExprNode(RxParser::UnaryExpressionContext *ctx) {
  if (ctx->unaryOperator()) {
    auto *unaryOp = ctx->unaryOperator();
    if (unaryOp->AMP()) {
      return std::make_unique<BorrowExprNode>(
          helper::getPosition(ctx), unaryOp->MUT() != nullptr,
          visitUnaryExprNode(ctx->unaryExpression()));
    }
    if (unaryOp->ANDAND()) {
      const auto outerPos = helper::getPosition(ctx);
      const auto ampPos = helper::getPosition(unaryOp->ANDAND());
      auto innerPos = outerPos;
      innerPos.startLine = ampPos.startLine;
      innerPos.startColumn = ampPos.startColumn + 1;
      return std::make_unique<BorrowExprNode>(
          outerPos, false,
          std::make_unique<BorrowExprNode>(
              innerPos, unaryOp->MUT() != nullptr,
              visitUnaryExprNode(ctx->unaryExpression())));
    }
    if (unaryOp->MINUS()) {
      return std::make_unique<UnaryExprNode>(
          helper::getPosition(ctx), UnaryOperator::NEG,
          visitUnaryExprNode(ctx->unaryExpression()));
    }
    if (unaryOp->NOT()) {
      return std::make_unique<UnaryExprNode>(
          helper::getPosition(ctx), UnaryOperator::NOT,
          visitUnaryExprNode(ctx->unaryExpression()));
    }
    if (unaryOp->STAR()) {
      return std::make_unique<DereferenceExprNode>(
          helper::getPosition(ctx), visitUnaryExprNode(ctx->unaryExpression()));
    }
    return nullptr;
  } else {
    return visitPostfixExprNode(ctx->postfixExpression());
  }
}

std::unique_ptr<ExprNode>
ASTBuilder::visitUnaryExprNode(RxParser::StatementUnaryExpressionContext *ctx) {
  if (ctx->unaryOperator()) {
    auto *unaryOp = ctx->unaryOperator();
    if (unaryOp->AMP()) {
      return std::make_unique<BorrowExprNode>(
          helper::getPosition(ctx), unaryOp->MUT() != nullptr,
          visitUnaryExprNode(ctx->unaryExpression()));
    }
    if (unaryOp->ANDAND()) {
      const auto outerPos = helper::getPosition(ctx);
      const auto ampPos = helper::getPosition(unaryOp->ANDAND());
      auto innerPos = outerPos;
      innerPos.startLine = ampPos.startLine;
      innerPos.startColumn = ampPos.startColumn + 1;
      return std::make_unique<BorrowExprNode>(
          outerPos, false,
          std::make_unique<BorrowExprNode>(
              innerPos, unaryOp->MUT() != nullptr,
              visitUnaryExprNode(ctx->unaryExpression())));
    }
    if (unaryOp->MINUS()) {
      return std::make_unique<UnaryExprNode>(
          helper::getPosition(ctx), UnaryOperator::NEG,
          visitUnaryExprNode(ctx->unaryExpression()));
    }
    if (unaryOp->NOT()) {
      return std::make_unique<UnaryExprNode>(
          helper::getPosition(ctx), UnaryOperator::NOT,
          visitUnaryExprNode(ctx->unaryExpression()));
    }
    if (unaryOp->STAR()) {
      return std::make_unique<DereferenceExprNode>(
          helper::getPosition(ctx), visitUnaryExprNode(ctx->unaryExpression()));
    }
    return nullptr;
  } else {
    return visitPostfixExprNode(ctx->statementPostfixExpression());
  }
}

std::unique_ptr<ExprNode>
ASTBuilder::visitUnaryExprNode(RxParser::ConditionUnaryExpressionContext *ctx) {
  if (ctx->unaryOperator()) {
    auto *unaryOp = ctx->unaryOperator();
    if (unaryOp->AMP()) {
      return std::make_unique<BorrowExprNode>(
          helper::getPosition(ctx), unaryOp->MUT() != nullptr,
          visitUnaryExprNode(ctx->conditionUnaryExpression()));
    }
    if (unaryOp->ANDAND()) {
      const auto outerPos = helper::getPosition(ctx);
      const auto ampPos = helper::getPosition(unaryOp->ANDAND());
      auto innerPos = outerPos;
      innerPos.startLine = ampPos.startLine;
      innerPos.startColumn = ampPos.startColumn + 1;
      return std::make_unique<BorrowExprNode>(
          outerPos, false,
          std::make_unique<BorrowExprNode>(
              innerPos, unaryOp->MUT() != nullptr,
              visitUnaryExprNode(ctx->conditionUnaryExpression())));
    }
    if (unaryOp->MINUS()) {
      return std::make_unique<UnaryExprNode>(
          helper::getPosition(ctx), UnaryOperator::NEG,
          visitUnaryExprNode(ctx->conditionUnaryExpression()));
    }
    if (unaryOp->NOT()) {
      return std::make_unique<UnaryExprNode>(
          helper::getPosition(ctx), UnaryOperator::NOT,
          visitUnaryExprNode(ctx->conditionUnaryExpression()));
    }
    if (unaryOp->STAR()) {
      return std::make_unique<DereferenceExprNode>(
          helper::getPosition(ctx),
          visitUnaryExprNode(ctx->conditionUnaryExpression()));
    }
    return nullptr;
  } else {
    return visitPostfixExprNode(ctx->conditionPostfixExpression());
  }
}

std::unique_ptr<ExprNode> ASTBuilder::visitUnaryExprNode(
    RxParser::ConditionBreakUnaryExpressionContext *ctx) {
  if (ctx->unaryOperator()) {
    auto *unaryOp = ctx->unaryOperator();
    if (unaryOp->AMP()) {
      return std::make_unique<BorrowExprNode>(
          helper::getPosition(ctx), unaryOp->MUT() != nullptr,
          visitUnaryExprNode(ctx->conditionUnaryExpression()));
    }
    if (unaryOp->ANDAND()) {
      const auto outerPos = helper::getPosition(ctx);
      const auto ampPos = helper::getPosition(unaryOp->ANDAND());
      auto innerPos = outerPos;
      innerPos.startLine = ampPos.startLine;
      innerPos.startColumn = ampPos.startColumn + 1;
      return std::make_unique<BorrowExprNode>(
          outerPos, false,
          std::make_unique<BorrowExprNode>(
              innerPos, unaryOp->MUT() != nullptr,
              visitUnaryExprNode(ctx->conditionUnaryExpression())));
    }
    if (unaryOp->MINUS()) {
      return std::make_unique<UnaryExprNode>(
          helper::getPosition(ctx), UnaryOperator::NEG,
          visitUnaryExprNode(ctx->conditionUnaryExpression()));
    }
    if (unaryOp->NOT()) {
      return std::make_unique<UnaryExprNode>(
          helper::getPosition(ctx), UnaryOperator::NOT,
          visitUnaryExprNode(ctx->conditionUnaryExpression()));
    }
    if (unaryOp->STAR()) {
      return std::make_unique<DereferenceExprNode>(
          helper::getPosition(ctx),
          visitUnaryExprNode(ctx->conditionUnaryExpression()));
    }
    return nullptr;
  } else {
    return visitPostfixExprNode(ctx->conditionBreakPostfixExpression());
  }
}

std::unique_ptr<ExprNode>
ASTBuilder::visitPostfixExprNode(RxParser::PostfixExpressionContext *ctx) {
  auto terminalNode = visitPrimaryExprNode(ctx->primaryExpression());
  for (auto *suffix : ctx->postfixSuffix()) {
    if (suffix->expression()) {
      terminalNode = std::make_unique<IndexExprNode>(
          helper::getPosition(ctx->getStart(), suffix->getStop()),
          std::move(terminalNode), visitExprNode(suffix->expression()));
    }
    if (suffix->callArguments()) {
      terminalNode = std::make_unique<CallExprNode>(
          helper::getPosition(ctx->getStart(), suffix->getStop()),
          std::move(terminalNode));
      auto *derivedNode = dynamic_cast<CallExprNode *>(terminalNode.get());
      for (auto *param : suffix->callArguments()->expression()) {
        derivedNode->callParams.push_back(visitExprNode(param));
      }
    }
    if (suffix->dotSuffix()) {
      if (suffix->dotSuffix()->identifier()) {
        terminalNode = std::make_unique<AccessExprNode>(
            helper::getPosition(ctx->getStart(), suffix->getStop()),
            std::move(terminalNode),
            suffix->dotSuffix()->identifier()->getText());
      } else {
        auto *segment = suffix->dotSuffix()->pathExprSegment();
        PathSegment seg;
        seg.pathIdentSegment = segment->pathIdentSegment()->getText();
        seg.args = visitGenericArgsNode(segment->genericArgs());
        auto call = std::make_unique<MethodCallExprNode>(
            helper::getPosition(ctx->getStart(), suffix->getStop()),
            std::move(terminalNode), std::move(seg));
        for (auto *param : suffix->dotSuffix()->callArguments()->expression()) {
          call->callParams.push_back(visitExprNode(param));
        }
        terminalNode = std::move(call);
      }
    }
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitPostfixExprNode(
    RxParser::StatementPostfixExpressionContext *ctx) {
  if (auto *primary = ctx->nonBlockPrimary(); primary) {
    auto terminalNode = visitNonBlockPrimaryExprNode(primary);
    for (auto *suffix : ctx->postfixSuffix()) {
      if (suffix->expression()) {
        terminalNode = std::make_unique<IndexExprNode>(
            helper::getPosition(ctx->getStart(), suffix->getStop()),
            std::move(terminalNode), visitExprNode(suffix->expression()));
      }
      if (suffix->callArguments()) {
        terminalNode = std::make_unique<CallExprNode>(
            helper::getPosition(ctx->getStart(), suffix->getStop()),
            std::move(terminalNode));
        auto *derivedNode = dynamic_cast<CallExprNode *>(terminalNode.get());
        for (auto *param : suffix->callArguments()->expression()) {
          derivedNode->callParams.push_back(visitExprNode(param));
        }
      }
      if (suffix->dotSuffix()) {
        if (suffix->dotSuffix()->identifier()) {
          terminalNode = std::make_unique<AccessExprNode>(
              helper::getPosition(ctx->getStart(), suffix->getStop()),
              std::move(terminalNode),
              suffix->dotSuffix()->identifier()->getText());
        } else {
          auto *segment = suffix->dotSuffix()->pathExprSegment();
          PathSegment seg;
          seg.pathIdentSegment = segment->pathIdentSegment()->getText();
          seg.args = visitGenericArgsNode(segment->genericArgs());
          auto call = std::make_unique<MethodCallExprNode>(
              helper::getPosition(ctx->getStart(), suffix->getStop()),
              std::move(terminalNode), std::move(seg));
          for (auto *param :
               suffix->dotSuffix()->callArguments()->expression()) {
            call->callParams.push_back(visitExprNode(param));
          }
          terminalNode = std::move(call);
        }
      }
    }
    return terminalNode;
  } else {
    auto terminalNode = visitWithBlockExprNode(ctx->expressionWithBlock());
    if (ctx->dotSuffix()->identifier()) {
      terminalNode = std::make_unique<AccessExprNode>(
          helper::getPosition(ctx->getStart(), ctx->dotSuffix()->getStop()),
          std::move(terminalNode), ctx->dotSuffix()->identifier()->getText());
    } else {
      auto *segment = ctx->dotSuffix()->pathExprSegment();
      PathSegment seg;
      seg.pathIdentSegment = segment->pathIdentSegment()->getText();
      seg.args = visitGenericArgsNode(segment->genericArgs());
      auto call = std::make_unique<MethodCallExprNode>(
          helper::getPosition(ctx->getStart(), ctx->dotSuffix()->getStop()),
          std::move(terminalNode), std::move(seg));
      for (auto *param : ctx->dotSuffix()->callArguments()->expression()) {
        call->callParams.push_back(visitExprNode(param));
      }
      terminalNode = std::move(call);
    }
    for (auto *suffix : ctx->postfixSuffix()) {
      if (suffix->expression()) {
        terminalNode = std::make_unique<IndexExprNode>(
            helper::getPosition(ctx->getStart(), suffix->getStop()),
            std::move(terminalNode), visitExprNode(suffix->expression()));
      }
      if (suffix->callArguments()) {
        terminalNode = std::make_unique<CallExprNode>(
            helper::getPosition(ctx->getStart(), suffix->getStop()),
            std::move(terminalNode));
        auto *derivedNode = dynamic_cast<CallExprNode *>(terminalNode.get());
        for (auto *param : suffix->callArguments()->expression()) {
          derivedNode->callParams.push_back(visitExprNode(param));
        }
      }
      if (suffix->dotSuffix()) {
        if (suffix->dotSuffix()->identifier()) {
          terminalNode = std::make_unique<AccessExprNode>(
              helper::getPosition(ctx->getStart(), suffix->getStop()),
              std::move(terminalNode),
              suffix->dotSuffix()->identifier()->getText());
        } else {
          auto *segment = suffix->dotSuffix()->pathExprSegment();
          PathSegment seg;
          seg.pathIdentSegment = segment->pathIdentSegment()->getText();
          seg.args = visitGenericArgsNode(segment->genericArgs());
          auto call = std::make_unique<MethodCallExprNode>(
              helper::getPosition(ctx->getStart(), suffix->getStop()),
              std::move(terminalNode), std::move(seg));
          for (auto *param :
               suffix->dotSuffix()->callArguments()->expression()) {
            call->callParams.push_back(visitExprNode(param));
          }
          terminalNode = std::move(call);
        }
      }
    }
    return terminalNode;
  }
}

std::unique_ptr<ExprNode> ASTBuilder::visitPostfixExprNode(
    RxParser::ConditionPostfixExpressionContext *ctx) {
  auto terminalNode = visitPrimaryExprNode(ctx->conditionPrimary());
  for (auto *suffix : ctx->postfixSuffix()) {
    if (suffix->expression()) {
      terminalNode = std::make_unique<IndexExprNode>(
          helper::getPosition(ctx->getStart(), suffix->getStop()),
          std::move(terminalNode), visitExprNode(suffix->expression()));
    }
    if (suffix->callArguments()) {
      terminalNode = std::make_unique<CallExprNode>(
          helper::getPosition(ctx->getStart(), suffix->getStop()),
          std::move(terminalNode));
      auto *derivedNode = dynamic_cast<CallExprNode *>(terminalNode.get());
      for (auto *param : suffix->callArguments()->expression()) {
        derivedNode->callParams.push_back(visitExprNode(param));
      }
    }
    if (suffix->dotSuffix()) {
      if (suffix->dotSuffix()->identifier()) {
        terminalNode = std::make_unique<AccessExprNode>(
            helper::getPosition(ctx->getStart(), suffix->getStop()),
            std::move(terminalNode),
            suffix->dotSuffix()->identifier()->getText());
      } else {
        auto *segment = suffix->dotSuffix()->pathExprSegment();
        PathSegment seg;
        seg.pathIdentSegment = segment->pathIdentSegment()->getText();
        seg.args = visitGenericArgsNode(segment->genericArgs());
        auto call = std::make_unique<MethodCallExprNode>(
            helper::getPosition(ctx->getStart(), suffix->getStop()),
            std::move(terminalNode), std::move(seg));
        for (auto *param : suffix->dotSuffix()->callArguments()->expression()) {
          call->callParams.push_back(visitExprNode(param));
        }
        terminalNode = std::move(call);
      }
    }
  }
  return terminalNode;
}

std::unique_ptr<ExprNode> ASTBuilder::visitPostfixExprNode(
    RxParser::ConditionBreakPostfixExpressionContext *ctx) {
  auto terminalNode = visitConditionPrimaryWithoutBareBlockExprNode(
      ctx->conditionPrimaryWithoutBareBlock());
  for (auto *suffix : ctx->postfixSuffix()) {
    if (suffix->expression()) {
      terminalNode = std::make_unique<IndexExprNode>(
          helper::getPosition(ctx->getStart(), suffix->getStop()),
          std::move(terminalNode), visitExprNode(suffix->expression()));
    }
    if (suffix->callArguments()) {
      auto call = std::make_unique<CallExprNode>(
          helper::getPosition(ctx->getStart(), suffix->getStop()),
          std::move(terminalNode));
      for (auto *param : suffix->callArguments()->expression()) {
        call->callParams.push_back(visitExprNode(param));
      }
      terminalNode = std::move(call);
    }
    if (auto *dot = suffix->dotSuffix()) {
      if (dot->identifier()) {
        terminalNode = std::make_unique<AccessExprNode>(
            helper::getPosition(ctx->getStart(), suffix->getStop()),
            std::move(terminalNode), dot->identifier()->getText());
      } else {
        auto *segment = dot->pathExprSegment();
        PathSegment seg;
        seg.pathIdentSegment = segment->pathIdentSegment()->getText();
        seg.args = visitGenericArgsNode(segment->genericArgs());
        auto call = std::make_unique<MethodCallExprNode>(
            helper::getPosition(ctx->getStart(), suffix->getStop()),
            std::move(terminalNode), std::move(seg));
        for (auto *param : dot->callArguments()->expression()) {
          call->callParams.push_back(visitExprNode(param));
        }
        terminalNode = std::move(call);
      }
    }
  }
  return terminalNode;
}

std::unique_ptr<ExprNode>
ASTBuilder::visitPrimaryExprNode(RxParser::PrimaryExpressionContext *ctx) {
  if (ctx->nonBlockPrimary()) {
    return visitNonBlockPrimaryExprNode(ctx->nonBlockPrimary());
  } else {
    return visitWithBlockExprNode(ctx->expressionWithBlock());
  }
}

std::unique_ptr<ExprNode>
ASTBuilder::visitPrimaryExprNode(RxParser::ConditionPrimaryContext *ctx) {
  if (ctx->conditionPrimaryWithoutBareBlock()) {
    return visitConditionPrimaryWithoutBareBlockExprNode(
        ctx->conditionPrimaryWithoutBareBlock());
  } else if (ctx->blockExpression()) {
    return visitBlockExprNode(ctx->blockExpression());
  }
  return nullptr;
}

std::unique_ptr<ExprNode>
ASTBuilder::visitConditionPrimaryWithoutBareBlockExprNode(
    RxParser::ConditionPrimaryWithoutBareBlockContext *ctx) {
  if (ctx->literalExpression()) {
    std::string literal;
    if (ctx->literalExpression()->INTEGER_LITERAL())
      literal = ctx->literalExpression()->INTEGER_LITERAL()->getText();
    else if (ctx->literalExpression()->TRUE())
      literal = ctx->literalExpression()->TRUE()->getText();
    else if (ctx->literalExpression()->FALSE())
      literal = ctx->literalExpression()->FALSE()->getText();
    return std::make_unique<LiteralExprNode>(helper::getPosition(ctx), literal);
  } else if (ctx->arrayExpression()) {
    auto array = std::make_unique<ArrayExprNode>(helper::getPosition(ctx));
    if (ctx->arrayExpression()->constValue()) {
      array->arrayEntries =
          std::pair{visitExprNode(ctx->arrayExpression()->expression(0)),
                    visitConstExprNode(ctx->arrayExpression()->constValue())};
    } else {
      std::vector<std::unique_ptr<ExprNode>> entries;
      for (auto *expr : ctx->arrayExpression()->expression()) {
        entries.push_back(visitExprNode(expr));
      }
      array->arrayEntries = std::move(entries);
    }
    return array;
  } else if (ctx->pathInExpression()) {
    return visitPathInExprNode(ctx->pathInExpression());
  } else if (ctx->LPAREN() && ctx->RPAREN() && !ctx->expression()) {
    return std::make_unique<UnitExprNode>(helper::getPosition(ctx));
  } else if (ctx->BREAK()) {
    const auto &pos = helper::getPosition(ctx);
    return ctx->conditionBreakExpression()
               ? std::make_unique<BreakExprNode>(
                     pos, visitExprNode(ctx->conditionBreakExpression()))
               : std::make_unique<BreakExprNode>(pos);
  } else if (ctx->RETURN()) {
    const auto &pos = helper::getPosition(ctx);
    return ctx->conditionExpression()
               ? std::make_unique<ReturnExprNode>(
                     pos, visitExprNode(ctx->conditionExpression()))
               : std::make_unique<ReturnExprNode>(pos);
  } else if (ctx->LPAREN() && ctx->RPAREN() && ctx->expression()) {
    return visitExprNode(ctx->expression());
  } else if (ctx->ifExpression()) {
    return visitIfExprNode(ctx->ifExpression());
  } else if (ctx->WHILE() || ctx->LOOP()) {
    return visitLoopExprNode(ctx);
  } else if (ctx->CONTINUE()) {
    return std::make_unique<ContinueExprNode>(helper::getPosition(ctx));
  }
  return nullptr;
}

std::unique_ptr<ExprNode> ASTBuilder::visitNonBlockPrimaryExprNode(
    RxParser::NonBlockPrimaryContext *ctx) {
  if (ctx->literalExpression()) {
    std::string literal;
    if (ctx->literalExpression()->INTEGER_LITERAL())
      literal = ctx->literalExpression()->INTEGER_LITERAL()->getText();
    else if (ctx->literalExpression()->TRUE())
      literal = ctx->literalExpression()->TRUE()->getText();
    else if (ctx->literalExpression()->FALSE())
      literal = ctx->literalExpression()->FALSE()->getText();
    return std::make_unique<LiteralExprNode>(helper::getPosition(ctx), literal);
  } else if (ctx->arrayExpression()) {
    auto array = std::make_unique<ArrayExprNode>(helper::getPosition(ctx));
    if (ctx->arrayExpression()->constValue()) {
      array->arrayEntries =
          std::pair{visitExprNode(ctx->arrayExpression()->expression(0)),
                    visitConstExprNode(ctx->arrayExpression()->constValue())};
    } else {
      std::vector<std::unique_ptr<ExprNode>> entries;
      for (auto *expr : ctx->arrayExpression()->expression()) {
        entries.push_back(visitExprNode(expr));
      }
      array->arrayEntries = std::move(entries);
    }
    return array;
  } else if (ctx->pathInExpression()) {
    if (ctx->LBRACE()) {
      auto structExpr = std::make_unique<StructExprNode>(
          helper::getPosition(ctx),
          visitPathInExprNode(ctx->pathInExpression()));
      if (auto *fields = ctx->structExprFields(); fields) {
        for (auto *exprField : fields->structExprField()) {
          structExprField field;
          field.fieldExpr = visitExprNode(exprField->expression());
          field.identifier = exprField->identifier()->getText();
          structExpr->structExprFields.push_back(std::move(field));
        }
      }
      return structExpr;
    } else {
      return visitPathInExprNode(ctx->pathInExpression());
    }
  } else if (ctx->LPAREN() && ctx->RPAREN() && !ctx->expression()) {
    return std::make_unique<UnitExprNode>(helper::getPosition(ctx));
  } else if (ctx->BREAK()) {
    const auto &pos = helper::getPosition(ctx);
    return ctx->expression() ? std::make_unique<BreakExprNode>(
                                   pos, visitExprNode(ctx->expression()))
                             : std::make_unique<BreakExprNode>(pos);
  } else if (ctx->RETURN()) {
    const auto &pos = helper::getPosition(ctx);
    return ctx->expression() ? std::make_unique<ReturnExprNode>(
                                   pos, visitExprNode(ctx->expression()))
                             : std::make_unique<ReturnExprNode>(pos);
  } else if (ctx->LPAREN() && ctx->RPAREN() && ctx->expression()) {
    return visitExprNode(ctx->expression());
  } else if (ctx->CONTINUE()) {
    return std::make_unique<ContinueExprNode>(helper::getPosition(ctx));
  }
  return nullptr;
}

std::unique_ptr<ConstExprNode>
ASTBuilder::visitConstExprNode(RxParser::ConstValueContext *ctx) {
  auto constExpr = std::make_unique<ConstExprNode>(helper::getPosition(ctx),
                                                   ctx->MINUS() != nullptr);
  std::string literal;
  if (ctx->INTEGER_LITERAL()) {
    literal = ctx->INTEGER_LITERAL()->getText();
    constExpr->constValue =
        std::make_unique<LiteralExprNode>(helper::getPosition(ctx), literal);
  } else if (ctx->TRUE()) {
    literal = ctx->TRUE()->getText();
    constExpr->constValue =
        std::make_unique<LiteralExprNode>(helper::getPosition(ctx), literal);
  } else if (ctx->FALSE()) {
    literal = ctx->FALSE()->getText();
    constExpr->constValue =
        std::make_unique<LiteralExprNode>(helper::getPosition(ctx), literal);
  } else if (ctx->pathInExpression()) {
    constExpr->constValue = visitPathInExprNode(ctx->pathInExpression());
  } else if (ctx->magnitude()) {
    constExpr->constValue = visitMagnitudeExprNode(ctx->magnitude());
  } else if (ctx->constValue()) {
    return visitConstExprNode(ctx->constValue());
  }
  return constExpr;
}

std::unique_ptr<MagnitudeExprNode>
ASTBuilder::visitMagnitudeExprNode(RxParser::MagnitudeContext *ctx) {
  auto magnitudeExpr =
      std::make_unique<MagnitudeExprNode>(helper::getPosition(ctx));
  std::string literal;
  if (ctx->INTEGER_LITERAL()) {
    literal = ctx->INTEGER_LITERAL()->getText();
    magnitudeExpr->magnitude = literal;
  } else if (ctx->pathInExpression()) {
    magnitudeExpr->magnitude = visitPathInExprNode(ctx->pathInExpression());
  } else if (ctx->magnitude()) {
    return visitMagnitudeExprNode(ctx->magnitude());
  }
  return magnitudeExpr;
}

std::unique_ptr<IfExprNode>
ASTBuilder::visitIfExprNode(RxParser::IfExpressionContext *ctx) {
  auto ifExpr = std::make_unique<IfExprNode>(
      helper::getPosition(ctx), visitExprNode(ctx->conditionExpression()),
      visitBlockExprNode(ctx->blockExpression(0)));
  if (ctx->ELSE()) {
    if (ctx->ifExpression()) {
      ifExpr->elseIfExpr = std::move(visitIfExprNode(ctx->ifExpression()));
    } else {
      ifExpr->elseExpr = std::move(visitBlockExprNode(ctx->blockExpression(1)));
    }
  }
  return ifExpr;
}

std::unique_ptr<ExprNode>
ASTBuilder::visitWithBlockExprNode(RxParser::ExpressionWithBlockContext *ctx) {
  if (ctx->blockExpression() && !ctx->LOOP() && !ctx->WHILE()) {
    return visitBlockExprNode(ctx->blockExpression());
  } else if (ctx->ifExpression()) {
    return visitIfExprNode(ctx->ifExpression());
  } else {
    return visitLoopExprNode(ctx);
  }
}

std::unique_ptr<ExprNode>
ASTBuilder::visitLoopExprNode(RxParser::ExpressionWithBlockContext *ctx) {
  if (ctx->LOOP()) {
    auto infinite =
        std::make_unique<InfiniteLoopExprNode>(helper::getPosition(ctx));
    infinite->body = visitBlockExprNode(ctx->blockExpression());
    return infinite;
  } else {
    auto predicate =
        std::make_unique<PredicateLoopExprNode>(helper::getPosition(ctx));
    predicate->body = visitBlockExprNode(ctx->blockExpression());
    predicate->conditions = visitExprNode(ctx->conditionExpression());
    return predicate;
  }
}

std::unique_ptr<ExprNode> ASTBuilder::visitLoopExprNode(
    RxParser::ConditionPrimaryWithoutBareBlockContext *ctx) {
  if (ctx->LOOP()) {
    auto infinite =
        std::make_unique<InfiniteLoopExprNode>(helper::getPosition(ctx));
    infinite->body = visitBlockExprNode(ctx->blockExpression());
    return infinite;
  } else {
    auto predicate =
        std::make_unique<PredicateLoopExprNode>(helper::getPosition(ctx));
    predicate->body = visitBlockExprNode(ctx->blockExpression());
    predicate->conditions = visitExprNode(ctx->conditionExpression());
    return predicate;
  }
}

std::unique_ptr<StmtNode>
ASTBuilder::visitStmtNode(RxParser::StatementContext *ctx) {
  if (ctx->SEMI() && !ctx->statementExpression() &&
      !ctx->expressionWithBlock() && !ctx->letStatement()) {
    return std::make_unique<EmptyStmtNode>(helper::getPosition(ctx));
  }
  if (ctx->letStatement()) {
    return visitLetStmtNode(ctx->letStatement());
  } else {
    return visitExprStmtNode(ctx);
  }
}

std::unique_ptr<StmtNode>
ASTBuilder::visitExprStmtNode(RxParser::StatementContext *ctx) {
  auto stmtNode =
      std::make_unique<ExpressionStmtNode>(helper::getPosition(ctx));
  if (ctx->statementExpression()) {
    stmtNode->expr = visitExprNode(ctx->statementExpression());
  } else if (ctx->expressionWithBlock()) {
    stmtNode->expr = visitWithBlockExprNode(ctx->expressionWithBlock());
  }
  stmtNode->hasSemicolon = ctx->SEMI() != nullptr;
  return stmtNode;
}

std::unique_ptr<StmtNode>
ASTBuilder::visitLetStmtNode(RxParser::LetStatementContext *ctx) {
  auto letStmtNode = std::make_unique<LetStmtNode>(
      helper::getPosition(ctx), ctx->identifierBinding()->MUT() != nullptr,
      visitExprNode(ctx->expression()));
  if (ctx->typeRef() != nullptr) {
    letStmtNode->type = visitTypeNode(ctx->typeRef());
  }
  letStmtNode->identifier = ctx->identifierBinding()->identifier()->getText();
  return letStmtNode;
}

std::unique_ptr<BlockExprNode>
ASTBuilder::visitBlockExprNode(RxParser::BlockExpressionContext *ctx) {
  auto blockExpr = std::make_unique<BlockExprNode>(helper::getPosition(ctx));
  for (auto *stmt : ctx->statement()) {
    if (stmt != nullptr) {
      blockExpr->statements.push_back(visitStmtNode(stmt));
    }
  }
  if (auto *tail = ctx->statementExpression()) {
    blockExpr->tailExpr = visitExprNode(tail);
  } else if (!blockExpr->statements.empty()) {
    auto *lastStmt =
        dynamic_cast<ExpressionStmtNode *>(blockExpr->statements.back().get());

    if (lastStmt && !lastStmt->hasSemicolon && lastStmt->expr) {
      blockExpr->tailExpr = std::move(lastStmt->expr);
      blockExpr->statements.pop_back();
    }
  }
  return blockExpr;
}
