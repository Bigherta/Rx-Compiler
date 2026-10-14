#include "ASTPrinter.hpp"
#include "AST.hpp"
#include <cstddef>
#include <iostream>
#include <optional>
#include <variant>

std::any ASTPrinter::visitCrate(CrateNode *node) {
  std::cout << "Crate" << '\n';
  for (std::size_t i = 0; i < node->items.size(); ++i) {
    const bool isLast = i == node->items.size() - 1;
    std::cout << prefix << (isLast ? "└── " : "├── ");
    const std::string oldPrefix = prefix;
    prefix += (isLast ? "    " : "│   ");
    node->items[i]->accept(this);
    prefix = oldPrefix;
  }
  return {};
}

std::any ASTPrinter::visitFunctionDeclItem(FunctionDeclItemNode *node) {
  std::cout << "Function Declaration of " + node->identifier << '\n';
  if (node->args.hasLifeTime)
    std::cout << prefix << "├── Lifetime" << '\n';
  for (std::size_t i = 0; i < node->args.genericArgs.size(); ++i) {
    std::cout << prefix << "├── Generic Argument: ";
    const std::string oldPrefix = prefix;
    prefix += "│   ";
    node->args.genericArgs[i].genericArgType->accept(this);
    prefix = oldPrefix;
  }
  if (node->selfParam) {
    std::cout << prefix << "├── Self Parameter "
              << (node->selfParam->isRef ? "&" : "")
              << (node->selfParam->isMut ? "mut " : "") << "self" << '\n';
  }
  for (std::size_t i = 0; i < node->functionParam.size(); ++i) {
    std::cout << prefix << "├── Parameter "
              << (node->functionParam[i].isMut ? "mut " : "")
              << node->functionParam[i].identifier << ": ";
    const std::string oldPrefix = prefix;
    prefix += "│   ";
    node->functionParam[i].paramType->accept(this);
    prefix = oldPrefix;
  }
  if (node->returnType) {
    std::cout << prefix << "├── Return Type: ";
    const std::string oldPrefix = prefix;
    prefix += "│   ";
    node->returnType->get()->accept(this);
    prefix = oldPrefix;
  }
  std::cout << prefix << "└── ";
  const std::string oldPrefix = prefix;
  prefix += "    ";
  node->functionBody->accept(this);
  prefix = oldPrefix;
  return {};
}

std::any ASTPrinter::visitConstantDeclItem(ConstantDeclItemNode *node) {
  std::cout << "Constant Declaration of " + node->identifier << '\n';
  std::cout << prefix << "├── ";
  const std::string oldPrefix = prefix;
  prefix += "│   ";
  node->constType->accept(this);
  prefix = oldPrefix;

  std::cout << prefix << "└── ";
  prefix += "    ";
  node->constValue->accept(this);
  prefix = oldPrefix;
  return {};
}

std::any ASTPrinter::visitStructDeclItem(StructDeclItemNode *node) {
  std::cout << "Struct Declaration of " + node->identifier << '\n';
  for (std::size_t i = 0; i < node->outerAttribute.size(); ++i) {
    const bool isLast =
        i == node->outerAttribute.size() - 1 && node->structFields.empty();
    std::cout << prefix << (isLast ? "└── " : "├── ");
    const std::string oldPrefix = prefix;
    prefix += (isLast ? "    " : "│   ");
    node->outerAttribute[i]->accept(this);
    prefix = oldPrefix;
  }
  for (std::size_t i = 0; i < node->structFields.size(); ++i) {
    const bool isLast = i == node->structFields.size() - 1;
    std::cout << prefix << (isLast ? "└── " : "├── ");
    const std::string oldPrefix = prefix;
    prefix += (isLast ? "    " : "│   ");
    std::cout << "struct member " + node->structFields[i].identifier << ": ";
    node->structFields[i].fieldType->accept(this);
    prefix = oldPrefix;
  }
  return {};
}

std::any ASTPrinter::visitImplDeclItem(ImplDeclItemNode *node) {
  std::cout << "Implementation Declaration" << '\n';
  std::cout << prefix << (node->associatedItems.empty() ? "└── " : "├── ");
  const std::string oldPrefix = prefix;
  prefix += (node->associatedItems.empty() ? "    " : "│   ");
  node->implType->accept(this);
  prefix = oldPrefix;
  for (std::size_t i = 0; i < node->associatedItems.size(); ++i) {
    const bool isLast = i == node->associatedItems.size() - 1;
    std::cout << prefix << (isLast ? "└── " : "├── ");
    prefix += (isLast ? "    " : "│   ");
    std::visit([&](auto &&item) { item->accept(this); },
               node->associatedItems[i]);
    prefix = oldPrefix;
  }
  return {};
}

std::any ASTPrinter::visitOuterAttribute(OuterAttributeNode *node) {
  std::cout << "OuterAttribute" << '\n';
  for (std::size_t i = 0; i < node->outerAttributes.size(); ++i) {
    const bool isLast = i == node->outerAttributes.size() - 1;
    std::cout << prefix << (isLast ? "└── " : "├── ");
    switch (node->outerAttributes[i]) {
    case Attribute::Copy: {
      std::cout << "Copy" << '\n';
      break;
    }
    case Attribute::Clone: {
      std::cout << "Clone" << '\n';
      break;
    }
    case Attribute::Eq: {
      std::cout << "Eq" << '\n';
      break;
    }
    case Attribute::PartialEq: {
      std::cout << "PartialEq" << '\n';
      break;
    }
    }
  }
  return {};
}

std::any ASTPrinter::visitUnitType(UnitTypeNode *) {
  std::cout << "Unit Type" << '\n';
  return {};
}

std::any ASTPrinter::visitPathInType(PathInTypeNode *node) {
  std::cout << "Path In Type" << '\n';
  for (std::size_t i = 0; i < node->path.segments.size(); ++i) {
    const bool isLast = i == node->path.segments.size() - 1;
    std::cout << prefix << (isLast ? "└── " : "├── ")
              << node->path.segments[i].pathIdentSegment << '\n';
    const std::string oldPrefix = prefix;
    prefix += (isLast ? "    " : "│   ");
    const auto &args = node->path.segments[i].args;
    if (args.hasLifeTime)
      std::cout << prefix << (args.genericArgs.empty() ? "└── " : "├── ")
                << "Lifetime" << '\n';
    for (std::size_t j = 0; j < args.genericArgs.size(); ++j) {
      const bool isLastArg = j == args.genericArgs.size() - 1;
      std::cout << prefix << (isLastArg ? "└── " : "├── ")
                << "Generic Argument: ";
      const std::string segmentPrefix = prefix;
      prefix += (isLastArg ? "    " : "│   ");
      args.genericArgs[j].genericArgType->accept(this);
      prefix = segmentPrefix;
    }
    prefix = oldPrefix;
  }
  return {};
}

std::any ASTPrinter::visitReferenceType(ReferenceTypeNode *node) {
  std::cout << "Reference Type &"
            << (node->isMut ? "mut" : "") << '\n';
  std::cout << prefix << "└── ";
  const std::string oldPrefix = prefix;
  prefix += "    ";
  node->refType->accept(this);
  prefix = oldPrefix;
  return {};
}

std::any ASTPrinter::visitArrayType(ArrayTypeNode *node) {
  std::cout << "Array Type" << '\n';
  std::cout << prefix << "├── Element Type: ";
  const std::string oldPrefix = prefix;
  prefix += "│   ";
  node->typeParam->accept(this);
  prefix = oldPrefix;

  std::cout << prefix << "└── Length: ";
  prefix += "    ";
  node->length->accept(this);
  prefix = oldPrefix;
  return {};
}

std::any ASTPrinter::visitEmptyStmt(EmptyStmtNode *) {
  std::cout << "Empty Statement" << '\n';
  return {};
}

std::any ASTPrinter::visitLetStmt(LetStmtNode *node) {
  std::cout << "Let Statement " << (node->isMut ? "mut " : "")
            << node->identifier << '\n';
  if (node->type) {
    std::cout << prefix << (node->value ? "├── " : "└── ") << "Type: ";
    const std::string oldPrefix = prefix;
    prefix += (node->value ? "│   " : "    ");
    node->type->get()->accept(this);
    prefix = oldPrefix;
  }
  if (node->value) {
    std::cout << prefix << "└── Value: ";
    const std::string oldPrefix = prefix;
    prefix += "    ";
    node->value->accept(this);
    prefix = oldPrefix;
  }
  return {};
}

std::any ASTPrinter::visitExpressionStmt(ExpressionStmtNode *node) {
  std::cout << (std::string) "Expression Statement " +
                   (node->hasSemicolon ? "with semicolon" : "without semicolon")
            << '\n';
  std::cout << prefix << "└── ";
  const std::string oldPrefix = prefix;
  prefix += "    ";
  node->expr->accept(this);
  prefix = oldPrefix;
  return {};
}

std::any ASTPrinter::visitUnitExpr(UnitExprNode *) {
  std::cout << "Unit Expr" << '\n';
  return {};
}

std::any ASTPrinter::visitPathInExpr(PathInExprNode *node) {
  std::cout << "Path In Expr" << '\n';
  for (std::size_t i = 0; i < node->path.segments.size(); ++i) {
    const bool isLast = i == node->path.segments.size() - 1;
    std::cout << prefix << (isLast ? "└── " : "├── ")
              << node->path.segments[i].pathIdentSegment << '\n';
    const std::string oldPrefix = prefix;
    prefix += (isLast ? "    " : "│   ");
    const auto &args = node->path.segments[i].args;
    if (args.hasLifeTime)
      std::cout << prefix << (args.genericArgs.empty() ? "└── " : "├── ")
                << "Lifetime" << '\n';
    for (std::size_t j = 0; j < args.genericArgs.size(); ++j) {
      const bool isLastArg = j == args.genericArgs.size() - 1;
      std::cout << prefix << (isLastArg ? "└── " : "├── ")
                << "Generic Argument: ";
      const std::string segmentPrefix = prefix;
      prefix += (isLastArg ? "    " : "│   ");
      args.genericArgs[j].genericArgType->accept(this);
      prefix = segmentPrefix;
    }
    prefix = oldPrefix;
  }
  return {};
}

std::any ASTPrinter::visitLiteralExpr(LiteralExprNode *node) {
  std::cout << "Literal Expr with its value " + node->literalValue << '\n';
  return {};
}

std::any ASTPrinter::visitUnaryExpr(UnaryExprNode *node) {
  std::cout << "Unary Expr " << (node->op == UnaryOperator::NEG ? "-" : "!")
            << '\n';
  std::cout << prefix << "└── ";
  const std::string oldPrefix = prefix;
  prefix += "    ";
  node->childExpr->accept(this);
  prefix = oldPrefix;
  return {};
}

std::any ASTPrinter::visitBinaryExpr(BinaryExprNode *node) {
  std::cout << "Binary Expr ";
  switch (node->op) {
  case BinaryOperator::Add: {
    std::cout << "+";
    break;
  }
  case BinaryOperator::Sub: {
    std::cout << "-";
    break;
  }
  case BinaryOperator::Mul: {
    std::cout << "*";
    break;
  }
  case BinaryOperator::Div: {
    std::cout << "/";
    break;
  }
  case BinaryOperator::Mod: {
    std::cout << "%";
    break;
  }
  case BinaryOperator::BitAnd: {
    std::cout << "&";
    break;
  }
  case BinaryOperator::BitOr: {
    std::cout << "|";
    break;
  }
  case BinaryOperator::BitXor: {
    std::cout << "^";
    break;
  }
  case BinaryOperator::ShiftLeft: {
    std::cout << "<<";
    break;
  }
  case BinaryOperator::ShiftRight: {
    std::cout << ">>";
    break;
  }
  }
  std::cout << '\n';
  std::cout << prefix << "├── Left: ";
  const std::string oldPrefix = prefix;
  prefix += "│   ";
  node->leftChildExpr->accept(this);
  prefix = oldPrefix;

  std::cout << prefix << "└── Right: ";
  prefix += "    ";
  node->rightChildExpr->accept(this);
  prefix = oldPrefix;
  return {};
}

std::any ASTPrinter::visitComparisonExpr(ComparisonExprNode *node) {
  std::cout << "Comparison Expr ";
  switch (node->op) {
  case ComparisonOperator::equal: {
    std::cout << "==";
    break;
  }
  case ComparisonOperator::notEqual: {
    std::cout << "!=";
    break;
  }
  case ComparisonOperator::greaterThan: {
    std::cout << ">";
    break;
  }
  case ComparisonOperator::lessThan: {
    std::cout << "<";
    break;
  }
  case ComparisonOperator::greaterThanOrEqual: {
    std::cout << ">=";
    break;
  }
  case ComparisonOperator::lessThanOrEqual: {
    std::cout << "<=";
    break;
  }
  }
  std::cout << '\n';
  std::cout << prefix << "├── Left: ";
  const std::string oldPrefix = prefix;
  prefix += "│   ";
  node->leftChildExpr->accept(this);
  prefix = oldPrefix;

  std::cout << prefix << "└── Right: ";
  prefix += "    ";
  node->rightChildExpr->accept(this);
  prefix = oldPrefix;
  return {};
}

std::any ASTPrinter::visitLogicExpr(LogicExprNode *node) {
  std::cout << "Logic Expr "
            << (node->op == LogicOperator::LogicAnd ? "&&" : "||") << '\n';
  std::cout << prefix << "├── Left: ";
  const std::string oldPrefix = prefix;
  prefix += "│   ";
  node->leftChildExpr->accept(this);
  prefix = oldPrefix;

  std::cout << prefix << "└── Right: ";
  prefix += "    ";
  node->rightChildExpr->accept(this);
  prefix = oldPrefix;
  return {};
}

std::any ASTPrinter::visitCastExpr(CastExprNode *node) {
  std::cout << "Cast Expr" << '\n';
  std::cout << prefix << "├── Value: ";
  const std::string oldPrefix = prefix;
  prefix += "│   ";
  node->castValue->accept(this);
  prefix = oldPrefix;

  std::cout << prefix << "└── Type: ";
  prefix += "    ";
  node->castType->accept(this);
  prefix = oldPrefix;
  return {};
}

std::any ASTPrinter::visitBorrowExpr(BorrowExprNode *node) {
  std::cout << "Borrow Expr &" << (node->isMut ? "mut" : "") << '\n';
  std::cout << prefix << "└── ";
  const std::string oldPrefix = prefix;
  prefix += "    ";
  node->borrowExpr->accept(this);
  prefix = oldPrefix;
  return {};
}

std::any ASTPrinter::visitDereferenceExpr(DereferenceExprNode *node) {
  std::cout << "Dereference Expr *" << '\n';
  std::cout << prefix << "└── ";
  const std::string oldPrefix = prefix;
  prefix += "    ";
  node->dereferenceExpr->accept(this);
  prefix = oldPrefix;
  return {};
}

std::any ASTPrinter::visitAssignmentExpr(AssignmentExprNode *node) {
  std::cout << "Assignment Expr ";
  switch (node->assignmentOp) {
  case AssignmentOperator::Assignment: {
    std::cout << "=";
    break;
  }
  case AssignmentOperator::additionAssignment: {
    std::cout << "+=";
    break;
  }
  case AssignmentOperator::subtractionAssignment: {
    std::cout << "-=";
    break;
  }
  case AssignmentOperator::multiplicationAssignment: {
    std::cout << "*=";
    break;
  }
  case AssignmentOperator::divisionAssignment: {
    std::cout << "/=";
    break;
  }
  case AssignmentOperator::moduloAssignment: {
    std::cout << "%=";
    break;
  }
  case AssignmentOperator::ANDAssignment: {
    std::cout << "&=";
    break;
  }
  case AssignmentOperator::ORAssignment: {
    std::cout << "|=";
    break;
  }
  case AssignmentOperator::XORAssignment: {
    std::cout << "^=";
    break;
  }
  case AssignmentOperator::leftShiftAssignment: {
    std::cout << "<<=";
    break;
  }
  case AssignmentOperator::rightShiftAssignment: {
    std::cout << ">>=";
    break;
  }
  }
  std::cout << '\n';
  std::cout << prefix << "├── Left: ";
  const std::string oldPrefix = prefix;
  prefix += "│   ";
  node->leftChildExpr->accept(this);
  prefix = oldPrefix;

  std::cout << prefix << "└── Right: ";
  prefix += "    ";
  node->rightChildExpr->accept(this);
  prefix = oldPrefix;
  return {};
}

std::any ASTPrinter::visitMagnitudeExpr(MagnitudeExprNode *node) {
  std::cout << "Magnitude Expr" << '\n';
  std::cout << prefix << "└── ";
  if (const auto *value = std::get_if<std::string>(&node->magnitude)) {
    std::cout << *value << '\n';
  } else {
    const std::string oldPrefix = prefix;
    prefix += "    ";
    std::get<std::unique_ptr<PathInExprNode>>(node->magnitude)->accept(this);
    prefix = oldPrefix;
  }
  return {};
}

std::any ASTPrinter::visitConstExpr(ConstExprNode *node) {
  std::cout << "Const Expr" << (node->isNegative ? " -" : "") << '\n';
  std::cout << prefix << "└── ";
  const std::string oldPrefix = prefix;
  prefix += "    ";
  std::visit([&](auto &&value) { value->accept(this); }, node->constValue);
  prefix = oldPrefix;
  return {};
}

std::any ASTPrinter::visitArrayExpr(ArrayExprNode *node) {
  using Elements = std::vector<std::unique_ptr<ExprNode>>;
  if (const auto *elements = std::get_if<Elements>(&node->arrayEntries)) {
    std::cout << "Array Expr" << '\n';
    for (std::size_t i = 0; i < elements->size(); ++i) {
      const bool isLast = i == elements->size() - 1;
      std::cout << prefix << (isLast ? "└── " : "├── ");
      const std::string oldPrefix = prefix;
      prefix += (isLast ? "    " : "│   ");
      (*elements)[i]->accept(this);
      prefix = oldPrefix;
    }
  } else {
    std::cout << "Array Repeat Expr" << '\n';
    const auto &repeat = std::get<
        std::pair<std::unique_ptr<ExprNode>, std::unique_ptr<ConstExprNode>>>(
        node->arrayEntries);
    std::cout << prefix << "├── Value: ";
    const std::string oldPrefix = prefix;
    prefix += "│   ";
    repeat.first->accept(this);
    prefix = oldPrefix;

    std::cout << prefix << "└── Length: ";
    prefix += "    ";
    repeat.second->accept(this);
    prefix = oldPrefix;
  }
  return {};
}

std::any ASTPrinter::visitIndexExpr(IndexExprNode *node) {
  std::cout << "Index Expr" << '\n';
  std::cout << prefix << "├── Array: ";
  const std::string oldPrefix = prefix;
  prefix += "│   ";
  node->arrayExpr->accept(this);
  prefix = oldPrefix;

  std::cout << prefix << "└── Index: ";
  prefix += "    ";
  node->indexExpr->accept(this);
  prefix = oldPrefix;
  return {};
}

std::any ASTPrinter::visitStructExpr(StructExprNode *node) {
  std::cout << "Struct Expr" << '\n';
  std::cout << prefix << (node->structExprFields.empty() ? "└── " : "├── ");
  const std::string oldPrefix = prefix;
  prefix += (node->structExprFields.empty() ? "    " : "│   ");
  node->path->accept(this);
  prefix = oldPrefix;
  for (std::size_t i = 0; i < node->structExprFields.size(); ++i) {
    const bool isLast = i == node->structExprFields.size() - 1;
    std::cout << prefix << (isLast ? "└── " : "├── ") << "Field "
              << node->structExprFields[i].identifier << ": ";
    prefix += (isLast ? "    " : "│   ");
    node->structExprFields[i].fieldExpr->accept(this);
    prefix = oldPrefix;
  }
  return {};
}

std::any ASTPrinter::visitCallExpr(CallExprNode *node) {
  std::cout << "Call Expr" << '\n';
  std::cout << prefix << (node->callParams.empty() ? "└── " : "├── ")
            << "Callee: ";
  const std::string oldPrefix = prefix;
  prefix += (node->callParams.empty() ? "    " : "│   ");
  node->callee->accept(this);
  prefix = oldPrefix;
  for (std::size_t i = 0; i < node->callParams.size(); ++i) {
    const bool isLast = i == node->callParams.size() - 1;
    std::cout << prefix << (isLast ? "└── " : "├── ") << "Argument " << i
              << ": ";
    prefix += (isLast ? "    " : "│   ");
    node->callParams[i]->accept(this);
    prefix = oldPrefix;
  }
  return {};
}

std::any ASTPrinter::visitMethodCallExpr(MethodCallExprNode *node) {
  std::cout << "Method Call Expr " << node->callMethod.pathIdentSegment << '\n';
  const auto &args = node->callMethod.args;
  const bool hasGenerics = args.hasLifeTime || !args.genericArgs.empty();
  const bool isLast = !hasGenerics && node->callParams.empty();
  std::cout << prefix << (isLast ? "└── " : "├── ") << "Receiver: ";
  const std::string oldPrefix = prefix;
  prefix += (isLast ? "    " : "│   ");
  node->callee->accept(this);
  prefix = oldPrefix;
  if (args.hasLifeTime)
    std::cout << prefix
              << (args.genericArgs.empty() && node->callParams.empty() ? "└── "
                                                                       : "├── ")
              << "Lifetime" << '\n';
  for (std::size_t i = 0; i < args.genericArgs.size(); ++i) {
    const bool isLastArg =
        i == args.genericArgs.size() - 1 && node->callParams.empty();
    std::cout << prefix << (isLastArg ? "└── " : "├── ")
              << "Generic Argument: ";
    prefix += (isLastArg ? "    " : "│   ");
    args.genericArgs[i].genericArgType->accept(this);
    prefix = oldPrefix;
  }
  for (std::size_t i = 0; i < node->callParams.size(); ++i) {
    const bool isLastArg = i == node->callParams.size() - 1;
    std::cout << prefix << (isLastArg ? "└── " : "├── ") << "Argument " << i
              << ": ";
    prefix += (isLastArg ? "    " : "│   ");
    node->callParams[i]->accept(this);
    prefix = oldPrefix;
  }
  return {};
}

std::any ASTPrinter::visitReturnExpr(ReturnExprNode *node) {
  std::cout << "Return Expr" << '\n';
  if (node->returnValue) {
    std::cout << prefix << "└── ";
    const std::string oldPrefix = prefix;
    prefix += "    ";
    node->returnValue->get()->accept(this);
    prefix = oldPrefix;
  }
  return {};
}

std::any ASTPrinter::visitAccessExpr(AccessExprNode *node) {
  std::cout << "Access Expr ." << node->identifier << '\n';
  std::cout << prefix << "└── ";
  const std::string oldPrefix = prefix;
  prefix += "    ";
  node->accessObject->accept(this);
  prefix = oldPrefix;
  return {};
}

std::any ASTPrinter::visitContinueExpr(ContinueExprNode *) {
  std::cout << "Continue Expr" << '\n';
  return {};
}

std::any ASTPrinter::visitBreakExpr(BreakExprNode *node) {
  std::cout << "Break Expr" << '\n';
  if (node->loopValue) {
    std::cout << prefix << "└── ";
    const std::string oldPrefix = prefix;
    prefix += "    ";
    node->loopValue->get()->accept(this);
    prefix = oldPrefix;
  }
  return {};
}

std::any ASTPrinter::visitBlockExpr(BlockExprNode *node) {
  std::cout << "Block Expression" << '\n';
  for (std::size_t i = 0; i < node->statements.size(); ++i) {
    const bool isLast =
        i == node->statements.size() - 1 && node->tailExpr == std::nullopt;
    std::cout << prefix << (isLast ? "└── " : "├── ");
    const std::string oldPrefix = prefix;
    prefix += (isLast ? "    " : "│   ");
    node->statements[i]->accept(this);
    prefix = oldPrefix;
  }
  if (node->tailExpr) {
    std::cout << prefix << "└── ";
    const std::string oldPrefix = prefix;
    prefix += "    ";
    node->tailExpr->get()->accept(this);
    prefix = oldPrefix;
  }
  return {};
}

std::any ASTPrinter::visitIfExpr(IfExprNode *node) {
  std::cout << "If Expr" << '\n';
  std::cout << prefix << "├── Condition: ";
  const std::string oldPrefix = prefix;
  prefix += "│   ";
  node->conditions->accept(this);
  prefix = oldPrefix;

  const bool isLast = !node->elseExpr && !node->elseIfExpr;
  std::cout << prefix << (isLast ? "└── " : "├── ") << "Then: ";
  prefix += (isLast ? "    " : "│   ");
  node->body->accept(this);
  prefix = oldPrefix;
  if (node->elseIfExpr) {
    std::cout << prefix << (node->elseExpr ? "├── " : "└── ") << "Else If: ";
    prefix += (node->elseExpr ? "│   " : "    ");
    node->elseIfExpr->get()->accept(this);
    prefix = oldPrefix;
  }
  if (node->elseExpr) {
    std::cout << prefix << "└── Else: ";
    prefix += "    ";
    node->elseExpr->get()->accept(this);
    prefix = oldPrefix;
  }
  return {};
}

std::any ASTPrinter::visitInfiniteLoopExpr(InfiniteLoopExprNode *node) {
  std::cout << "Infinite Loop Expr" << '\n';
  std::cout << prefix << "└── ";
  const std::string oldPrefix = prefix;
  prefix += "    ";
  node->body->accept(this);
  prefix = oldPrefix;
  return {};
}

std::any ASTPrinter::visitPredicateLoopExpr(PredicateLoopExprNode *node) {
  std::cout << "Predicate Loop Expr" << '\n';
  std::cout << prefix << "├── Condition: ";
  const std::string oldPrefix = prefix;
  prefix += "│   ";
  node->conditions->accept(this);
  prefix = oldPrefix;

  std::cout << prefix << "└── Body: ";
  prefix += "    ";
  node->body->accept(this);
  prefix = oldPrefix;
  return {};
}
